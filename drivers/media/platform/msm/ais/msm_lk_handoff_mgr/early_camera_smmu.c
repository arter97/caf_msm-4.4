/* Copyright (c) 2019, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/types.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/stddef.h>
#include <asm/dma-iommu.h>
#include "early_camera_smmu.h"

static struct early_camera_info *cinfo;

int get_pages_and_sgt_from_pa(struct page **pages, struct sg_table **psgt,
	struct early_camera_info *cinfo)
{
	dma_addr_t paddr;
	int npages = cinfo->earlycam_mem_size >> PAGE_SHIFT;
	int i, ret;
	struct sg_table *sgt;

	pr_debug("npages = %d, earlycam_mem_size = %zu\n",
		npages, cinfo->earlycam_mem_size);
	pages = __vmalloc(sizeof(struct page *) * npages,
		GFP_KERNEL | __GFP_HIGHMEM, PAGE_KERNEL);
	if (pages == NULL) {
		pr_err("alloc pages failed\n");
		return -ENOMEM;
	}

	paddr = cinfo->earlycam_mem_paddr;

	for (i = 0; i < npages; i++) {
		pages[i] = phys_to_page(paddr);
		paddr += PAGE_SIZE;
	}

	sgt = kmalloc(sizeof(struct sg_table), GFP_KERNEL);
	if (!sgt) {
		pr_err("failed to allocate sgt\n");
		ret = -ENOMEM;
		goto out;
	}

	ret = sg_alloc_table_from_pages(sgt, pages, npages, 0,
		npages << PAGE_SHIFT, GFP_KERNEL);
	if (ret) {
		pr_err("failed to populate sgt from pages\n");
		return -ENOMEM;
	}
	*psgt = sgt;
	return 0;
out:
	kfree(sgt);
	return ret;
}

int early_camera_mem_parse_dt(struct device *dev,
	struct early_camera_info *cinfo)
{
	int ret = 0;
	size_t size = 0;
	struct device_node *node;
	dma_addr_t start;

	node = of_parse_phandle(dev->of_node,
		"linux,contiguous-region", 0);
	if (node) {
		struct resource r;

		ret = of_address_to_resource(node, 0, &r);
		if (ret)
			return ret;

		size = r.end - r.start;
		size = PAGE_ALIGN(size);
		start = (dma_addr_t)r.start;
		pr_debug("r.start = %llx, r.end = %llx\n", r.start, r.end);
		pr_debug("size = %zu, start = %llx\n", size, start);
		cinfo->earlycam_mem_paddr = start;
		cinfo->earlycam_mem_size = size;
		of_node_put(node);
	} else {
		pr_err("Error: Early camera node not found");
		ret = -1;
	}
	return ret;
}

int cam_smmu_early_camera_init(struct device *dev)
{
	int rc = 0;

	cinfo = kmalloc(sizeof(struct early_camera_info), GFP_KERNEL);
	if (cinfo == NULL) {
		pr_err("alloc cinfo failed\n");
		return -ENOMEM;
	}
	pr_debug("alloc cinfo %pK\n", cinfo);

	cinfo->msm_obj =
		kmalloc(sizeof(struct msm_cam_object), GFP_KERNEL);
	if (cinfo->msm_obj == NULL) {
		pr_err("alloc msm_obj failed\n");
		goto error2;
	}
	pr_debug("alloc msm_obj %pK\n", cinfo->msm_obj);

	rc = early_camera_mem_parse_dt(dev, cinfo);
	if (rc) {
		pr_err("parse memory dt failed: %d\n", rc);
		goto error1;
	}
	pr_debug("parse memory dt success\n");
	cinfo->smmu_early_init_done = true;

	return rc;

error1:
	kfree(cinfo->msm_obj);
	cinfo->msm_obj = NULL;
error2:
	kfree(cinfo);
	cinfo = NULL;
	return -ENOMEM;
}
EXPORT_SYMBOL(cam_smmu_early_camera_init);

void cam_smmu_early_camera_deinit(void)
{
	if (cinfo) {
		if (cinfo->msm_obj) {
			kfree(cinfo->msm_obj->pages);
			cinfo->msm_obj->pages = NULL;
			kfree(cinfo->msm_obj->sgt);
			cinfo->msm_obj->sgt = NULL;
		}
		kfree(cinfo->msm_obj);
		cinfo->msm_obj = NULL;
	}
	kfree(cinfo);
	cinfo = NULL;
}
EXPORT_SYMBOL(cam_smmu_early_camera_deinit);

int cam_smmu_early_camera_physical_to_physical_map(
	struct dma_iommu_mapping *mapping)
{
	size_t bytes_map = 0;
	int rc = 0;

	/* get the status of init before proceeding for map */
	if (cinfo->smmu_early_init_done) {
	rc = get_pages_and_sgt_from_pa(cinfo->msm_obj->pages,
		&cinfo->msm_obj->sgt, cinfo);
	if (rc) {
		pr_err("early_cam_set_pages_and_sgt failed\n");
		goto error;
	}
	pr_debug("cinfo->msm_obj->sgt->sgl = %pK",
		cinfo->msm_obj->sgt->sgl);
	pr_debug("cinfo->msm_obj->sgt->nents = %d\n",
		cinfo->msm_obj->sgt->nents);

	bytes_map = iommu_map_sg(mapping->domain,
		cinfo->earlycam_mem_paddr,
		cinfo->msm_obj->sgt->sgl,
		cinfo->msm_obj->sgt->nents,
		IOMMU_WRITE | IOMMU_NOEXEC);
	pr_debug("iommu_map_sg = %zu\n", bytes_map);
	if (!bytes_map) {
		pr_err("iommu_map_sg failed\n");
		goto error;
	}
	} else {
		pr_err("cam_smmu_early_camera_init not called\n");
		rc = -1;
	}

error:
	return rc;
}
EXPORT_SYMBOL(cam_smmu_early_camera_physical_to_physical_map);

void cam_smmu_early_camera_config_stage1_translation(
	struct dma_iommu_mapping *mapping, int enable)
{
	int data_en = 0;

	if (enable)
		/* 0 means enable stage 1 translation */
		data_en = 0;
	else
		/* 1 means disable stage 1 translation */
		data_en = 1;

	iommu_domain_set_attr(mapping->domain, DOMAIN_ATTR_EARLY_MAP, &data_en);
}
EXPORT_SYMBOL(cam_smmu_early_camera_config_stage1_translation);

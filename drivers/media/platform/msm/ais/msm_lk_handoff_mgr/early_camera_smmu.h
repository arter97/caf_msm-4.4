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

#include <linux/iommu.h>

struct early_camera_info {
	/* get status of smmu init */
	bool smmu_early_init_done;

	/* physical address of memory node for camera buffer */
	phys_addr_t earlycam_mem_paddr;

	/* size of memory node */
	size_t earlycam_mem_size;

	/* camera objects for smmu mapping */
	struct msm_cam_object *msm_obj;
};

struct msm_cam_object {
	struct page **pages;
	struct sg_table *sgt;
};

int cam_smmu_early_camera_init(struct device *dev);
void cam_smmu_early_camera_deinit(void);
void cam_smmu_early_camera_config_stage1_translation(
		struct dma_iommu_mapping *mapping, int enable);
int cam_smmu_early_camera_physical_to_physical_map(
		struct dma_iommu_mapping *mapping);

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

/* This file is to handle LK to edrm app transition */

#include "msm_early_cam.h"
#include "msm_isp.h"

static bool lk_running;
static bool smmu_attached;

void msm_init_lk_handoff_driver(void)
{
	/* Query once and store LK status */
	lk_running = msm_early_camera_running();
}

/* Return stored value to avoid conflicting status */
bool msm_lk_handoff_get_lk_status(bool cached)
{
	if (cached)
		return (lk_running && smmu_attached);
	else
		/* Query actual status from LK */
		return (smmu_attached && (msm_early_camera_running()));
}

void msm_lk_handoff_update_smmu_status(bool status)
{
	/* Store early camera smmu attach status */
	smmu_attached = status;
}

void msm_lk_handoff_kill_lk_camera(void)
{
	msm_kill_early_camera();
}

/* VFE functions */
int msm_lk_handoff_vfe47_init_hardware(void *vfedev)
{
	struct vfe_device *vfe_dev = (struct vfe_device *)vfedev;

	if (lk_running && smmu_attached && (vfe_dev->pdev->id == 0)) {
		/* Fill dummy data */
		vfe_dev->ahb_vote = CAM_AHB_SVS_VOTE;
		vfe_dev->common_data->dual_vfe_res->vfe_base[vfe_dev->pdev->id]
			= vfe_dev->vfe_base;
		return 1; /* Success */
	}
	return 0;
}

int msm_lk_handoff_vfe47_release_hardware(void *vfedev)
{
	struct vfe_device *vfe_dev = (struct vfe_device *)vfedev;

	if (lk_running && smmu_attached && (vfe_dev->pdev->id == 0)) {
		/* Currently not perform hw operations, only status setting */
		vfe_dev->irq0_mask = 0;
		vfe_dev->irq1_mask = 0;
		vfe_dev->common_data->dual_vfe_res->vfe_base[vfe_dev->pdev->id]
			= NULL;
		vfe_dev->ahb_vote = CAM_AHB_SUSPEND_VOTE;
		return 1; /* Success */
	}
	return 0;
}

int msm_lk_handoff_vfe47_clear_status_reg(void *vfedev)
{
	struct vfe_device *vfe_dev = (struct vfe_device *)vfedev;

	if (lk_running && smmu_attached && (vfe_dev->pdev->id == 0))
		return 1; /* Success */
	return 0;
}

int msm_lk_handoff_vfe47_reset_hardware(void *vfedev)
{
	struct vfe_device *vfe_dev = (struct vfe_device *)vfedev;

	if (lk_running && smmu_attached && (vfe_dev->pdev->id == 0))
		return 1; /* Success */
	return 0;
}

int msm_lk_handoff_vfe47_init_hardware_reg(void *vfedev)
{
	struct vfe_device *vfe_dev = (struct vfe_device *)vfedev;

	if (lk_running && smmu_attached && (vfe_dev->pdev->id == 0))
		return 1; /* Success */
	return 0;
}

int msm_lk_handoff_vfe47_enable_clks(void *vfedev)
{
	struct vfe_device *vfe_dev = (struct vfe_device *)vfedev;

	if (lk_running && smmu_attached && (vfe_dev->pdev->id == 0))
		return 1; /* Success */
	return 0;
}

int msm_lk_handoff_vfe47_enable_regulators(void *vfedev)
{
	struct vfe_device *vfe_dev = (struct vfe_device *)vfedev;

	if (lk_running && smmu_attached && (vfe_dev->pdev->id == 0))
		return 1; /* Success */
	return 0;
}

int msm_lk_handoff_vfe47_process_ops(void *vfedev)
{
	struct vfe_device *vfe_dev = (struct vfe_device *)vfedev;

	if (lk_running && smmu_attached && (vfe_dev->pdev->id == 0))
		return 1; /* Success */
	return 0;
}

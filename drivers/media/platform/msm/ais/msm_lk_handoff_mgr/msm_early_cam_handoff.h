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

#ifndef MSM_EARLY_CAM_HANDOFF_H
#define MSM_EARLY_CAM_HANDOFF_H

void msm_init_lk_handoff_driver(void);
bool msm_lk_handoff_get_lk_status(bool cached);
void msm_lk_handoff_update_smmu_status(bool status);
void msm_lk_handoff_kill_lk_camera(void);

/* VFE functions */
int msm_lk_handoff_vfe47_init_hardware(void *vfedev);
int msm_lk_handoff_vfe47_release_hardware(void *vfedev);
int msm_lk_handoff_vfe47_clear_status_reg(void *vfedev);
int msm_lk_handoff_vfe47_reset_hardware(void *vfedev);
int msm_lk_handoff_vfe47_init_hardware_reg(void *vfedev);
int msm_lk_handoff_vfe47_enable_clks(void *vfedev);
int msm_lk_handoff_vfe47_enable_regulators(void *vfedev);
int msm_lk_handoff_vfe47_process_ops(void *vfedev);

#endif /* MSM_EARLY_CAM_HANDOFF_H */

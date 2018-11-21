/*
 * Copyright (c) 2018, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/of_address.h>
#include <linux/debugfs.h>
#include <linux/memblock.h>

#include "msm_drv.h"
#include "sde_kms.h"
#include "edrm_kms.h"
#include "sde_splash.h"
#include "edrm_splash.h"

/* scratch registers */
#define SCRATCH_REGISTER_0		0x014
#define SCRATCH_REGISTER_1		0x018
#define SCRATCH_REGISTER_2		0x01C
#define SCRATCH_REGISTER_3		0x020

#define SDE_RUNNING_VALUE		0xC001CAFE
#define SDE_LK_STOP_VALUE		0xDEADDEAD
#define SDE_EXIT_VALUE			0xDEADBEEF
#define SDE_LK_IMMEDIATE_STOP_VALUE	0xFEFEFEFE

/**
 * edrm_splash_notify_lk_stop_splash.
 *
 * Function to stop early splash in LK.
 */
void edrm_splash_notify_lk_stop_splash(struct msm_kms *kms)
{
	struct msm_edrm_kms *edrm_kms = to_edrm_kms(kms);
	struct msm_drm_private *master_priv =
			edrm_kms->master_dev->dev_private;
	struct sde_kms *master_kms;

	master_kms = to_sde_kms(master_priv->kms);
	/* write splash stop signal to scratch register */
	writel_relaxed(SDE_LK_STOP_VALUE, master_kms->mmio +
			SCRATCH_REGISTER_1);
}

/**
 * edrm_splash_poll_lk_stop_splash.
 *
 * Function to poll for early splash stop in LK.
 */
void edrm_splash_poll_lk_stop_splash(struct msm_kms *kms)
{
	int i = 0;
	u32 reg_value = 0;
	struct msm_edrm_kms *edrm_kms = to_edrm_kms(kms);
	struct msm_drm_private *master_priv = edrm_kms->master_dev->dev_private;
	struct sde_kms *master_kms;

	master_kms = to_sde_kms(master_priv->kms);
	/* each read may wait up to 10000us, worst case polling is 4 sec */
	while (i < 400) {
		/* read LK status from scratch register*/
		reg_value = readl_relaxed(master_kms->mmio +
				SCRATCH_REGISTER_1);
		if (reg_value == SDE_EXIT_VALUE) {
			edrm_kms->lk_running_flag = false;
			break;
		}
		usleep_range(8000, 10000);
		i++;
	}
}

/*
 * Below function will indicate early display exited or not started.
 */
int edrm_splash_get_lk_status(struct msm_kms *kms)
{
	u32 reg_value = 0;
	struct msm_edrm_kms *edrm_kms = to_edrm_kms(kms);
	struct msm_drm_private *master_priv = edrm_kms->master_dev->dev_private;
	struct sde_kms *master_kms;

	master_kms = to_sde_kms(master_priv->kms);
	reg_value = readl_relaxed(master_kms->mmio + SCRATCH_REGISTER_1);
	switch (reg_value) {
	case SDE_RUNNING_VALUE:
		return SPLASH_STATUS_RUNNING;
	case SDE_EXIT_VALUE:
		return SPLASH_STATUS_NOT_START;
	}
	return SPLASH_STATUS_NOT_START;
}


/*
 * Below function indicates early display has started.
 */
void edrm_display_acquire(struct msm_kms *kms)
{
	struct msm_edrm_kms *edrm_kms = to_edrm_kms(kms);
	struct sde_kms *master_kms;
	struct sde_splash_info *master_sinfo;
	struct msm_drm_private *master_priv =
			edrm_kms->master_dev->dev_private;

	master_kms = to_sde_kms(master_priv->kms);
	master_sinfo = &master_kms->splash_info;
	master_sinfo->early_display_enabled = true;
}

/*
 * Below function indicates early display is exited or has not started.
 */
void edrm_display_release(struct msm_kms *kms)
{
	struct msm_edrm_kms *edrm_kms = to_edrm_kms(kms);
	struct sde_kms *master_kms;
	struct sde_splash_info *master_sinfo;
	struct msm_drm_private *master_priv =
			edrm_kms->master_dev->dev_private;

	master_kms = to_sde_kms(master_priv->kms);
	master_sinfo = &master_kms->splash_info;
	master_sinfo->early_display_enabled = false;
}

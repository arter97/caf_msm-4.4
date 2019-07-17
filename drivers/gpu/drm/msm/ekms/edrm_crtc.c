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

#include "edrm_crtc.h"
#include "edrm_plane.h"
#include "edrm_encoder.h"
#include "sde_kms.h"

/* display control path Flush register offset */
#define FLUSH_OFFSET   0x18

#define COMMIT_MAX_POLLING 40
static void edrm_crtc_atomic_flush(struct drm_crtc *crtc,
		struct drm_crtc_state *old_crtc_state)
{
	struct drm_plane *plane;
	unsigned plane_mask = 0;

	if (!crtc) {
		pr_err("invalid crtc\n");
		return;
	}

	drm_atomic_crtc_for_each_plane(plane, crtc) {
		if (!plane->state->fence)
			continue;

		WARN_ON(!plane->state->fb);

		fence_wait(plane->state->fence, false);
		fence_put(plane->state->fence);
		plane->state->fence = NULL;
	}

	plane_mask = old_crtc_state->plane_mask;
	plane_mask |= crtc->state->plane_mask;

	drm_for_each_plane_mask(plane, crtc->dev, plane_mask) {
		edrm_plane_flush(plane, crtc);
	}

	/* special handling for disable */
	if (!crtc->state->active) {
		/* commit to disable all planes */
		edrm_crtc_commit_kickoff(crtc);
		edrm_crtc_wait_for_commit_done(crtc);
	}
}

static void edrm_crtc_enable(struct drm_crtc *crtc)
{
	const struct drm_plane_helper_funcs *funcs;
	struct drm_plane *plane;

	drm_atomic_crtc_for_each_plane(plane, crtc) {
		funcs = plane->helper_private;
		funcs->atomic_update(plane, plane->state);
	}
}

static void edrm_crtc_disable(struct drm_crtc *crtc)
{
	const struct drm_plane_helper_funcs *funcs;
	struct drm_plane *plane;

	drm_atomic_crtc_for_each_plane(plane, crtc) {
		funcs = plane->helper_private;
		funcs->atomic_disable(plane, plane->state);
	}
}

void edrm_crtc_destroy(struct drm_crtc *crtc)
{
	struct edrm_crtc *edrm_crtc = to_edrm_crtc(crtc);

	drm_crtc_cleanup(crtc);
	kfree(edrm_crtc);
}

static const struct drm_crtc_funcs edrm_crtc_funcs = {
	.reset =  drm_atomic_helper_crtc_reset,
	.set_config = drm_atomic_helper_set_config,
	.destroy = edrm_crtc_destroy,
	.page_flip = drm_atomic_helper_page_flip,
	.atomic_duplicate_state = drm_atomic_helper_crtc_duplicate_state,
	.atomic_destroy_state = drm_atomic_helper_crtc_destroy_state,
};

static const struct drm_crtc_helper_funcs edrm_crtc_helper_funcs = {
	.disable = edrm_crtc_disable,
	.enable = edrm_crtc_enable,
	.atomic_flush = edrm_crtc_atomic_flush,
};

struct drm_crtc *edrm_crtc_init(struct drm_device *dev,
					struct msm_edrm_display *display,
					struct drm_plane *primary_plane)
{
	struct edrm_crtc *edrm_crtc;
	struct drm_crtc *crtc;
	int ret;

	edrm_crtc = kzalloc(sizeof(*edrm_crtc), GFP_KERNEL);
	if (!edrm_crtc) {
		ret = -ENOMEM;
		goto fail_no_mem;
	}

	crtc = &edrm_crtc->base;
	ret = drm_crtc_init_with_planes(dev, crtc, primary_plane, NULL,
		&edrm_crtc_funcs);
	if (ret)
		goto fail;

	drm_crtc_helper_add(crtc, &edrm_crtc_helper_funcs);
	edrm_crtc->display_id = display->display_id;

	return crtc;
fail:
	kfree(edrm_crtc);
fail_no_mem:
	return ERR_PTR(ret);
}

void edrm_crtc_commit_kickoff(struct drm_crtc *crtc)
{
	struct drm_device *dev;
	struct msm_drm_private *priv;
	struct msm_edrm_kms *edrm_kms;
	struct msm_edrm_display *display;
	struct edrm_crtc *edrm_crtc;
	struct sde_kms *master_kms;
	struct msm_drm_private *master_priv;
	u32 ctl_off;

	dev = crtc->dev;
	priv = dev->dev_private;
	edrm_kms = to_edrm_kms(priv->kms);
	master_priv = edrm_kms->master_dev->dev_private;
	master_kms = to_sde_kms(master_priv->kms);
	edrm_crtc = to_edrm_crtc(crtc);

	display = &edrm_kms->display[edrm_crtc->display_id];
	ctl_off = display->ctl_off;

	/* Trigger the flush */
	writel_relaxed(edrm_crtc->sspp_flush_mask, master_kms->mmio + ctl_off +
		FLUSH_OFFSET);
}

void edrm_crtc_complete_commit(struct drm_crtc *crtc,
		struct drm_crtc_state *old_state)
{
}

void edrm_crtc_prepare_commit(struct drm_crtc *crtc,
				struct drm_crtc_state *old_state)
{
}

int edrm_crtc_wait_for_commit_done(struct drm_crtc *crtc)
{
	struct drm_device *dev;
	struct msm_drm_private *priv;
	struct msm_edrm_kms *edrm_kms;
	struct msm_edrm_display *display;
	struct edrm_crtc *edrm_crtc;
	struct sde_kms *master_kms;
	struct msm_drm_private *master_priv;
	u32 ctl_off;
	u32 flush_register = 0;
	int i;

	dev = crtc->dev;
	priv = dev->dev_private;
	edrm_kms = to_edrm_kms(priv->kms);
	master_priv = edrm_kms->master_dev->dev_private;
	master_kms = to_sde_kms(master_priv->kms);
	edrm_crtc = to_edrm_crtc(crtc);
	display = &edrm_kms->display[edrm_crtc->display_id];
	ctl_off = display->ctl_off;

	/* poll edrm_crtc->sspp_flush_mask until cleared */
	for (i = 0; i < COMMIT_MAX_POLLING; i++) {
		flush_register = readl_relaxed(master_kms->mmio +
				ctl_off + 0x18);
		if ((flush_register & edrm_crtc->sspp_flush_mask) != 0)
			usleep_range(1000, 2000);
		else
			break;
	}
	if (i == COMMIT_MAX_POLLING)
		pr_err("flush polling times out %x\n", flush_register);

	/* reset sspp_flush_mask */
	edrm_crtc->sspp_flush_mask = 0;

	return 0;
}

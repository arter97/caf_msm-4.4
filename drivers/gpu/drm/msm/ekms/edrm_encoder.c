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

#include "edrm_encoder.h"
#include "edrm_crtc.h"
#include "sde_kms.h"

static void edrm_encoder_enable(struct drm_encoder *drm_enc)
{
	pr_debug("eDRM Encoder enable\n");
}

static void edrm_encoder_disable(struct drm_encoder *drm_enc)
{
	pr_debug("eDRM Encoder disable\n");
}

void edrm_encoder_destroy(struct drm_encoder *encoder)
{
	struct edrm_encoder *edrm_enc = to_edrm_encoder(encoder);

	drm_encoder_cleanup(encoder);
	kfree(edrm_enc);
}

static const struct drm_encoder_helper_funcs edrm_encoder_helper_funcs = {
	.disable = edrm_encoder_disable,
	.enable = edrm_encoder_enable,
};

static const struct drm_encoder_funcs edrm_encoder_funcs = {
	.destroy = edrm_encoder_destroy,
};

struct drm_encoder *edrm_encoder_init(struct drm_device *dev,
					struct msm_edrm_display *display)
{
	struct edrm_encoder *edrm_encoder;
	struct drm_encoder *encoder;
	int ret;

	edrm_encoder = kzalloc(sizeof(*edrm_encoder), GFP_KERNEL);
	if (!edrm_encoder)
		return ERR_PTR(-ENOMEM);

	encoder = &edrm_encoder->base;

	ret = drm_encoder_init(dev, encoder,
			&edrm_encoder_funcs,
			display->encoder_type);
	if (ret)
		goto fail;

	drm_encoder_helper_add(encoder, &edrm_encoder_helper_funcs);

	edrm_encoder->intf_idx = display->intf_id;

	return encoder;
fail:
	kfree(edrm_encoder);
	return ERR_PTR(ret);
}

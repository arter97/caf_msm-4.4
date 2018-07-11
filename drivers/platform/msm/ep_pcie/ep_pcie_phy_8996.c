/* Copyright (c) 2015-2016, 2018, The Linux Foundation. All rights reserved.
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

/*
 * MSM PCIe PHY endpoint mode
 */
#include <linux/delay.h>

#include "ep_pcie_com_8996.h"
#include "ep_pcie_phy_8996.h"

void ep_pcie_phy_init(struct ep_pcie_dev_t *dev)
{
	int i;
	struct ep_pcie_phy_info_t *phy_seq;

	EP_PCIE_DBG(dev,
		"PCIe V%d: PHY V%d: Initializing 14nm QMP phy - 100MHz\n",
		dev->rev, dev->phy_rev);

	if (dev->phy_sequence) {
		i =  dev->phy_len;
		phy_seq = dev->phy_sequence;
		while (i--) {
			ep_pcie_write_reg(dev->phy,
				phy_seq->offset,
				phy_seq->val);
			if (phy_seq->delay)
				usleep_range(phy_seq->delay,
					phy_seq->delay + 1);
			phy_seq++;
		}
	} else {
		EP_PCIE_ERR(dev, "PCIe V%d: PHY V%d: Missing phy sequence!\n",
			dev->rev, dev->phy_rev);
	}

	ep_pcie_write_reg(dev->phy, PCIE_COM_SW_RESET, 0x0);
	ep_pcie_write_reg(dev->phy, PCIE_COM_START_CONTROL, BIT(1) | BIT(0));
}

void ep_pcie_phy_bringup_port(struct ep_pcie_dev_t *dev)
{
	ep_pcie_write_reg(dev->phy, PCIE_PORT_POWER_DOWN_CONTROL, 0x03);
	ep_pcie_write_reg(dev->phy, PCIE_PORT_SW_RESET, 0x0);
	ep_pcie_write_reg(dev->phy, PCIE_PORT_START_CONTROL, 0x0a);
}

bool ep_pcie_phy_is_ready(struct ep_pcie_dev_t *dev)
{
	if (readl_relaxed(dev->phy + PCIE_COM_PCS_READY_STATUS) & BIT(0))
		return true;
	else
		return false;
}

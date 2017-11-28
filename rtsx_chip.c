/* Driver for Realtek PCI-Express card reader
 *
 * Copyright(c) 2009 Realtek Semiconductor Corp. All rights reserved.  
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, see <http:
 *
 * Author:
 *   wwang (wei_wang@realsil.com.cn)
 *   No. 450, Shenhu Road, Suzhou Industry Park, Suzhou, China
 */

#include <linux/blkdev.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/workqueue.h>

#include "rtsx.h"
#include "rtsx_transport.h"
#include "rtsx_scsi.h"
#include "rtsx_card.h"
#include "rtsx_chip.h"
#include "rtsx_sys.h"
#include "general.h"

#include "sd.h"
#include "ms.h"

void rtsx_disable_card_int(struct rtsx_chip *chip)
{
	u32 reg = rtsx_readl(chip, RTSX_BIER);
	
	reg &= ~(SD_INT_EN | MS_INT_EN);
	rtsx_writel(chip, RTSX_BIER, reg);
}

void rtsx_enable_card_int(struct rtsx_chip *chip)
{
	u32 reg = rtsx_readl(chip, RTSX_BIER);
	int i;
	
	for (i = 0; i <= chip->max_lun; i++) {
		
		if (chip->lun2card[i] & SD_CARD) {
			reg |= SD_INT_EN;
		}
		if (chip->lun2card[i] & MS_CARD) {
			reg |= MS_INT_EN;
		}
	}
	
	rtsx_writel(chip, RTSX_BIER, reg);
}

void rtsx_enable_bus_int(struct rtsx_chip *chip)
{
	u32 reg = 0;
#ifndef DISABLE_CARD_INT
	int i;
#endif

	reg = TRANS_OK_INT_EN | TRANS_FAIL_INT_EN | DELINK_INT_EN;
	
#ifndef DISABLE_CARD_INT
	for (i = 0; i <= chip->max_lun; i++) {
		RTSX_DEBUGP(("lun2card[%d] = 0x%02x\n", i, chip->lun2card[i]));
		
		if (chip->lun2card[i] & SD_CARD) {
			reg |= SD_INT_EN;
		}
		if (chip->lun2card[i] & MS_CARD) {
			reg |= MS_INT_EN;
		}
	}
#endif

#ifdef SUPPORT_OCP
	reg |= SD_OC_INT_EN;
#endif
	if (!chip->adma_mode) {
		reg |= DATA_DONE_INT_EN;
	}

	rtsx_writel(chip, RTSX_BIER, reg);
	
	RTSX_DEBUGP(("RTSX_BIER: 0x%08x\n", reg));
}

void rtsx_disable_bus_int(struct rtsx_chip *chip)
{
	rtsx_writel(chip, RTSX_BIER, 0);
}

static int rtsx_init_pull_ctl(struct rtsx_chip *chip)
{
	int retval;

	rtsx_init_cmd(chip);

	rtsx_add_cmd(chip, WRITE_REG_CMD, CARD_PULL_CTL2, 0xFF, 0x55);
	if (CHECK_IC_VER(chip, IC_VER_C))
		rtsx_add_cmd(chip, WRITE_REG_CMD, CARD_PULL_CTL3, 0xFF, 0xE5);
	else
		rtsx_add_cmd(chip, WRITE_REG_CMD, CARD_PULL_CTL3, 0xFF, 0xD5);
	rtsx_add_cmd(chip, WRITE_REG_CMD, CARD_PULL_CTL5, 0xFF, 0x55);
	rtsx_add_cmd(chip, WRITE_REG_CMD, CARD_PULL_CTL6, 0xFF, 0x15);
	
	retval = rtsx_send_cmd(chip, 0, 100);
	if (retval < 0)
		TRACE_RET(chip, STATUS_FAIL);
	
	return STATUS_SUCCESS;
}

int rtsx_reset_chip(struct rtsx_chip *chip)
{
	int retval;

	rtsx_writel(chip, RTSX_HCBAR, chip->host_cmds_addr);

	rtsx_disable_aspm(chip);
	
	if (chip->asic_code) {
		u16 val;
 		
 		retval = rtsx_write_phy_register(chip, 0x00, chip->phy_pcr);
 		if (retval != STATUS_SUCCESS) {
 			TRACE_RET(chip, STATUS_FAIL);
 		}

 		retval = rtsx_write_phy_register(chip, 0x03, chip->phy_rcr2);
 		if (retval != STATUS_SUCCESS) {
 			TRACE_RET(chip, STATUS_FAIL);
 		}
		retval = rtsx_write_phy_register(chip, 0x19, 0xFE6C);
		if (retval != STATUS_SUCCESS) {
			TRACE_RET(chip, STATUS_FAIL);
		}
		wait_timeout(1);
		retval = rtsx_write_phy_register(chip, 0x0A, 0x05C0);
		if (retval != STATUS_SUCCESS) {
			TRACE_RET(chip, STATUS_FAIL);
		}
		retval = rtsx_read_phy_register(chip, 0x08, &val);
		if (retval != STATUS_SUCCESS) {
			TRACE_RET(chip, STATUS_FAIL);
		}
		RTSX_DEBUGP(("Read from phy 0x08: 0x%04x\n", val));

		if (chip->phy_voltage <= 0x3F) {
			chip->phy_voltage &= 0x3F;
			RTSX_DEBUGP(("chip->phy_voltage = 0x%x\n", chip->phy_voltage));
			val &= ~0x3F;
			val |= chip->phy_voltage;
			RTSX_DEBUGP(("Write to phy 0x08: 0x%04x\n", val));
			retval = rtsx_write_phy_register(chip, 0x08, val);
			if (retval != STATUS_SUCCESS) {
				TRACE_RET(chip, STATUS_FAIL);
			}
		} else {
			chip->phy_voltage = (u8)(val & 0x3F);
			RTSX_DEBUGP(("Default, chip->phy_voltage = 0x%x\n", chip->phy_voltage));
		}
	}
	
	RTSX_WRITE_REG(chip, HOST_SLEEP_STATE, 0x03, 0x00);

	RTSX_WRITE_REG(chip, CARD_CLK_EN, 0x1E, 0);

	RTSX_WRITE_REG(chip, ASPM_FORCE_CTL, 0x13, 0);

#ifdef SUPPORT_OCP
	RTSX_WRITE_REG(chip, FPDCTL, OC_POWER_DOWN, 0);
	RTSX_WRITE_REG(chip, OCPPARA1, SD_OCP_TIME_MASK, SD_OCP_TIME_800);
	RTSX_WRITE_REG(chip, OCPPARA2, SD_OCP_THD_MASK, chip->sd_400mA_ocp_thd);
	RTSX_WRITE_REG(chip, OCPGLITCH, SD_OCP_GLITCH_MASK, SD_OCP_GLITCH_1000);
	RTSX_WRITE_REG(chip, OCPCTL, 0xFF, SD_OCP_INT_EN | SD_DETECT_EN);
#else
	RTSX_WRITE_REG(chip, FPDCTL, OC_POWER_DOWN, OC_POWER_DOWN);
#endif

	RTSX_WRITE_REG(chip, GPIO_CTL, 0x02, 0x02);
#ifdef LED_AUTO_BLINK
	RTSX_WRITE_REG(chip, OLT_LED_CTL, LED_SHINE_EN | LED_SPEED_MASK, 0x02);
#endif
	
	RTSX_WRITE_REG(chip, CHANGE_LINK_STATE, 0x0A, 0);
	
	RTSX_WRITE_REG(chip, CARD_DRIVE_SEL, 0xFF, chip->card_drive_sel);
	RTSX_WRITE_REG(chip, SD30_DRIVE_SEL, 0x07, chip->sd30_drive_sel_3v3);

	if (chip->asic_code) {
		RTSX_WRITE_REG(chip, SSC_CTL1, 0xFF, SSC_8X_EN | SSC_SEL_4M);
		RTSX_WRITE_REG(chip, SSC_CTL2, 0xFF, 0x12);
	}

	RTSX_WRITE_REG(chip, CHANGE_LINK_STATE, 0x16, 0x10);

	if (chip->aspm_l0s_l1_en) {
		if (!chip->dynamic_aspm) {
			retval = rtsx_write_config_byte(chip, LCTLR, chip->aspm_l0s_l1_en);
			if (retval != STATUS_SUCCESS) {
				TRACE_RET(chip, STATUS_FAIL);
			}
			if (chip->config_host_aspm) {
				rtsx_enable_host_aspm(chip);
			}
			chip->aspm_level[0] = chip->aspm_l0s_l1_en;
			chip->aspm_enabled = 1;
		}
		rtsx_get_host_aspm(chip, &chip->host_aspm_val);
		chip->host_aspm_val &= 0xCF;
		RTSX_DEBUGP(("chip->host_aspm_val = 0x%x\n", chip->host_aspm_val));
	} else {
		retval = rtsx_write_config_byte(chip, LCTLR, chip->aspm_l0s_l1_en);
		if (retval != STATUS_SUCCESS) {
			TRACE_RET(chip, STATUS_FAIL);
		}
	}	
	
	retval = rtsx_write_config_byte(chip, 0x81, 1);
	if (retval != STATUS_SUCCESS) {
		TRACE_RET(chip, STATUS_FAIL);
	}
	
	retval = rtsx_write_cfg_dw(chip, 0, 0x70C, 0xFF000000, 0x5B);
	if (retval != STATUS_SUCCESS) {
		TRACE_RET(chip, STATUS_FAIL);
	}
	
	RTSX_WRITE_REG(chip, IRQSTAT0, LINK_RDY_INT, LINK_RDY_INT);

	RTSX_WRITE_REG(chip, PERST_GLITCH_WIDTH, 0xFF, 0x80);
	
	RTSX_WRITE_REG(chip, PWD_SUSPEND_EN, 0xFF, 0xFF);
	RTSX_WRITE_REG(chip, PWR_GATE_CTRL, PWR_GATE_EN, PWR_GATE_EN);

	rtsx_enable_bus_int(chip);
	
#ifdef HW_INT_WRITE_CLR
	RTSX_WRITE_REG(chip, NFTS_TX_CTRL, 0x02, 0);
#endif
	
	chip->need_reset = 0;

	chip->int_reg = rtsx_readl(chip, RTSX_BIPR);
#ifdef HW_INT_WRITE_CLR
	rtsx_writel(chip, RTSX_BIPR, chip->int_reg);
#endif
	
	if (chip->int_reg & SD_EXIST) {
		chip->need_reset |= SD_CARD;
	}
	if (chip->int_reg & MS_EXIST) {
		chip->need_reset |= MS_CARD;
	}
	if (chip->int_reg & CARD_EXIST) {
		RTSX_WRITE_REG(chip, SSC_CTL1, SSC_RSTB, SSC_RSTB);
	}

	RTSX_DEBUGP(("In rtsx_init_chip, chip->need_reset = 0x%x\n", (unsigned int)(chip->need_reset)));

	RTSX_WRITE_REG(chip, RCCTL, 0x01, 0x00);



	if (chip->remote_wakeup_en && !CHK_AUTODELINK_EN(chip)) {
		RTSX_WRITE_REG(chip, WAKE_SEL_CTL, 0x07, 0x07);
		if (chip->aux_pwr_exist) {
			RTSX_WRITE_REG(chip, PME_FORCE_CTL, 0xFF, 0x33);
		}
	} else {
		RTSX_WRITE_REG(chip, WAKE_SEL_CTL, 0x07, 0x04);
		RTSX_WRITE_REG(chip, PME_FORCE_CTL, 0xFF, 0x30);
	}

	if (chip->force_clkreq_0) {
		RTSX_WRITE_REG(chip, PETXCFG, 0x08, 0x08);
		RTSX_WRITE_REG(chip, 0xFF03, 0x80, 0x80);
	} else {
		RTSX_WRITE_REG(chip, PETXCFG, 0x08, 0x00);
	}
	
	if (CHECK_PID(chip, 0x5227)) {
		if (chip->ltr_en) {
			u16 val;

			if (rtsx_read_config_word(chip, 0x98, &val) < 0)
				TRACE_RET(chip, STATUS_FAIL);
			RTSX_DEBUGP(("Config byte 0x98: 0x%04x\n", val));
			if (val & 0x400) {
				chip->ltr_enabled = 1;
				RTSX_WRITE_REG(chip, LTR_CTL, 0xFF, 0xA3);
			} else {
				chip->ltr_enabled = 0;
			}
		}
	}

	if (chip->ft2_fast_mode) {
		RTSX_WRITE_REG(chip, CARD_PWR_CTL, 0xFF, MS_PARTIAL_POWER_ON | SD_PARTIAL_POWER_ON);
		udelay(chip->pmos_pwr_on_interval);
		RTSX_WRITE_REG(chip, CARD_PWR_CTL, 0xFF, MS_POWER_ON | SD_POWER_ON);
		
		wait_timeout(200);
	}
	
	if (chip->asic_code) {
		RTSX_WRITE_REG(chip, 0xFE78, 0x03, 0x00);
		RTSX_WRITE_REG(chip, 0xFE78, 0x03, 0x01);
	} else {
		RTSX_WRITE_REG(chip, 0xFE78, 0x03, 0x01);
	}

	retval = rtsx_init_pull_ctl(chip);
	if (retval != STATUS_SUCCESS)
		TRACE_RET(chip, STATUS_FAIL);

	rtsx_reset_detected_cards(chip, 0);

	chip->driver_first_load = 0;
	
	return STATUS_SUCCESS;
}

static inline int check_sd_speed_prior(u32 sd_speed_prior)
{
	int i, fake_para = 0;
	
	for (i = 0; i < 4; i++) {
		u8 tmp = (u8)(sd_speed_prior >> (i*8));
		if ((tmp < 0x01) || (tmp > 0x04)) {
			fake_para = 1;
			break;
		}
	}
	
	return !fake_para;
}

static inline int check_sd_current_prior(u32 sd_current_prior)
{
	int i, fake_para = 0;
	
	for (i = 0; i < 4; i++) {
		u8 tmp = (u8)(sd_current_prior >> (i*8));
		if (tmp > 0x03) {
			fake_para = 1;
			break;
		}
	}
	
	return !fake_para;
}

static inline int rts5229_set_cfg_epcore(struct rtsx_chip *chip,
		u16 autoload_addr, u16 cfg_addr, u8 func, u8 mode, u8 data)
{
	RTSX_WRITE_REG(chip, autoload_addr, 0xFF, data);
	RTSX_WRITE_REG(chip, autoload_addr + 1, 0xFF,
			(mode & 0x0F) | ((func & 0x03) << 4) | (((u8)cfg_addr & 0x03) << 6));
	RTSX_WRITE_REG(chip, autoload_addr + 2, 0xFF, (u8)(cfg_addr >> 2));
	RTSX_WRITE_REG(chip, autoload_addr + 3, 0xFF, ((u8)(cfg_addr >> 10) & 0x3F) | 0x80);

	return STATUS_SUCCESS;
}

static inline int rts5229_set_cfg_phy(struct rtsx_chip *chip,
		u16 autoload_addr, u8 phy_addr, u16 data)
{
	RTSX_WRITE_REG(chip, autoload_addr, 0xFF, (u8)data);
	RTSX_WRITE_REG(chip, autoload_addr + 1, 0xFF, (u8)(data >> 8));
	RTSX_WRITE_REG(chip, autoload_addr + 2, 0xFF, phy_addr);
	RTSX_WRITE_REG(chip, autoload_addr + 3, 0xC0,  0x40);

	return STATUS_SUCCESS;
}

static inline int rts5229_auto_load(struct rtsx_chip *chip, u32 relink_time,
		struct rts5229_auto_load_map *map_item, u8 cnt)
{
	int retval;
	u16 autoload_addr;
	u8 i;

	RTSX_WRITE_REG(chip, 0xFF00, 0xFF, (cnt & 0x7F) | 0x80);
	RTSX_WRITE_REG(chip, 0xFF01, 0xFF, (u8)relink_time);
	RTSX_WRITE_REG(chip, 0xFF02, 0xFF, (u8)(relink_time >> 8));
	RTSX_WRITE_REG(chip, 0xFF03, 0xFF, (u8)(relink_time >> 16) & 0x3F);

	RTSX_WRITE_REG(chip, 0xFF04, 0xFF, (u8)chip->ssvid);
	RTSX_WRITE_REG(chip, 0xFF05, 0xFF, (u8)(chip->ssvid >> 8));
	RTSX_WRITE_REG(chip, 0xFF06, 0xFF, (u8)chip->ssdid);
	RTSX_WRITE_REG(chip, 0xFF07, 0xFF, (u8)(chip->ssdid >> 8));

	autoload_addr = 0xFF08;
	for (i = 0; i < cnt; i++) {
		if (map_item[i].type == CFG_EPCORE) {
			RTSX_DEBUGP(("CFG_EPCORE: cfg_addr = 0x%04x, "
					"func = %d, mode = 0x%x, data = 0x%02x\n",
					map_item[i].item.cfg_epcore.cfg_addr,
					map_item[i].item.cfg_epcore.func,
					map_item[i].item.cfg_epcore.mode,
					map_item[i].item.cfg_epcore.data));
			
			retval = rts5229_set_cfg_epcore(chip, autoload_addr, 
					map_item[i].item.cfg_epcore.cfg_addr,
					map_item[i].item.cfg_epcore.func,
					map_item[i].item.cfg_epcore.mode,
					map_item[i].item.cfg_epcore.data);
			if (retval != STATUS_SUCCESS)
				TRACE_RET(chip, STATUS_FAIL);

			autoload_addr += 4;
		} else if (map_item[i].type == CFG_PHY) {
			RTSX_DEBUGP(("CFG_PHY: phy_addr = 0x%02x, data = 0x%04x\n",
					map_item[i].item.cfg_phy.phy_addr,
					map_item[i].item.cfg_phy.data));

			retval = rts5229_set_cfg_phy(chip, autoload_addr,
					map_item[i].item.cfg_phy.phy_addr,
					map_item[i].item.cfg_phy.data);
			if (retval != STATUS_SUCCESS)
				TRACE_RET(chip, STATUS_FAIL);

			autoload_addr += 4;
		}
	}

	return STATUS_SUCCESS;
}

static inline void rts5229_init_cfg_epcore(struct rtsx_chip *chip,
		u16 cfg_addr, u8 func, u8 data)
{
	int i;

	if (chip->al_map_cnt >= AL_MAP_MAX_CNT)
		return;
	i = chip->al_map_cnt++;

	chip->al_map[i].type = CFG_EPCORE;
	chip->al_map[i].item.cfg_epcore.cfg_addr = cfg_addr & 0xFFFC;
	chip->al_map[i].item.cfg_epcore.func = func;
	chip->al_map[i].item.cfg_epcore.mode = (u8)0x01 << (cfg_addr & 0x03);
	chip->al_map[i].item.cfg_epcore.data = data;
}

static inline void rts5229_init_cfg_phy(struct rtsx_chip *chip,	u8 phy_addr, u16 data)
{
	int i;

	if (chip->al_map_cnt >= AL_MAP_MAX_CNT)
		return;
	i = chip->al_map_cnt++;

	chip->al_map[i].type = CFG_PHY;
	chip->al_map[i].item.cfg_phy.phy_addr = phy_addr;
	chip->al_map[i].item.cfg_phy.data = data;
}

static int rtsx_init_from_hw(struct rtsx_chip *chip)
{
	int retval;
	u32 lval;
	u8 val;

	val = rtsx_readb(chip, 0x1C);
	if ((val & 0x10) == 0) {
		chip->asic_code = 1;
	} else {
		chip->asic_code = 0;
	}

	retval = rtsx_read_register(chip, 0xFE90, &val);
	if (retval != STATUS_SUCCESS)	
		TRACE_RET(chip, STATUS_FAIL);
	RTSX_DEBUGP(("0xFE90: 0x%x\n", val));
	chip->ic_version = val & 0x0F;

	if (!chip->use_hw_setting)
		return STATUS_SUCCESS;

	retval = rtsx_read_cfg_dw(chip, 0, 0x724, &lval);
	if (retval != STATUS_SUCCESS)
		TRACE_RET(chip, STATUS_FAIL);
	RTSX_DEBUGP(("dw in 0x724: 0x%x\n", lval));

	val = (u8)(lval >> 24);
	if ((val & 0x01) == 0) {
		u8 lun_mode[4] = {
			0xFF,
			MS_LUN,
			SD_LUN,
			DEFAULT_SINGLE,
		};
		u8 sd_drive[4] = {
			0x01,	
			0x02,	
			0x05,	
			0x03	
		};
		u8 ssc_depth1[4] = {
			SSC_DEPTH_512K,
			SSC_DEPTH_1M,
			SSC_DEPTH_2M,
			SSC_DEPTH_4M,
		};
		u8 ssc_depth2[4] = {
			SSC_DEPTH_256K,
			SSC_DEPTH_512K,
			SSC_DEPTH_1M,
			SSC_DEPTH_2M,
		};

		rts5229_init_cfg_epcore(chip, 0x727, 0, val);

		chip->lun_mode = lun_mode[(val >> 6) & 0x03];	
		chip->aspm_l0s_l1_en = (val >> 4) & 0x03;
		chip->sd30_drive_sel_1v8 = sd_drive[(val >> 2) & 0x03];
		chip->card_drive_sel &= 0x3F;
		chip->card_drive_sel |= ((val >> 1) & 0x01) << 6;

		val = (u8)(lval >> 16);
		rts5229_init_cfg_epcore(chip, 0x726, 0, val);

		if ((val & 0xC0) != 0xC0) {
			chip->asic_sd_hs_clk = (49 - ((val >> 6) & 0x03) * 2) * 2;
			chip->asic_mmc_52m_clk = chip->asic_sd_hs_clk;
		}
		chip->sdr50_en = (val >> 5) & 0x01;
		chip->ddr50_en = (val >> 4) & 0x01;
		chip->sdr104_en = (val >> 3) & 0x01;
		if ((val & 0x07) != 0x07)
			chip->asic_ms_hg_clk = (59 - (val & 0x07)) * 2;

		val = (u8)(lval >> 8);
		rts5229_init_cfg_epcore(chip, 0x725, 0, val);

		if ((val & 0xE0) != 0xE0)	
			chip->asic_sd_sdr104_clk = 206 - ((val >> 5) & 0x07) * 3;
		if ((val & 0x1C) != 0x1C)
			chip->asic_sd_sdr50_clk = 98 - ((val >> 2) & 0x07) * 2;
		if ((val & 0x03) != 0x03)
			chip->asic_sd_ddr50_clk = (48 - (val & 0x03) * 2) * 2;

		val = (u8)lval;
		rts5229_init_cfg_epcore(chip, 0x724, 0, val);

		chip->ssc_depth_sd_sdr104 = ssc_depth1[(val >> 6) & 0x03];
		chip->ssc_depth_sd_sdr50 = chip->ssc_depth_sd_sdr104;
		chip->ssc_depth_sd_ddr50 = ssc_depth1[(val >> 4) & 0x03];
		chip->ssc_depth_sd_hs = ssc_depth2[(val >> 2) & 0x03];
		chip->ssc_depth_mmc_52m = chip->ssc_depth_sd_hs;
		chip->ssc_depth_ms_hg = ssc_depth2[val & 0x03];

		retval = rtsx_read_cfg_dw(chip, 0, 0x814, &lval);
		if (retval != STATUS_SUCCESS)
			TRACE_RET(chip, STATUS_FAIL);
		RTSX_DEBUGP(("dw in 0x814: 0x%x\n", lval));

		val = (u8)lval;
		rts5229_init_cfg_epcore(chip, 0x814, 0, val);
		
		if (chip->auto_delink_en != 2)
			chip->auto_delink_en = (val & 0x80) ? 1 : 0;
		chip->sd30_drive_sel_3v3 = sd_drive[(val >> 5) & 0x03];
	}

	if (chip->hp_watch_bios_hotplug && CHK_AUTODELINK_EN(chip)) {
		u8 reg58, reg5b;
		
		if (rtsx_read_pci_cfg_byte(0x00, 0x1C, 0x02, 0x58, &reg58) < 0) {
			return STATUS_SUCCESS;
		}
		if (rtsx_read_pci_cfg_byte(0x00, 0x1C, 0x02, 0x5B, &reg5b) < 0) {
			return STATUS_SUCCESS;
		}
		
		RTSX_DEBUGP(("reg58 = 0x%x, reg5b = 0x%x\n", reg58, reg5b));
		
		if ((reg58 == 0x00) && (reg5b == 0x01)) {
			chip->auto_delink_en = 0;
		}
	}
	
	return STATUS_SUCCESS;
}

int rtsx_init_chip(struct rtsx_chip *chip)
{
	struct sd_info *sd_card = &(chip->sd_card);
	struct ms_info *ms_card = &(chip->ms_card);
	int retval;
	unsigned int i;
	
	RTSX_DEBUGP(("VID: 0x%04x, PID: 0x%04x, SSVID: 0x%04x, SSDID: 0x%04x\n", 
		     chip->vendor_id, chip->product_id, chip->ssvid, chip->ssdid));
		     
	chip->ic_version = 0;
	
#ifdef _MSG_TRACE
	chip->msg_idx = 0;
#endif

	memset(sd_card, 0, sizeof(struct sd_info));
	memset(ms_card, 0, sizeof(struct ms_info));
	
	chip->sd_reset_counter = 0;
	chip->ms_reset_counter = 0;
	
	chip->sd_show_cnt = MAX_SHOW_CNT;
	chip->ms_show_cnt = MAX_SHOW_CNT;
	
	chip->auto_delink_cnt = 0;
	chip->auto_delink_allowed = 1;
	rtsx_set_stat(chip, RTSX_STAT_INIT);

	chip->ltr_enabled = 0;
	chip->aspm_enabled = 0;
	chip->cur_card = 0;
	chip->phy_debug_mode = 0;

	chip->al_map_cnt = 0;
	
	for (i = 0; i < MAX_ALLOWED_LUN_CNT; i++) {
		set_sense_type(chip, i, SENSE_TYPE_NO_SENSE);
		chip->rw_fail_cnt[i] = 0;
	}
		     
	if (!check_sd_speed_prior(chip->sd_speed_prior)) {
		chip->sd_speed_prior = 0x01040203;
	}
	RTSX_DEBUGP(("sd_speed_prior = 0x%08x\n", chip->sd_speed_prior));
	
	if (!check_sd_current_prior(chip->sd_current_prior)) {
		chip->sd_current_prior = 0x00010203;
	}
	RTSX_DEBUGP(("sd_current_prior = 0x%08x\n", chip->sd_current_prior));

	if ((chip->sd_ddr_tx_phase > 31) || (chip->sd_ddr_tx_phase < 0)) {
		chip->sd_ddr_tx_phase = 0;
	}
	if ((chip->mmc_ddr_tx_phase > 31) || (chip->mmc_ddr_tx_phase < 0)) {
		chip->mmc_ddr_tx_phase = 0;
	}
	
	RTSX_WRITE_REG(chip, FPDCTL, SSC_POWER_DOWN, 0);
	udelay(200);

	RTSX_WRITE_REG(chip, CLK_DIV, 0x07, 0x07);
	
	retval = rtsx_init_from_hw(chip);
	if (retval != STATUS_SUCCESS) {
		TRACE_RET(chip, STATUS_FAIL);
	}
	
	chip->ss_en = 0;
	
	RTSX_DEBUGP(("chip->asic_code = %d\n", chip->asic_code));
	RTSX_DEBUGP(("chip->use_hw_setting = %d\n", chip->use_hw_setting));
	RTSX_DEBUGP(("chip->ic_version = 0x%x\n", chip->ic_version));
	RTSX_DEBUGP(("chip->aux_pwr_exist = %d\n", chip->aux_pwr_exist));
	RTSX_DEBUGP(("chip->aspm_l0s_l1_en = %d\n", chip->aspm_l0s_l1_en));
	RTSX_DEBUGP(("chip->dynamic_aspm = %d\n", chip->dynamic_aspm));
	RTSX_DEBUGP(("chip->auto_delink_en = %d\n", chip->auto_delink_en));
	RTSX_DEBUGP(("chip->ss_en = %d\n", chip->ss_en));
	RTSX_DEBUGP(("chip->lun_mode = %d\n", chip->lun_mode));
	RTSX_DEBUGP(("chip->card_drive_sel = 0x%x\n", chip->card_drive_sel));
	RTSX_DEBUGP(("chip->sd30_drive_sel_1v8 = 0x%x\n", chip->sd30_drive_sel_1v8));
	RTSX_DEBUGP(("chip->sd30_drive_sel_3v3 = 0x%x\n", chip->sd30_drive_sel_3v3));
	RTSX_DEBUGP(("chip->sdr50_en = %d\n", chip->sdr50_en));
	RTSX_DEBUGP(("chip->ddr50_en = %d\n", chip->ddr50_en));
	RTSX_DEBUGP(("chip->sdr104_en = %d\n", chip->sdr104_en));
	RTSX_DEBUGP(("chip->asic_sd_hs_clk = %d\n", chip->asic_sd_hs_clk));
	RTSX_DEBUGP(("chip->asic_mmc_52m_clk = %d\n", chip->asic_mmc_52m_clk));
	RTSX_DEBUGP(("chip->asic_ms_hg_clk = %d\n", chip->asic_ms_hg_clk));
	RTSX_DEBUGP(("chip->asic_sd_sdr104_clk = %d\n", chip->asic_sd_sdr104_clk));
	RTSX_DEBUGP(("chip->asic_sd_sdr50_clk = %d\n", chip->asic_sd_sdr50_clk));
	RTSX_DEBUGP(("chip->asic_sd_ddr50_clk = %d\n", chip->asic_sd_ddr50_clk));
	RTSX_DEBUGP(("chip->ssc_depth_sd_sdr104 = %d\n", chip->ssc_depth_sd_sdr104));
	RTSX_DEBUGP(("chip->ssc_depth_sd_sdr50 = %d\n", chip->ssc_depth_sd_sdr50));
	RTSX_DEBUGP(("chip->ssc_depth_sd_ddr50 = %d\n", chip->ssc_depth_sd_ddr50));
	RTSX_DEBUGP(("chip->ssc_depth_sd_hs = %d\n", chip->ssc_depth_sd_hs));
	RTSX_DEBUGP(("chip->ssc_depth_mmc_52m = %d\n", chip->ssc_depth_mmc_52m));
	RTSX_DEBUGP(("chip->ssc_depth_ms_hg = %d\n", chip->ssc_depth_ms_hg));

	chip->card2lun[SD_CARD] = 0;
	chip->card2lun[MS_CARD] = 0;

	if (CHECK_LUN_MODE(chip, DEFAULT_SINGLE))
		chip->lun2card[0] = SD_CARD | MS_CARD;
	else if (CHECK_LUN_MODE(chip, SD_LUN))
		chip->lun2card[0] = SD_CARD;
	else if (CHECK_LUN_MODE(chip, MS_LUN))
		chip->lun2card[0] = MS_CARD;
	else
		TRACE_RET(chip, STATUS_FAIL);

	chip->max_lun = 0;

	if (chip->asic_code) {
		rts5229_init_cfg_phy(chip, 0x00, chip->phy_pcr);
		rts5229_init_cfg_phy(chip, 0x03, chip->phy_rcr2);
		rts5229_init_cfg_phy(chip, 0x19, 0xFE6C);
		rts5229_init_cfg_phy(chip, 0x0A, 0x05C0);
	}

	retval = rts5229_auto_load(chip, chip->relink_time,
			chip->al_map, (u8)chip->al_map_cnt);
	if (retval != STATUS_SUCCESS)
		TRACE_RET(chip, STATUS_FAIL);

	retval = rtsx_reset_chip(chip);
	if (retval != STATUS_SUCCESS) {
		TRACE_RET(chip, STATUS_FAIL);
	}
	
	return STATUS_SUCCESS;
}

void rtsx_release_chip(struct rtsx_chip *chip)
{
	ms_free_l2p_tbl(chip);
	chip->card_exist = 0;
	chip->card_ready = 0;
}

#ifndef LED_AUTO_BLINK
static inline void rtsx_blink_led(struct rtsx_chip *chip)
{
	if (chip->card_exist && chip->blink_led) {
		if (chip->led_toggle_counter < LED_TOGGLE_INTERVAL) {
			chip->led_toggle_counter ++;
		} else {
			chip->led_toggle_counter = 0;
			toggle_led(chip);
		}
	}
}
#endif

void rtsx_polling_func(struct rtsx_chip *chip)
{
#ifdef SUPPORT_SD_LOCK
	struct sd_info *sd_card = &(chip->sd_card);
#endif
	int ss_allowed;
	
	if (rtsx_chk_stat(chip, RTSX_STAT_SUSPEND)) {
		return;
	}
	
	if (rtsx_chk_stat(chip, RTSX_STAT_DELINK)) {
		goto Delink_Stage;
	}
	
	if (chip->polling_config) {
		u8 val;
		rtsx_read_config_byte(chip, 0, &val);
	}

	if (rtsx_chk_stat(chip, RTSX_STAT_SS)) {
		return;
	}
	
#ifdef SUPPORT_OCP
	if (chip->ocp_int) {
		rtsx_read_register(chip, OCPSTAT, &(chip->ocp_stat));
		
		if (chip->card_exist & SD_CARD) {
			sd_power_off_card3v3(chip);
		} else if (chip->card_exist & MS_CARD) {
			ms_power_off_card3v3(chip);
		}
		
		chip->ocp_int = 0;
	}
#endif
		
#ifdef SUPPORT_SD_LOCK
	if (sd_card->sd_erase_status) {
		if (chip->card_exist & SD_CARD) {
			u8 val;
			rtsx_read_register(chip, SD_BUS_STAT, &val);
			if (val & SD_DAT0_STATUS) {
				sd_card->sd_erase_status = SD_NOT_ERASE;
				sd_card->sd_lock_notify = 1;
				chip->need_reinit |= SD_CARD;
			}
		} else {
			sd_card->sd_erase_status = SD_NOT_ERASE;
		}
	}
#endif

	rtsx_init_cards(chip);
	
	if (chip->ss_en) {
		ss_allowed = 1;
	} else {
		ss_allowed = 0;
	}
	
	if (ss_allowed) {
		if (rtsx_get_stat(chip) != RTSX_STAT_IDLE) {
			chip->ss_counter = 0;
		} else {
			if (chip->ss_counter < (chip->ss_idle_period / POLLING_INTERVAL)) {
				chip->ss_counter ++;
			} else {
				rtsx_exclusive_enter_ss(chip);
				return;
			}
		}
	}
	
	if (chip->idle_counter < IDLE_MAX_COUNT) {
		chip->idle_counter ++;
	} else {
		if (rtsx_get_stat(chip) != RTSX_STAT_IDLE) {
			RTSX_DEBUGP(("Idle state!\n"));
			rtsx_set_stat(chip, RTSX_STAT_IDLE);

#ifdef LED_AUTO_BLINK
			disable_auto_blink(chip);
#else
			chip->led_toggle_counter = 0;
#endif
			rtsx_force_power_on(chip, SSC_PDCTL);
			
			if (chip->led_always_on && chip->card_ready && chip->blink_led) {
				turn_on_led(chip);
			} else {
				turn_off_led(chip);
			}
			
			if (chip->auto_power_down && !chip->card_ready) {
				rtsx_force_power_down(chip, SSC_PDCTL | OC_PDCTL);
			}
		}
	}

	switch (rtsx_get_stat(chip)) {
	case RTSX_STAT_RUN:
#ifndef LED_AUTO_BLINK
		rtsx_blink_led(chip);
#endif 
		do_remaining_work(chip);
		break;

	case RTSX_STAT_IDLE:
		rtsx_enable_aspm(chip);	

		if (chip->ltr_enabled)
			rtsx_write_register(chip, LTR_CTL, 0xFF, 0x83);
		break;

	default:
		break;
	}

	
#ifdef SUPPORT_OCP
	if (chip->ocp_stat & (SD_OC_NOW | SD_OC_EVER)) {
		RTSX_DEBUGP(("Over current, OCPSTAT is 0x%x\n", chip->ocp_stat));
		if (chip->card_exist & SD_CARD) {
			rtsx_write_register(chip, CARD_OE, SD_OUTPUT_EN, 0);
			chip->card_fail |= SD_CARD;
		} else if (chip->card_exist & MS_CARD) {
			rtsx_write_register(chip, CARD_OE, MS_OUTPUT_EN, 0);
			chip->card_fail |= MS_CARD;
		}
		card_power_off(chip, SD_CARD);
	}
#endif

Delink_Stage:
	if (CHK_AUTODELINK_EN(chip) && chip->auto_delink_allowed && 
			!chip->card_ready && !chip->card_ejected) {
		int enter_L1 = chip->auto_delink_in_L1 && (chip->aspm_l0s_l1_en || chip->ss_en);
		int delink_stage1_cnt = chip->delink_stage1_step;
		int delink_stage2_cnt = delink_stage1_cnt + chip->delink_stage2_step;
		int delink_stage3_cnt = delink_stage2_cnt + chip->delink_stage3_step;
		
		if (chip->auto_delink_cnt <= delink_stage3_cnt) {
			if (chip->auto_delink_cnt == delink_stage1_cnt) {
				rtsx_set_stat(chip, RTSX_STAT_DELINK);
				
				clear_first_install_mark(chip);
				
				if (chip->card_exist) {
					RTSX_DEBUGP(("False card inserted, do force delink\n"));

					if (enter_L1) {
						rtsx_write_register(chip, HOST_SLEEP_STATE, 0x03, 1);
					}
					rtsx_write_register(chip, CHANGE_LINK_STATE, 0x0A, 0x0A);

					if (enter_L1) {
						rtsx_enter_L1(chip);
					}
					
					chip->auto_delink_cnt = delink_stage3_cnt + 1;
				} else {
					RTSX_DEBUGP(("No card inserted, do delink\n"));
                
					if (enter_L1) {
						rtsx_write_register(chip, HOST_SLEEP_STATE, 0x03, 1);
					}
#ifdef HW_INT_WRITE_CLR
					rtsx_writel(chip, RTSX_BIPR, 0xFFFFFFFF);
					RTSX_DEBUGP(("RTSX_BIPR: 0x%x\n", rtsx_readl(chip, RTSX_BIPR)));
#endif
					rtsx_write_register(chip, CHANGE_LINK_STATE, 0x02, 0x02);
					
					if (enter_L1) {
						rtsx_enter_L1(chip);
					}
				}
			}

			if (chip->auto_delink_cnt == delink_stage2_cnt) {
				RTSX_DEBUGP(("Try to do force delink\n"));
				
				if (enter_L1) {
					rtsx_exit_L1(chip);
				}
				
				rtsx_write_register(chip, CHANGE_LINK_STATE, 0x0A, 0x0A);
			}
			
			if (chip->auto_delink_cnt == delink_stage3_cnt) {
			}
			
			chip->auto_delink_cnt ++;
		}
	} else {
		chip->auto_delink_cnt = 0;
	}
}

void rtsx_undo_delink(struct rtsx_chip *chip)
{
	chip->auto_delink_allowed = 0;
	rtsx_write_register(chip, CHANGE_LINK_STATE, 0x0A, 0x00);
}

/**
 * rtsx_stop_cmd - stop command transfer and DMA transfer
 * @chip: Realtek's card reader chip
 * @card: flash card type
 *
 * Stop command transfer and DMA transfer.
 * This function is called in error handler. 
 */
void rtsx_stop_cmd(struct rtsx_chip *chip, int card)
{
	int i;
	
	for (i = 0; i <= 8; i++) {
		int addr = RTSX_HCBAR + i * 4;
		u32 reg;
		reg = rtsx_readl(chip, addr);
		RTSX_DEBUGP(("BAR (0x%02x): 0x%08x\n", addr, reg));
	}
	rtsx_writel(chip, RTSX_HCBCTLR, STOP_CMD);
	rtsx_writel(chip, RTSX_HDBCTLR, STOP_DMA);
	
	for (i = 0; i < 16; i++) {
		u16 addr = 0xFE20 + (u16)i;
		u8 val;
		rtsx_read_register(chip, addr, &val);
		RTSX_DEBUGP(("0x%04X: 0x%02x\n", addr, val));
	}

	rtsx_write_register(chip, DMACTL, 0x80, 0x80);
	rtsx_write_register(chip, RBCTL, 0x80, 0x80);
}

#define MAX_RW_REG_CNT		1024

int rtsx_write_register(struct rtsx_chip *chip, u16 addr, u8 mask, u8 data)
{
	int i;
	u32 val = 3 << 30;

	val |= (u32)(addr & 0x3FFF) << 16;
	val |= (u32)mask << 8;
	val |= (u32)data;

	rtsx_writel(chip, RTSX_HAIMR, val);

	for (i = 0; i < MAX_RW_REG_CNT; i++) {
		val = rtsx_readl(chip, RTSX_HAIMR);
		if ((val & (1 << 31)) == 0) {
			if (data != (u8)val) {
				TRACE_RET(chip, STATUS_FAIL);
			}
			return STATUS_SUCCESS;
		}
	}

	TRACE_RET(chip, STATUS_TIMEDOUT);
}

int rtsx_read_register(struct rtsx_chip *chip, u16 addr, u8 *data)
{
	u32 val = 2 << 30;
	int i;

	if (data) {
		*data = 0;
	}

	val |= (u32)(addr & 0x3FFF) << 16;

	rtsx_writel(chip, RTSX_HAIMR, val);

	for (i = 0; i < MAX_RW_REG_CNT; i++) {
		val = rtsx_readl(chip, RTSX_HAIMR);
		if ((val & (1 << 31)) == 0) {
			break;
		}
	}

	if (i >= MAX_RW_REG_CNT) {
		TRACE_RET(chip, STATUS_TIMEDOUT);
	}

	if (data) {
		*data = (u8)(val & 0xFF);
	}

	return STATUS_SUCCESS;
}

int rtsx_write_cfg_dw(struct rtsx_chip *chip, u8 func_no, u16 addr, u32 mask, u32 val)
{
	u8 mode = 0, tmp;
	int i;
	

	for (i = 0; i < 4; i++) {
		if (mask & 0xFF) {
			RTSX_WRITE_REG(chip, CFGDATA0 + i, 0xFF, (u8)(val & mask & 0xFF));
			mode |= (1 << i);
		}
		mask >>= 8;
		val >>= 8;
	}

	if (mode) {
		RTSX_WRITE_REG(chip, CFGADDR0, 0xFF, (u8)addr);
		RTSX_WRITE_REG(chip, CFGADDR1, 0xFF, (u8)(addr >> 8));

		RTSX_WRITE_REG(chip, CFGRWCTL, 0xFF, 0x80 | mode | ((func_no & 0x03) << 4));

		for (i = 0; i < MAX_RW_REG_CNT; i++) {
			RTSX_READ_REG(chip, CFGRWCTL, &tmp);
			if ((tmp & 0x80) == 0) {
				break;
			}
		}
	}
	
	return STATUS_SUCCESS;
}

int rtsx_read_cfg_dw(struct rtsx_chip *chip, u8 func_no, u16 addr, u32 *val)
{
	int i;
	u8 tmp;
	u32 data = 0;
	

	RTSX_WRITE_REG(chip, CFGADDR0, 0xFF, (u8)addr);
	RTSX_WRITE_REG(chip, CFGADDR1, 0xFF, (u8)(addr >> 8));
	RTSX_WRITE_REG(chip, CFGRWCTL, 0xFF, 0x80 | ((func_no & 0x03) << 4));

	for (i = 0; i < MAX_RW_REG_CNT; i++) {
		RTSX_READ_REG(chip, CFGRWCTL, &tmp);
		if ((tmp & 0x80) == 0) {
			break;
		}
	}

	for (i = 0; i < 4; i++) {
		RTSX_READ_REG(chip, CFGDATA0 + i, &tmp);
		data |= (u32)tmp << (i * 8);
	}

	if (val) {
		*val = data;
	}
	
	return STATUS_SUCCESS;
}

int rtsx_write_cfg_seq(struct rtsx_chip *chip, u8 func, u16 addr, u8 *buf, int len)
{
	u32 *data, *mask;
	u16 offset = addr % 4;
	u16 aligned_addr = addr - offset;
	int dw_len, i, j;
	int retval;
	
	RTSX_DEBUGP(("%s\n", __FUNCTION__));
	
	if (!buf) {
		TRACE_RET(chip, STATUS_NOMEM);
	}
	
	if ((len + offset) % 4) {
		dw_len = (len + offset) / 4 + 1;
	} else {
		dw_len = (len + offset) / 4;
	}
	RTSX_DEBUGP(("dw_len = %d\n", dw_len));
	
	data = (u32 *)vmalloc(dw_len * 4);
	if (!data) {
		TRACE_RET(chip, STATUS_NOMEM);
	}
	memset(data, 0, dw_len * 4);
	
	mask = (u32 *)vmalloc(dw_len * 4);
	if (!mask) {
		vfree(data);
		TRACE_RET(chip, STATUS_NOMEM);
	}
	memset(mask, 0, dw_len * 4);
	
	j = 0;
	for (i = 0; i < len; i++) {
		mask[j] |= 0xFF << (offset * 8);
		data[j] |= buf[i] << (offset * 8);
		if (++offset == 4) {
			j++;
			offset = 0;
		}
	}
	
	RTSX_DUMP(mask, dw_len * 4);
	RTSX_DUMP(data, dw_len * 4);
	
	for (i = 0; i < dw_len; i++) {
		retval = rtsx_write_cfg_dw(chip, func, aligned_addr + i * 4, mask[i], data[i]);
		if (retval != STATUS_SUCCESS) {
			vfree(data);
			vfree(mask);
			TRACE_RET(chip, STATUS_FAIL);
		}
	}
	
	vfree(data);
	vfree(mask);
	
	return STATUS_SUCCESS;
}

int rtsx_read_cfg_seq(struct rtsx_chip *chip, u8 func, u16 addr, u8 *buf, int len)
{
	u32 *data;
	u16 offset = addr % 4;
	u16 aligned_addr = addr - offset;
	int dw_len, i, j;
	int retval;
	
	RTSX_DEBUGP(("%s\n", __FUNCTION__));
	
	if ((len + offset) % 4) {
		dw_len = (len + offset) / 4 + 1;
	} else {
		dw_len = (len + offset) / 4;
	}
	RTSX_DEBUGP(("dw_len = %d\n", dw_len));
	
	data = (u32 *)vmalloc(dw_len * 4);
	if (!data) {
		TRACE_RET(chip, STATUS_NOMEM);
	}
	
	for (i = 0; i < dw_len; i++) {
		retval = rtsx_read_cfg_dw(chip, func, aligned_addr + i * 4, data + i);
		if (retval != STATUS_SUCCESS) {
			vfree(data);
			TRACE_RET(chip, STATUS_FAIL);
		}
	}
	
	if (buf) {
		j = 0;
		
		for (i = 0; i < len; i++) {
			buf[i] = (u8)(data[j] >> (offset * 8));
			if (++offset == 4) {
				j++;
				offset = 0;
			}
		}
	}
	
	vfree(data);
	
	return STATUS_SUCCESS;
}

int rtsx_write_phy_register(struct rtsx_chip *chip, u8 addr, u16 val)
{
	int i, finished = 0;
	u8 tmp;

	RTSX_WRITE_REG(chip, PHYDATA0, 0xFF, (u8)val);
	RTSX_WRITE_REG(chip, PHYDATA1, 0xFF, (u8)(val >> 8));
	RTSX_WRITE_REG(chip, PHYADDR, 0xFF, addr);

	RTSX_WRITE_REG(chip, PHYRWCTL, 0xFF, 0x81);

	for (i = 0; i < 100000; i++) {
		RTSX_READ_REG(chip, PHYRWCTL, &tmp);
		if (!(tmp & 0x80)) {
			finished = 1;
			break;
		}
	}
	
	if (!finished) {
		TRACE_RET(chip, STATUS_FAIL);
	}
	
	return STATUS_SUCCESS;
}

int rtsx_read_phy_register(struct rtsx_chip *chip, u8 addr, u16 *val)
{
	int i, finished = 0;
	u16 data = 0;
	u8 tmp;

	RTSX_WRITE_REG(chip, PHYADDR, 0xFF, addr);
	RTSX_WRITE_REG(chip, PHYRWCTL, 0xFF, 0x80);

	for (i = 0; i < 100000; i++) {
		RTSX_READ_REG(chip, PHYRWCTL, &tmp);
		if (!(tmp & 0x80)) {
			finished = 1;
			break;
		}
	}
	
	if (!finished) {
		TRACE_RET(chip, STATUS_FAIL);
	}

	RTSX_READ_REG(chip, PHYDATA0, &tmp);
	data = tmp;
	RTSX_READ_REG(chip, PHYDATA1, &tmp);
	data |= (u16)tmp << 8;

	if (val) {
		*val = data;
	}
	
	return STATUS_SUCCESS;
}

int rtsx_clr_phy_reg_bit(struct rtsx_chip *chip, u8 reg, u8 bit)
{
	int retval;
	u16 value;
	
	retval = rtsx_read_phy_register(chip, reg, &value);
	if (retval != STATUS_SUCCESS) {
		TRACE_RET(chip, STATUS_FAIL);
	}
	if (value & (1 << bit)) {
		value &= ~(1 << bit);
		retval = rtsx_write_phy_register(chip, reg, value);
		if (retval != STATUS_SUCCESS) {
			TRACE_RET(chip, STATUS_FAIL);
		}
	}
	
	return STATUS_SUCCESS;
}

int rtsx_set_phy_reg_bit(struct rtsx_chip *chip, u8 reg, u8 bit)
{
	int retval;
	u16 value;
	
	retval = rtsx_read_phy_register(chip, reg, &value);
	if (retval != STATUS_SUCCESS) {
		TRACE_RET(chip, STATUS_FAIL);
	}
	if (0 == (value & (1 << bit))) {
		value |= (1 << bit);
		retval = rtsx_write_phy_register(chip, reg, value);
		if (retval != STATUS_SUCCESS) {
			TRACE_RET(chip, STATUS_FAIL);
		}
	}
	
	return STATUS_SUCCESS;
}

int rtsx_check_link_ready(struct rtsx_chip *chip)
{
	u8 val;

	RTSX_READ_REG(chip, IRQSTAT0, &val);
	
	RTSX_DEBUGP(("IRQSTAT0: 0x%x\n", val));
	if (val & LINK_RDY_INT) {
		RTSX_DEBUGP(("Delinked!\n"));

		rtsx_write_register(chip, IRQSTAT0, LINK_RDY_INT, LINK_RDY_INT);

		return STATUS_FAIL;
	}

	return STATUS_SUCCESS;
}

static void rtsx_handle_pm_dstate(struct rtsx_chip *chip, u8 dstate)
{
	RTSX_DEBUGP(("%04x set pm_dstate to %d\n", chip->product_id, dstate));
	
	rtsx_write_config_byte(chip, 0x44, dstate);
	rtsx_write_config_byte(chip, 0x45, 0);
}

void rtsx_enter_L1(struct rtsx_chip *chip)
{
	rtsx_handle_pm_dstate(chip, 2);
}

void rtsx_exit_L1(struct rtsx_chip *chip)
{
	rtsx_write_config_byte(chip, 0x44, 0);
	rtsx_write_config_byte(chip, 0x45, 0);
}

void rtsx_enter_ss(struct rtsx_chip *chip)
{
	RTSX_DEBUGP(("Enter Selective Suspend State!\n"));
	
	rtsx_write_register(chip, IRQSTAT0, LINK_RDY_INT, LINK_RDY_INT);

	if (chip->power_down_in_ss) {
		rtsx_power_off_card(chip);
	
		rtsx_force_power_down(chip, SSC_PDCTL | OC_PDCTL);
	}

	if (CHK_AUTODELINK_EN(chip)) {
		rtsx_write_register(chip, HOST_SLEEP_STATE, 0x01, 0x01);
	} else {
		if (!chip->phy_debug_mode) {		
			u32 tmp;
			tmp = rtsx_readl(chip,RTSX_BIER) ;
			tmp |= CARD_INT;
			rtsx_writel(chip, RTSX_BIER, tmp);
		}

		rtsx_write_register(chip, CHANGE_LINK_STATE, 0x02, 0);
	}

	rtsx_enter_L1(chip);

	RTSX_CLR_DELINK(chip);
	
	rtsx_set_stat(chip, RTSX_STAT_SS);
}

void rtsx_exit_ss(struct rtsx_chip *chip)
{
	RTSX_DEBUGP(("Exit Selective Suspend State!\n"));

	rtsx_exit_L1(chip);

	if (chip->power_down_in_ss) {
		rtsx_force_power_on(chip, SSC_PDCTL | OC_PDCTL);

		udelay(1000);
	}

	if (RTSX_TST_DELINK(chip)) {
		chip->need_reinit = SD_CARD | MS_CARD;
		rtsx_reinit_cards(chip, 1);
		RTSX_CLR_DELINK(chip);
	} else if (chip->power_down_in_ss) {
		chip->need_reinit = SD_CARD | MS_CARD;
		rtsx_reinit_cards(chip, 0);
	}
}

int rtsx_pre_handle_interrupt(struct rtsx_chip *chip)
{
	u32 status, int_enable;
	int exit_ss = 0;
#ifdef SUPPORT_OCP
	u32 ocp_int = 0;

	ocp_int = SD_OC_INT;
#endif
	
	if (chip->ss_en) {
		chip->ss_counter = 0;
		if (rtsx_get_stat(chip) == RTSX_STAT_SS) {
			exit_ss = 1;
			rtsx_exit_L1(chip);
			
			rtsx_set_stat(chip, RTSX_STAT_RUN);
		}
	}

	int_enable = rtsx_readl(chip, RTSX_BIER);
	
	chip->int_reg = rtsx_readl(chip, RTSX_BIPR);

#ifdef HW_INT_WRITE_CLR
	rtsx_writel(chip, RTSX_BIPR, chip->int_reg);
#endif
	
	if (((chip->int_reg & int_enable) == 0) || (chip->int_reg == 0xFFFFFFFF)) {
		return STATUS_FAIL;
	}
	
	status = chip->int_reg &= (int_enable | 0x7FFFFF);
	
	if (status & CARD_INT) {
		chip->auto_delink_cnt = 0;

		if (status & SD_INT) {
			if (status & SD_EXIST) {
				RTSX_MSG_IN_INT(("Insert SD\n"));
				set_bit(SD_NR, &(chip->need_reset));
			} else {
				RTSX_MSG_IN_INT(("Remove SD\n"));
				set_bit(SD_NR, &(chip->need_release));
				chip->sd_reset_counter = 0;
				chip->sd_show_cnt = 0;
				clear_bit(SD_NR, &(chip->need_reset));
			}
		} else {
			if (exit_ss && (status & SD_EXIST)) {
				set_bit(SD_NR, &(chip->need_reinit));
			}
		}

		if (status & MS_INT) {
			if (status & MS_EXIST) {
				RTSX_MSG_IN_INT(("Insert MS\n"));
				set_bit(MS_NR, &(chip->need_reset));
			} else {
				RTSX_MSG_IN_INT(("Remove MS\n"));
				set_bit(MS_NR, &(chip->need_release));
				chip->ms_reset_counter = 0;
				chip->ms_show_cnt = 0;
				clear_bit(MS_NR, &(chip->need_reset));
			}
		} else {
			if (exit_ss && (status & MS_EXIST)) {
				set_bit(MS_NR, &(chip->need_reinit));
			}
		}
	}
	
#ifdef SUPPORT_OCP
	chip->ocp_int = ocp_int & status;
#endif
	
	return STATUS_SUCCESS;	
}

void rtsx_do_before_power_down(struct rtsx_chip *chip, int pm_stat)
{
	int retval;
	
	RTSX_DEBUGP(("rtsx_do_before_power_down, pm_stat = %d\n", pm_stat));
	
	rtsx_set_stat(chip, RTSX_STAT_SUSPEND);
	
	retval = rtsx_force_power_on(chip, SSC_PDCTL);
	if (retval != STATUS_SUCCESS) {
		return;
	}
	
	rtsx_release_cards(chip);
	rtsx_disable_bus_int(chip);
	if (chip->blink_led)
		turn_off_led(chip);
	
	rtsx_write_register(chip, PETXCFG, 0x08, 0x08);
	
	if (pm_stat == PM_S1) {
		RTSX_DEBUGP(("Host enter S1\n"));
		rtsx_write_register(chip, HOST_SLEEP_STATE, 0x03, HOST_ENTER_S1);
	} else if (pm_stat == PM_S3) {
		if (chip->s3_pwr_off_delay > 0) {
			wait_timeout(chip->s3_pwr_off_delay);
		}
		RTSX_DEBUGP(("Host enter S3\n"));
		rtsx_write_register(chip, HOST_SLEEP_STATE, 0x03, HOST_ENTER_S3);
	}
	
	if (chip->do_delink_before_power_down && CHK_AUTODELINK_EN(chip)) {
		rtsx_write_register(chip, CHANGE_LINK_STATE, 0x02, 2);
	}
	
	
	rtsx_force_power_down(chip, SSC_PDCTL | OC_PDCTL);
	
	chip->cur_clk = 0;
	chip->cur_card = 0;
	
	chip->card_exist = 0;
}

void rtsx_enable_aspm(struct rtsx_chip *chip)
{
	if (chip->aspm_l0s_l1_en && chip->dynamic_aspm) {
		if (!chip->aspm_enabled) {
			RTSX_DEBUGP(("Try to enable ASPM\n"));
			chip->aspm_enabled = 1;
			
			if (chip->asic_code) {
				rtsx_write_phy_register(chip, 0x07, 0);
			}
			rtsx_write_config_byte(chip, LCTLR, chip->aspm_l0s_l1_en);
			
			if (chip->config_host_aspm) {
				rtsx_set_host_aspm(chip, chip->host_aspm_val);
			}
		}
	}

	return;
}

void rtsx_disable_aspm(struct rtsx_chip *chip)
{
	if (chip->aspm_l0s_l1_en && chip->dynamic_aspm) {
		if (chip->aspm_enabled) {
			RTSX_DEBUGP(("Try to disable ASPM\n"));
			chip->aspm_enabled = 0;

			if (chip->config_host_aspm) {
				rtsx_disable_host_aspm(chip);
			}
			
			rtsx_write_config_byte(chip, LCTLR, 0x00);
			wait_timeout(1);
		}
	}

	return;	
}

void rtsx_enter_work_state(struct rtsx_chip *chip)
{
	rtsx_disable_aspm(chip);

	if (chip->ltr_enabled)
		rtsx_write_register(chip, LTR_CTL, 0xFF, 0xA3);
}

int rtsx_read_ppbuf(struct rtsx_chip *chip, u8 *buf, int buf_len)
{
	int retval;
	int i;
	u16 reg_addr;
	u8 *ptr;
	
	if (!buf) {
		TRACE_RET(chip, STATUS_ERROR);
	}
	
	ptr = buf;
	reg_addr = PPBUF_BASE2;
#ifdef USING_PPBUF
	for (i = 0; i < buf_len/256; i++) {
		int j;
		
		rtsx_init_cmd(chip);
		
		for (j = 0; j < 256; j++) {
			rtsx_add_cmd(chip, READ_REG_CMD, reg_addr++, 0, 0);
		}
		
		retval = rtsx_send_cmd(chip, 0, 250);
		if (retval < 0) {
			TRACE_RET(chip, STATUS_FAIL);
		}
		
		memcpy(ptr, rtsx_get_cmd_data(chip), 256);
		ptr += 256;
	}
	
	if (buf_len%256) {
		rtsx_init_cmd(chip);
		
		for (i = 0; i < buf_len%256; i++) {
			rtsx_add_cmd(chip, READ_REG_CMD, reg_addr++, 0, 0);
		}
		
		retval = rtsx_send_cmd(chip, 0, 250);
		if (retval < 0) {
			TRACE_RET(chip, STATUS_FAIL);
		}
	}
#else
	rtsx_init_cmd(chip);
	
	for (i = 0; i < buf_len%256; i++) {
		rtsx_add_cmd(chip, READ_REG_CMD, reg_addr++, 0, 0);
	}
	
	retval = rtsx_send_cmd(chip, 0, 250);
	if (retval < 0) {
		TRACE_RET(chip, STATUS_FAIL);
	}
#endif
	
	memcpy(ptr, rtsx_get_cmd_data(chip), buf_len%256);
	
	return STATUS_SUCCESS;
}

int rtsx_write_ppbuf(struct rtsx_chip *chip, u8 *buf, int buf_len)
{
	int retval;
	int i;
	u16 reg_addr;
	u8 *ptr;
	
	if (!buf) {
		TRACE_RET(chip, STATUS_ERROR);
	}
	
	ptr = buf;
	reg_addr = PPBUF_BASE2;
#ifdef USING_PPBUF
	for (i = 0; i < buf_len/256; i++) {
		int j;
		
		rtsx_init_cmd(chip);
		
		for (j = 0; j < 256; j++) {
			rtsx_add_cmd(chip, WRITE_REG_CMD, reg_addr++, 0xFF, *ptr);
			ptr++;
		}
		
		retval = rtsx_send_cmd(chip, 0, 250);
		if (retval < 0) {
			TRACE_RET(chip, STATUS_FAIL);
		}
	}
	
	if (buf_len%256) {
		rtsx_init_cmd(chip);
		
		for (i = 0; i < buf_len%256; i++) {
			rtsx_add_cmd(chip, WRITE_REG_CMD, reg_addr++, 0xFF, *ptr);
			ptr++;
		}
		
		retval = rtsx_send_cmd(chip, 0, 250);
		if (retval < 0) {
			TRACE_RET(chip, STATUS_FAIL);
		}
	}
#else
	rtsx_init_cmd(chip);
	
	for (i = 0; i < buf_len%256; i++) {
		rtsx_add_cmd(chip, WRITE_REG_CMD, reg_addr++, 0xFF, *ptr);
		ptr++;
	}
	
	retval = rtsx_send_cmd(chip, 0, 250);
	if (retval < 0) {
		TRACE_RET(chip, STATUS_FAIL);
	}
#endif
	
	return STATUS_SUCCESS;
}

int rtsx_check_chip_exist(struct rtsx_chip *chip)
{
	if (rtsx_readl(chip, 0) == 0xFFFFFFFF) {
		TRACE_RET(chip, STATUS_FAIL);
	}
	
	return STATUS_SUCCESS;
}

int rtsx_force_power_on(struct rtsx_chip *chip, u8 ctl)
{
	int retval;
	u8 mask = 0;
	
	if (ctl & SSC_PDCTL) {
		mask |= SSC_POWER_DOWN;
	}
	
#ifdef SUPPORT_OCP
	if (ctl & OC_PDCTL) {
		mask |= SD_OC_POWER_DOWN;
	}
#endif
	
	if (mask) {
		retval = rtsx_write_register(chip, FPDCTL, mask, 0);
		if (retval != STATUS_SUCCESS) {
			TRACE_RET(chip, STATUS_FAIL);
		}
	}
	
	return STATUS_SUCCESS;
}

int rtsx_force_power_down(struct rtsx_chip *chip, u8 ctl)
{
	int retval;
	u8 mask = 0, val = 0;
	
	if (ctl & SSC_PDCTL) {
		mask |= SSC_POWER_DOWN;
	}
	
#ifdef SUPPORT_OCP
	if (ctl & OC_PDCTL) {
		mask |= SD_OC_POWER_DOWN;
	}
#endif
	
	if (mask) {
		val = mask;
		retval = rtsx_write_register(chip, FPDCTL, mask, val);
		if (retval != STATUS_SUCCESS) {
			TRACE_RET(chip, STATUS_FAIL);
		}
	}
	
	return STATUS_SUCCESS;
}

void rtsx_wait_rb_full(struct rtsx_chip *chip)
{
	int i;
	u8 val;
	
	for (i = 0; i < 1000; i++) {
		if (rtsx_read_register(chip, RBCTL, &val)) {
			break;
		}
		if (val & RB_FULL) {
			break;
		}
		
		mdelay(1);
	}
}

void rtsx_set_stat(struct rtsx_chip *chip, enum RTSX_STAT stat)
{
	if (stat != RTSX_STAT_IDLE) {
		chip->idle_counter = 0;
	}
	
	if (chip->rtsx_stat != stat) {
		chip->rtsx_stat = stat;
#ifdef LED_AUTO_BLINK
		if ((stat == RTSX_STAT_RUN) && chip->blink_led) {
			enable_auto_blink(chip);
		}
#endif
	}
}


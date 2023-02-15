/* Driver for Realtek PCI-Express card reader
 * Header file
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

#ifndef __REALTEK_RTSX_SCSI_H
#define __REALTEK_RTSX_SCSI_H

#include "rtsx.h"
#include "rtsx_chip.h"

#define MS_SP_CMND		0xFA
#define MS_FORMAT		0xA0
#define GET_MS_INFORMATION	0xB0

#define VENDOR_CMND		0xF0

#define READ_STATUS		0x09

#define READ_MEM		0x0D
#define WRITE_MEM		0x0E
#define GET_BUS_WIDTH		0x13
#define GET_SD_CSD		0x14
#define TOGGLE_GPIO		0x15
#define TRACE_MSG		0x18

#define SCSI_APP_CMD		0x10

#define PP_READ10		0x1A
#define PP_WRITE10		0x0A
#define READ_HOST_REG		0x1D
#define WRITE_HOST_REG		0x0D
#define SET_VAR			0x05
#define GET_VAR			0x15
#define DMA_READ		0x16
#define DMA_WRITE		0x06
#define GET_DEV_STATUS		0x10
#define GET_CARD_STATUS		0x12
#define SET_CHIP_MODE		0x27
#define SUIT_CMD		0xE0
#define WRITE_PHY		0x07
#define READ_PHY		0x17
#define WRITE_CFG		0x0E
#define READ_CFG		0x1E

#define INIT_BATCHCMD		0x41
#define ADD_BATCHCMD		0x42
#define SEND_BATCHCMD		0x43
#define GET_BATCHRSP		0x44

#define CHIP_NORMALMODE		0x00
#define CHIP_DEBUGMODE		0x01

#define SD_PASS_THRU_MODE	0xD0
#define SD_EXECUTE_NO_DATA	0xD1
#define SD_EXECUTE_READ		0xD2
#define SD_EXECUTE_WRITE	0xD3
#define SD_GET_RSP		0xD4
#define SD_HW_RST		0xD6

#ifdef SUPPORT_MAGIC_GATE
#define CMD_MSPRO_MG_RKEY	0xA4
#define CMD_MSPRO_MG_SKEY	0xA3

#define KC_MG_R_PRO		0xBE

#define KF_SET_LEAF_ID		0x31
#define KF_GET_LOC_EKB		0x32
#define KF_CHG_HOST		0x33
#define KF_RSP_CHG		0x34
#define KF_RSP_HOST		0x35
#define KF_GET_ICV		0x36
#define KF_SET_ICV		0x37
#endif

#define	SENSE_TYPE_NO_SENSE				0
#define	SENSE_TYPE_MEDIA_CHANGE				1
#define	SENSE_TYPE_MEDIA_NOT_PRESENT			2
#define	SENSE_TYPE_MEDIA_LBA_OVER_RANGE			3
#define	SENSE_TYPE_MEDIA_LUN_NOT_SUPPORT		4
#define	SENSE_TYPE_MEDIA_WRITE_PROTECT			5
#define	SENSE_TYPE_MEDIA_INVALID_CMD_FIELD		6
#define	SENSE_TYPE_MEDIA_UNRECOVER_READ_ERR		7
#define	SENSE_TYPE_MEDIA_WRITE_ERR			8
#define SENSE_TYPE_FORMAT_IN_PROGRESS			9
#define SENSE_TYPE_FORMAT_CMD_FAILED			10
#ifdef SUPPORT_MAGIC_GATE
#define SENSE_TYPE_MG_KEY_FAIL_NOT_ESTAB		0x0b
#define SENSE_TYPE_MG_KEY_FAIL_NOT_AUTHEN		0x0c
#define SENSE_TYPE_MG_INCOMPATIBLE_MEDIUM		0x0d
#define SENSE_TYPE_MG_WRITE_ERR				0x0e
#endif
#ifdef SUPPORT_SD_LOCK
#define SENSE_TYPE_MEDIA_READ_FORBIDDEN			0x10
#endif

#define	PIO_MODE_0			0x01
#define	PIO_MODE_1			0x02
#define	PIO_MODE_2			0x04
#define	PIO_MODE_3			0x08
#define	PIO_MODE_4			0x10
#define	PIO_MODE_5			0x20
#define	PIO_MODE_6			0x40
#define	PIO_MULTI_SETCTORS		0x80
#define	UDMA_MODE_0			0x01
#define	UDMA_MODE_1			0x02
#define	UDMA_MODE_2			0x04
#define	UDMA_MODE_3			0x08
#define	UDMA_MODE_4			0x10
#define	UDMA_MODE_5			0x20
#define	UDMA_MODE_6			0x40
#define	UDMA_MODE_7			0x80


#define PP_OC_LUN_STAT_SPRT		0x08
#define PP_WP_LUN_STAT_SPRT		0x20
#define PP_VBUS_INFO_SPRT		0x02
#define PP_CARD_VOLTAGE			0x08
#define pp_CARD_VOLTAGE_1V8		0x08


#define PP_SD_CPRM			0x08
#ifdef SUPPORT_MAGIC_GATE
#define PP_MAGIC_GATE			0x02
#else
#define PP_MAGIC_GATE			0x00
#endif


#define PP_AUTO_DELINK_EN_DEF		0x10


#define PP_VBUS_TOO_LOW			0x02


#define PP_LUN_WRITE_PROTECT		0x20
#define PP_LUN_READ_PROTECT		0x40


#define PP_FLASH_CODE			0x40
#define PP_CODE_MODE_FUNC		0x40
#define PP_FLASH_OP_VER2		0x02
#define	PP_USB_SPEED_FUNCTION_SUPPORT	0x08



#define PP_SD_LOCK_FUNC			0x01
#define PP_SD_LOCK_SUPPORT		0x80
#define PP_SD_ERASING			0x01
#define PP_SD_LOCKED			0x02
#define PP_SD_PWD_EXIST			0x04


#define PP_SD_SC			0x00
#define PP_SD_HC			0x01
#define PP_SD_XC			0x02
#define PP_SD_STD			0x00
#define PP_SD_HIGH			0x01
#define PP_SD_SDR50			0x02
#define PP_SD_SDR104			0x03
#define PP_SD_DDR50			0x04

#define PP_MMC_SC			0x00
#define PP_MMC_HC			0x01
#define PP_MMC_XC			0x02
#define PP_MMC_STD			0x00
#define PP_MMC_HIGH			0x01
#define PP_MMC_DDR50			0x04

void scsi_show_command(struct scsi_cmnd *srb);
void set_sense_type(struct rtsx_chip *chip, unsigned int lun, int sense_type);
void set_sense_data(struct rtsx_chip *chip, unsigned int lun, u8 err_code, u8 sense_key,
		u32 info, u8 asc, u8 ascq, u8 sns_key_info0, u16 sns_key_info1);
int rtsx_scsi_handler(struct scsi_cmnd *srb, struct rtsx_chip *chip);

#endif


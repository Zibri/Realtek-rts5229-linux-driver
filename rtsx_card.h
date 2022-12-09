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

#ifndef __REALTEK_RTSX_CARD_H
#define __REALTEK_RTSX_CARD_H

#include "debug.h"
#include "rtsx.h"
#include "rtsx_chip.h"
#include "rtsx_transport.h"
#include "sd.h"

#define SSC_POWER_DOWN		0x01
#define SD_OC_POWER_DOWN	0x02
#define ALL_POWER_DOWN		0x03
#define OC_POWER_DOWN		0x02

#define PMOS_STRG_MASK		0x10
#define PMOS_STRG_800mA		0x10
#define PMOS_STRG_400mA		0x00

#define POWER_OFF		0x03
#define PARTIAL_POWER_ON	0x01
#define POWER_ON		0x00

#define MS_POWER_OFF		0x0C
#define MS_PARTIAL_POWER_ON	0x04
#define MS_POWER_ON		0x00
#define MS_POWER_MASK		0x0C

#define SD_POWER_OFF		0x03
#define SD_PARTIAL_POWER_ON	0x01
#define SD_POWER_ON		0x00
#define SD_POWER_MASK		0x03

#define SD_OUTPUT_EN		0x04
#define MS_OUTPUT_EN		0x08

#define CLK_LOW_FREQ		0x01

#define CLK_DIV_1		0x01
#define CLK_DIV_2		0x02
#define CLK_DIV_4		0x03
#define CLK_DIV_8		0x04

#define SSC_80			0
#define SSC_100			1
#define SSC_120			2
#define SSC_150			3
#define SSC_200			4

#define SD_CLK_EN		0x04
#define MS_CLK_EN		0x08

#define SD_MOD_SEL		2
#define MS_MOD_SEL		3

#define CHANGE_CLK		0x01


#define	SD_CRC7_ERR			0x80
#define	SD_CRC16_ERR			0x40
#define	SD_CRC_WRITE_ERR		0x20
#define	SD_CRC_WRITE_ERR_MASK	    	0x1C
#define	GET_CRC_TIME_OUT		0x02
#define	SD_TUNING_COMPARE_ERR		0x01

#define	SD_RSP_80CLK_TIMEOUT		0x01

#define	SD_CLK_TOGGLE_EN		0x80
#define	SD_CLK_FORCE_STOP	        0x40
#define	SD_DAT3_STATUS		        0x10
#define	SD_DAT2_STATUS		        0x08
#define	SD_DAT1_STATUS		        0x04
#define	SD_DAT0_STATUS		        0x02
#define	SD_CMD_STATUS			0x01

#define	SD_IO_USING_1V8		        0x80
#define	SD_IO_USING_3V3		        0x7F
#define	TYPE_A_DRIVING		        0x00
#define	TYPE_B_DRIVING			0x01
#define	TYPE_C_DRIVING			0x02
#define	TYPE_D_DRIVING		        0x03

#define	DDR_FIX_RX_DAT			0x00
#define	DDR_VAR_RX_DAT			0x80
#define	DDR_FIX_RX_DAT_EDGE		0x00
#define	DDR_FIX_RX_DAT_14_DELAY		0x40
#define	DDR_FIX_RX_CMD			0x00
#define	DDR_VAR_RX_CMD			0x20
#define	DDR_FIX_RX_CMD_POS_EDGE		0x00
#define	DDR_FIX_RX_CMD_14_DELAY		0x10
#define	SD20_RX_POS_EDGE		0x00
#define	SD20_RX_14_DELAY		0x08
#define SD20_RX_SEL_MASK		0x08

#define	DDR_FIX_TX_CMD_DAT		0x00
#define	DDR_VAR_TX_CMD_DAT		0x80
#define	DDR_FIX_TX_DAT_14_TSU		0x00
#define	DDR_FIX_TX_DAT_12_TSU		0x40
#define	DDR_FIX_TX_CMD_NEG_EDGE		0x00
#define	DDR_FIX_TX_CMD_14_AHEAD		0x20
#define	SD20_TX_NEG_EDGE		0x00
#define	SD20_TX_14_AHEAD		0x10
#define SD20_TX_SEL_MASK		0x10
#define	DDR_VAR_SDCLK_POL_SWAP		0x01

#define	SD_TRANSFER_START		0x80
#define	SD_TRANSFER_END			0x40
#define SD_STAT_IDLE			0x20
#define	SD_TRANSFER_ERR			0x10
#define	SD_TM_NORMAL_WRITE		0x00
#define	SD_TM_AUTO_WRITE_3		0x01
#define	SD_TM_AUTO_WRITE_4		0x02
#define	SD_TM_AUTO_READ_3		0x05
#define	SD_TM_AUTO_READ_4		0x06
#define	SD_TM_CMD_RSP			0x08
#define	SD_TM_AUTO_WRITE_1		0x09
#define	SD_TM_AUTO_WRITE_2		0x0A
#define	SD_TM_NORMAL_READ		0x0C
#define	SD_TM_AUTO_READ_1		0x0D
#define	SD_TM_AUTO_READ_2		0x0E
#define	SD_TM_AUTO_TUNING		0x0F

#define PHASE_CHANGE			0x80
#define PHASE_NOT_RESET			0x40

#define DCMPS_CHANGE			0x80
#define DCMPS_CHANGE_DONE	   	0x40
#define DCMPS_ERROR			0x20
#define DCMPS_CURRENT_PHASE     	0x1F


#define SD_CLK_DIVIDE_0			0x00
#define	SD_CLK_DIVIDE_256		0xC0
#define	SD_CLK_DIVIDE_128		0x80
#define	SD_BUS_WIDTH_1			0x00
#define	SD_BUS_WIDTH_4			0x01
#define	SD_BUS_WIDTH_8			0x02
#define	SD_ASYNC_FIFO_NOT_RST		0x10
#define	SD_20_MODE			0x00
#define	SD_DDR_MODE			0x04
#define	SD_30_MODE			0x08

#define SD_CLK_DIVIDE_MASK		0xC0

#define SD_CMD_IDLE			0x80

#define SD_DATA_IDLE			0x80

#define DCM_RESET			0x08
#define DCM_LOCKED			0x04
#define DCM_208M			0x00
#define DCM_TX			        0x01
#define DCM_RX			        0x02

#define DRP_START			0x80
#define DRP_DONE			0x40

#define DRP_WRITE			0x80
#define DRP_READ			0x00
#define DCM_WRITE_ADDRESS_50		0x50
#define DCM_WRITE_ADDRESS_51		0x51
#define DCM_READ_ADDRESS_00		0x00
#define DCM_READ_ADDRESS_51		0x51

#define	SD_CALCULATE_CRC7		0x00
#define	SD_NO_CALCULATE_CRC7		0x80
#define	SD_CHECK_CRC16			0x00
#define	SD_NO_CHECK_CRC16		0x40
#define SD_NO_CHECK_WAIT_CRC_TO		0x20
#define	SD_WAIT_BUSY_END		0x08
#define	SD_NO_WAIT_BUSY_END		0x00
#define	SD_CHECK_CRC7			0x00
#define	SD_NO_CHECK_CRC7		0x04
#define	SD_RSP_LEN_0			0x00
#define	SD_RSP_LEN_6			0x01
#define	SD_RSP_LEN_17			0x02
#define	SD_RSP_TYPE_R0		0x04
#define	SD_RSP_TYPE_R1		0x01
#define	SD_RSP_TYPE_R1b		0x09
#define	SD_RSP_TYPE_R2		0x02
#define	SD_RSP_TYPE_R3		0x05
#define	SD_RSP_TYPE_R4		0x05
#define	SD_RSP_TYPE_R5		0x01
#define	SD_RSP_TYPE_R6		0x01
#define	SD_RSP_TYPE_R7		0x01

#define	SD_RSP_80CLK_TIMEOUT_EN	    0x01

#define	SAMPLE_TIME_RISING		0x00
#define	SAMPLE_TIME_FALLING		0x80
#define	PUSH_TIME_DEFAULT		0x00
#define	PUSH_TIME_ODD			0x40
#define	NO_EXTEND_TOGGLE		0x00
#define	EXTEND_TOGGLE_CHK		0x20
#define	MS_BUS_WIDTH_1			0x00
#define	MS_BUS_WIDTH_4			0x10
#define	MS_BUS_WIDTH_8			0x18
#define	MS_2K_SECTOR_MODE		0x04
#define	MS_512_SECTOR_MODE		0x00
#define	MS_TOGGLE_TIMEOUT_EN		0x00
#define	MS_TOGGLE_TIMEOUT_DISEN		0x01
#define MS_NO_CHECK_INT			0x02

#define	WAIT_INT			0x80
#define	NO_WAIT_INT			0x00
#define	NO_AUTO_READ_INT_REG		0x00
#define	AUTO_READ_INT_REG		0x40
#define	MS_CRC16_ERR			0x20
#define	MS_RDY_TIMEOUT			0x10
#define	MS_INT_CMDNK			0x08
#define	MS_INT_BREQ			0x04
#define	MS_INT_ERR			0x02
#define	MS_INT_CED			0x01

#define	MS_TRANSFER_START		0x80
#define	MS_TRANSFER_END			0x40
#define	MS_TRANSFER_ERR			0x20
#define	MS_BS_STATE			0x10
#define	MS_TM_READ_BYTES		0x00
#define	MS_TM_NORMAL_READ		0x01
#define	MS_TM_WRITE_BYTES		0x04
#define	MS_TM_NORMAL_WRITE		0x05
#define	MS_TM_AUTO_READ			0x08
#define	MS_TM_AUTO_WRITE		0x0C

#define CARD_SHARE_MASK			0x0F
#define	CARD_SHARE_NORMAL		0x00
#define	CARD_SHARE_SD			0x04
#define	CARD_SHARE_MS			0x08

#define SD_STOP			0x04
#define MS_STOP			0x08
#define SD_CLR_ERR		0x40
#define MS_CLR_ERR		0x80

#define CRC_FIX_CLK		(0x00 << 0)
#define CRC_VAR_CLK0		(0x01 << 0)
#define CRC_VAR_CLK1		(0x02 << 0)
#define SD30_FIX_CLK		(0x00 << 2)
#define SD30_VAR_CLK0		(0x01 << 2)
#define SD30_VAR_CLK1		(0x02 << 2)
#define SAMPLE_FIX_CLK		(0x00 << 4)
#define SAMPLE_VAR_CLK0		(0x01 << 4)
#define SAMPLE_VAR_CLK1		(0x02 << 4)

#define PINGPONG_BUFFER		0x01
#define RING_BUFFER		0x00

#define RB_FLUSH		0x80
#define RB_FULL			0x02

#define DMA_DONE_INT_EN			0x80
#define SUSPEND_INT_EN			0x40
#define LINK_RDY_INT_EN			0x20
#define LINK_DOWN_INT_EN		0x10

#define DMA_DONE_INT			0x80
#define SUSPEND_INT			0x40
#define LINK_RDY_INT			0x20
#define LINK_DOWN_INT			0x10

#define MRD_ERR_INT_EN			0x40
#define MWR_ERR_INT_EN			0x20
#define SCSI_CMD_INT_EN			0x10
#define TLP_RCV_INT_EN			0x08
#define TLP_TRSMT_INT_EN		0x04
#define MRD_COMPLETE_INT_EN		0x02
#define MWR_COMPLETE_INT_EN		0x01

#define MRD_ERR_INT			0x40
#define MWR_ERR_INT			0x20
#define SCSI_CMD_INT			0x10
#define TLP_RX_INT			0x08
#define TLP_TX_INT			0x04
#define MRD_COMPLETE_INT		0x02
#define MWR_COMPLETE_INT		0x01

#define MSG_RX_INT_EN			0x08
#define MRD_RX_INT_EN			0x04
#define MWR_RX_INT_EN			0x02
#define CPLD_RX_INT_EN			0x01

#define MSG_RX_INT			0x08
#define MRD_RX_INT			0x04
#define MWR_RX_INT			0x02
#define CPLD_RX_INT			0x01

#define MSG_TX_INT_EN			0x08
#define MRD_TX_INT_EN			0x04
#define MWR_TX_INT_EN			0x02
#define CPLD_TX_INT_EN			0x01

#define MSG_TX_INT			0x08
#define MRD_TX_INT			0x04
#define MWR_TX_INT			0x02
#define CPLD_TX_INT			0x01

#define DMA_RST				0x80
#define DMA_BUSY			0x04
#define DMA_DIR_TO_CARD			0x00
#define DMA_DIR_FROM_CARD		0x02
#define DMA_EN				0x01
#define DMA_128				(0 << 4)
#define DMA_256				(1 << 4)
#define DMA_512				(2 << 4)
#define DMA_1024			(3 << 4)
#define DMA_PACK_SIZE_MASK		0x30


#define	RSTB_MODE_DETECT		0x80
#define	MODE_OUT_VLD			0x40
#define	MODE_OUT_0_NONE			0x00
#define	MODE_OUT_10_NONE		0x04
#define	MODE_OUT_10_47			0x05
#define	MODE_OUT_10_180			0x06
#define	MODE_OUT_10_680			0x07
#define	MODE_OUT_16_NONE		0x08
#define	MODE_OUT_16_47			0x09
#define	MODE_OUT_16_180			0x0A
#define	MODE_OUT_16_680			0x0B
#define	MODE_OUT_NONE_NONE		0x0C
#define	MODE_OUT_NONE_47		0x0D
#define	MODE_OUT_NONE_180		0x0E
#define	MODE_OUT_NONE_680		0x0F

#define SD_DETECT_EN			0x08
#define SD_OCP_INT_EN			0x04
#define SD_OCP_INT_CLR			0x02
#define SD_OC_CLR			0x01

#define SD_OCP_DETECT			0x08
#define SD_OC_NOW			0x04
#define SD_OC_EVER			0x02

#define SD_OCP_GLITCH_MASK		0x07
#define SD_OCP_GLITCH_6_4		0x00
#define SD_OCP_GLITCH_64		0x01
#define SD_OCP_GLITCH_640		0x02
#define SD_OCP_GLITCH_1000		0x03
#define SD_OCP_GLITCH_2000		0x04
#define SD_OCP_GLITCH_4000		0x05
#define SD_OCP_GLITCH_8000		0x06
#define SD_OCP_GLITCH_10000		0x07


#define SD_OCP_TIME_60			0x00
#define SD_OCP_TIME_100			0x01
#define SD_OCP_TIME_200			0x02
#define SD_OCP_TIME_400			0x03
#define SD_OCP_TIME_600			0x04
#define SD_OCP_TIME_800			0x05
#define SD_OCP_TIME_1100		0x06
#define SD_OCP_TIME_MASK		0x07

#define SD_OCP_THD_450			0x00
#define SD_OCP_THD_550			0x01
#define SD_OCP_THD_650			0x02
#define SD_OCP_THD_750			0x03
#define SD_OCP_THD_850			0x04
#define SD_OCP_THD_950			0x05
#define SD_OCP_THD_1050			0x06
#define SD_OCP_THD_1150			0x07
#define SD_OCP_THD_MASK			0x07

#define FPGA_MS_PULL_CTL_EN		0xEF
#define FPGA_SD_PULL_CTL_EN		0xF7
#define FPGA_MS_PULL_CTL_BIT		0x10
#define FPGA_SD_PULL_CTL_BIT		0x08

#define LED_SHINE_EN			0x08
#define LED_SPEED_MASK			0x07

#define SSC_RSTB		0x80
#define SSC_8X_EN		0x40
#define SSC_FIX_FRAC		0x20
#define SSC_SEL_1M		0x00
#define SSC_SEL_2M		0x08
#define SSC_SEL_4M		0x10
#define SSC_SEL_8M		0x18

#define SSC_DEPTH_MASK		0x07
#define SSC_DEPTH_DISALBE	0x00
#define SSC_DEPTH_4M		0x01
#define SSC_DEPTH_2M		0x02
#define SSC_DEPTH_1M		0x03
#define SSC_DEPTH_512K		0x04
#define SSC_DEPTH_256K		0x05
#define SSC_DEPTH_128K		0x06
#define SSC_DEPTH_64K		0x07

#define SD_D7_NP		0x00
#define SD_D7_PD		(0x01 << 4)
#define SD_DAT7_PU		(0x02 << 4)
#define SD_CLK_NP		0x00
#define SD_CLK_PD		(0x01 << 2)
#define SD_CLK_PU		(0x02 << 2)
#define SD_D5_NP		0x00
#define SD_D5_PD		0x01
#define SD_D5_PU		0x02

#define MS_D1_NP		0x00
#define MS_D1_PD		(0x01 << 6)
#define MS_D1_PU		(0x02 << 6)
#define MS_D2_NP		0x00
#define MS_D2_PD		(0x01 << 4)
#define MS_D2_PU		(0x02 << 4)
#define MS_CLK_NP		0x00
#define MS_CLK_PD		(0x01 << 2)
#define MS_CLK_PU		(0x02 << 2)
#define MS_D6_NP		0x00
#define MS_D6_PD		0x01
#define MS_D6_PU		0x02

#define SD_D6_NP		0x00
#define SD_D6_PD		(0x01 << 6)
#define SD_D6_PU		(0x02 << 6)
#define SD_D0_NP		0x00
#define SD_D0_PD		(0x01 << 4)
#define SD_D0_PU		(0x02 << 4)
#define SD_D1_NP		0x00
#define SD_D1_PD		0x01
#define SD_D1_PU		0x02

#define MS_D3_NP		0x00
#define MS_D3_PD		(0x01 << 6)
#define MS_D3_PU		(0x02 << 6)
#define MS_D0_NP		0x00
#define MS_D0_PD		(0x01 << 4)
#define MS_D0_PU		(0x02 << 4)
#define MS_BS_NP		0x00
#define MS_BS_PD		(0x01 << 2)
#define MS_BS_PU		(0x02 << 2)

#define SD_D4_NP		0x00
#define SD_D4_PD		(0x01 << 6)
#define SD_D4_PU		(0x02 << 6)

#define MS_D7_NP		0x00
#define MS_D7_PD		(0x01 << 6)
#define MS_D7_PU		(0x02 << 6)

#define SD_D3_NP		0x00
#define SD_D3_PD		(0x01 << 4)
#define SD_D3_PU		(0x02 << 4)
#define SD_D2_NP		0x00
#define SD_D2_PD		(0x01 << 2)
#define SD_D2_PU		(0x02 << 2)

#define MS_INS_PD		0x00
#define MS_INS_PU		(0x01 << 7)
#define SD_WP_NP		0x00
#define SD_WP_PD		(0x01 << 5)
#define SD_WP_PU		(0x02 << 5)
#define SD_CD_PD		0x00
#define SD_CD_PU		(0x01 << 4)
#define SD_CMD_NP		0x00
#define SD_CMD_PD		(0x01 << 2)
#define SD_CMD_PU		(0x02 << 2)

#define MS_D5_NP		0x00
#define MS_D5_PD		(0x01 << 2)
#define MS_D5_PU		(0x02 << 2)
#define MS_D4_NP		0x00
#define MS_D4_PD		0x01
#define MS_D4_PU		0x02

#define FORCE_PM_CLOCK		0x10
#define EN_CLOCK_PM		0x01

#define HOST_ENTER_S3		0x02
#define HOST_ENTER_S1		0x01

#define AUX_PWR_DETECTED	0x01

#define PHY_DEBUG_MODE		0x01

#define PWR_GATE_EN		0x01
#define LDO3318_PWR_MASK	0x06
#define LDO_ON			0x06
#define LDO_SUSPEND		0x02
#define LDO_OFF			0x00

#define SD_CFG1			0xFDA0
#define SD_CFG2			0xFDA1
#define SD_CFG3			0xFDA2
#define SD_STAT1		0xFDA3
#define SD_STAT2		0xFDA4
#define SD_BUS_STAT		0xFDA5
#define SD_PAD_CTL		0xFDA6
#define SD_SAMPLE_POINT_CTL	0xFDA7
#define SD_PUSH_POINT_CTL	0xFDA8
#define SD_CMD0			0xFDA9
#define SD_CMD1			0xFDAA
#define SD_CMD2			0xFDAB
#define SD_CMD3			0xFDAC
#define SD_CMD4			0xFDAD
#define SD_CMD5			0xFDAE
#define SD_BYTE_CNT_L		0xFDAF
#define SD_BYTE_CNT_H		0xFDB0
#define SD_BLOCK_CNT_L		0xFDB1
#define SD_BLOCK_CNT_H		0xFDB2
#define SD_TRANSFER		0xFDB3
#define SD_CMD_STATE		0xFDB5
#define SD_DATA_STATE		0xFDB6

#define	DCM_DRP_CTL         	0xFC23
#define	DCM_DRP_TRIG		0xFC24
#define	DCM_DRP_CFG         	0xFC25
#define	DCM_DRP_WR_DATA_L   	0xFC26
#define	DCM_DRP_WR_DATA_H   	0xFC27
#define	DCM_DRP_RD_DATA_L   	0xFC28
#define	DCM_DRP_RD_DATA_H   	0xFC29
#define SD_VPCLK0_CTL		0xFC2A
#define SD_VPCLK1_CTL		0xFC2B
#define SD_DCMPS0_CTL		0xFC2C
#define SD_DCMPS1_CTL		0xFC2D
#define SD_VPTX_CTL		SD_VPCLK0_CTL
#define SD_VPRX_CTL		SD_VPCLK1_CTL
#define SD_DCMPS_TX_CTL		SD_DCMPS0_CTL
#define SD_DCMPS_RX_CTL		SD_DCMPS1_CTL

#define CARD_CLK_SOURCE		0xFC2E

#define CARD_PWR_CTL		0xFD50
#define CARD_CLK_SWITCH		0xFD51
#define CARD_SHARE_MODE		0xFD52
#define CARD_DRIVE_SEL		0xFD53
#define CARD_STOP		0xFD54
#define CARD_OE			0xFD55

#define OLT_LED_CTL		0xFC1E
#define GPIO_CTL		0xFC1F

#define CARD_DATA_SOURCE	0xFD5B
#define CARD_SELECT		0xFD5C
#define SD30_DRIVE_SEL		0xFD5E

#define CARD_CLK_EN		0xFD69

#define FPDCTL			0xFC00
#define PDINFO			0xFC01

#define CLK_CTL			0xFC02
#define CLK_DIV			0xFC03
#define CLK_SEL			0xFC04

#define SSC_DIV_N_0		0xFC0F
#define SSC_DIV_N_1		0xFC10

#define RCCTL			0xFC14

#define FPGA_PULL_CTL		0xFC1D

#define CARD_PULL_CTL1		0xFD60
#define CARD_PULL_CTL2		0xFD61
#define CARD_PULL_CTL3		0xFD62
#define CARD_PULL_CTL4		0xFD63
#define CARD_PULL_CTL5		0xFD64
#define CARD_PULL_CTL6		0xFD65

#define IRQEN0				0xFE20
#define IRQSTAT0			0xFE21
#define IRQEN1				0xFE22
#define IRQSTAT1			0xFE23
#define TLPRIEN				0xFE24
#define TLPRISTAT			0xFE25
#define TLPTIEN				0xFE26
#define TLPTISTAT			0xFE27
#define DMATC0				0xFE28
#define DMATC1				0xFE29
#define DMATC2				0xFE2A
#define DMATC3				0xFE2B
#define DMACTL				0xFE2C
#define BCTL				0xFE2D
#define RBBC0				0xFE2E
#define RBBC1				0xFE2F
#define RBDAT				0xFE30
#define RBCTL				0xFE34
#define CFGADDR0			0xFE35
#define CFGADDR1			0xFE36
#define CFGDATA0			0xFE37
#define CFGDATA1			0xFE38
#define CFGDATA2			0xFE39
#define CFGDATA3			0xFE3A
#define CFGRWCTL			0xFE3B
#define PHYRWCTL			0xFE3C
#define PHYDATA0			0xFE3D
#define PHYDATA1			0xFE3E
#define PHYADDR				0xFE3F
#define MSGRXDATA0			0xFE40
#define MSGRXDATA1			0xFE41
#define MSGRXDATA2			0xFE42
#define MSGRXDATA3			0xFE43
#define MSGTXDATA0			0xFE44
#define MSGTXDATA1			0xFE45
#define MSGTXDATA2			0xFE46
#define MSGTXDATA3			0xFE47
#define MSGTXCTL			0xFE48
#define PETXCFG				0xFE49
#define LTR_CTL				0xFE4A

#define CDRESUMECTL			0xFE52
#define WAKE_SEL_CTL			0xFE54
#define PME_FORCE_CTL			0xFE56
#define ASPM_FORCE_CTL			0xFE57
#define PM_CLK_FORCE_CTL		0xFE58
#define PERST_GLITCH_WIDTH		0xFE5C
#define CHANGE_LINK_STATE		0xFE5B
#define RESET_LOAD_REG			0xFE5E
#define HOST_SLEEP_STATE		0xFE60

#define NFTS_TX_CTRL			0xFE72

#define PWR_GATE_CTRL			0xFE75
#define PWD_SUSPEND_EN			0xFE76

#define MS_CFG				0xFD40
#define MS_TPC				0xFD41
#define MS_TRANS_CFG			0xFD42
#define MS_TRANSFER			0xFD43
#define MS_INT_REG			0xFD44
#define MS_BYTE_CNT			0xFD45
#define MS_SECTOR_CNT_L			0xFD46
#define MS_SECTOR_CNT_H			0xFD47
#define MS_DBUS_H			0xFD48

#define SSC_CTL1			0xFC11
#define SSC_CTL2			0xFC12

#define OCPCTL				0xFC15
#define OCPSTAT				0xFC16
#define OCPGLITCH			0xFC17
#define OCPPARA1			0xFC18
#define OCPPARA2			0xFC19

#define SRAM_BASE			0xE600
#define RBUF_BASE			0xF400
#define PPBUF_BASE1			0xF800
#ifdef USING_PPBUF
#define PPBUF_BASE2			0xFA00
#else
#define PPBUF_BASE2			0xF800
#endif
#define IMAGE_FLAG_ADDR0		0xCE80
#define IMAGE_FLAG_ADDR1		0xCE81

#ifdef USING_PPBUF
#define PPBUF_LEN			512
#else
#define PPBUF_LEN			64
#endif

#define READ_OP			1
#define WRITE_OP		2

#define LCTLR		0x80

#define POLLING_WAIT_CNT	1
#define IDLE_MAX_COUNT		10

#define DEBOUNCE_CNT			5

void do_remaining_work(struct rtsx_chip *chip);
void do_reset_sd_card(struct rtsx_chip *chip);
void do_reset_ms_card(struct rtsx_chip *chip);
void rtsx_power_off_card(struct rtsx_chip *chip);
void rtsx_release_cards(struct rtsx_chip *chip);
void rtsx_reset_cards(struct rtsx_chip *chip);
void rtsx_reinit_cards(struct rtsx_chip *chip, int reset_chip);
void rtsx_init_cards(struct rtsx_chip *chip);
int switch_ssc_clock(struct rtsx_chip *chip, int clk);
int switch_normal_clock(struct rtsx_chip *chip, int clk);
int enable_card_clock(struct rtsx_chip *chip, u8 card);
int disable_card_clock(struct rtsx_chip *chip, u8 card);
int card_rw(struct scsi_cmnd *srb, struct rtsx_chip *chip, u32 sec_addr, u16 sec_cnt);
void trans_dma_enable(enum dma_data_direction dir, struct rtsx_chip *chip, u32 byte_cnt, u8 pack_size);
void toggle_gpio(struct rtsx_chip *chip);
void toggle_led(struct rtsx_chip *chip);
void turn_on_led(struct rtsx_chip *chip);
void turn_off_led(struct rtsx_chip *chip);
#ifdef LED_AUTO_BLINK
void enable_auto_blink(struct rtsx_chip *chip);
void disable_auto_blink(struct rtsx_chip *chip);
#endif

int card_share_mode(struct rtsx_chip *chip, int card);
int select_card(struct rtsx_chip *chip, int card);
int detect_card_cd(struct rtsx_chip *chip, int card);
int check_card_exist(struct rtsx_chip *chip, unsigned int lun);
int check_card_ready(struct rtsx_chip *chip, unsigned int lun);
int check_card_wp(struct rtsx_chip *chip, unsigned int lun);
int check_card_fail(struct rtsx_chip *chip, unsigned int lun);
int check_card_ejected(struct rtsx_chip *chip, unsigned int lun);
void eject_card(struct rtsx_chip *chip, unsigned int lun);
u8 get_lun_card(struct rtsx_chip *chip, unsigned int lun);

static inline u32 get_card_size(struct rtsx_chip *chip, unsigned int lun)
{
#ifdef SUPPORT_SD_LOCK
	struct sd_info *sd_card = &(chip->sd_card);

	if ((get_lun_card(chip, lun) == SD_CARD) && (sd_card->sd_lock_status & SD_LOCKED)) {
		return 0;
	} else {
		return chip->capacity[lun];
	}
#else
	return chip->capacity[lun];
#endif
}

static inline int switch_clock(struct rtsx_chip *chip, int clk)
{
	int retval = 0;

	if (chip->asic_code) {
		retval = switch_ssc_clock(chip, clk);
	} else {
		retval = switch_normal_clock(chip, clk);
	}

	return retval;
}

int card_power_on(struct rtsx_chip *chip, u8 card);
int card_power_off(struct rtsx_chip *chip, u8 card);

static inline int card_power_off_all(struct rtsx_chip *chip)
{
	RTSX_WRITE_REG(chip, CARD_PWR_CTL, 0x0F, 0x0F);

	return STATUS_SUCCESS;
}

static inline void rtsx_clear_sd_error(struct rtsx_chip *chip)
{
	rtsx_write_register(chip, CARD_STOP, SD_STOP | SD_CLR_ERR, SD_STOP | SD_CLR_ERR);
}

static inline void rtsx_clear_ms_error(struct rtsx_chip *chip)
{
	rtsx_write_register(chip, CARD_STOP, MS_STOP | MS_CLR_ERR, MS_STOP | MS_CLR_ERR);
}

#endif


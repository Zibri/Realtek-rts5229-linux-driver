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

#include "rtsx.h"
#include "rtsx_scsi.h"
#include "rtsx_transport.h"
#include "rtsx_chip.h"
#include "rtsx_card.h"
#include "debug.h"

/***********************************************************************
 * Scatter-gather transfer buffer access routines
 ***********************************************************************/

/* Copy a buffer of length buflen to/from the srb's transfer buffer.
 * (Note: for scatter-gather transfers (srb->use_sg > 0), srb->request_buffer
 * points to a list of s-g entries and we ignore srb->request_bufflen.
 * For non-scatter-gather transfers, srb->request_buffer points to the
 * transfer buffer itself and srb->request_bufflen is the buffer's length.)
 * Update the *index and *offset variables so that the next copy will
 * pick up from where this one left off. */

unsigned int rtsx_stor_access_xfer_buf(unsigned char *buffer,
	unsigned int buflen, struct scsi_cmnd *srb, unsigned int *index,
	unsigned int *offset, enum xfer_buf_dir dir)
{
	unsigned int cnt;

	/* If not using scatter-gather, just transfer the data directly.
	 * Make certain it will fit in the available buffer space. */
	if (scsi_sg_count(srb) == 0) {
		if (*offset >= scsi_bufflen(srb))
			return 0;
		cnt = min(buflen, scsi_bufflen(srb) - *offset);
		if (dir == TO_XFER_BUF)
			memcpy((unsigned char *) scsi_sglist(srb) + *offset,
					buffer, cnt);
		else
			memcpy(buffer, (unsigned char *) scsi_sglist(srb) +
					*offset, cnt);
		*offset += cnt;

	/* Using scatter-gather.  We have to go through the list one entry
	 * at a time.  Each s-g entry contains some number of pages, and
	 * each page has to be kmap()'ed separately.  If the page is already
	 * in kernel-addressable memory then kmap() will return its address.
	 * If the page is not directly accessible -- such as a user buffer
	 * located in high memory -- then kmap() will map it to a temporary
	 * position in the kernel's virtual address space. */
	} else {
		struct scatterlist *sg =
				(struct scatterlist *) scsi_sglist(srb)
				+ *index;

		/* This loop handles a single s-g list entry, which may
		 * include multiple pages.  Find the initial page structure
		 * and the starting offset within the page, and update
		 * the *offset and *index values for the next loop. */
		cnt = 0;
		while (cnt < buflen && *index < scsi_sg_count(srb)) {
			struct page *page = sg_page(sg) +
					((sg->offset + *offset) >> PAGE_SHIFT);
			unsigned int poff =
					(sg->offset + *offset) & (PAGE_SIZE-1);
			unsigned int sglen = sg->length - *offset;

			if (sglen > buflen - cnt) {


				sglen = buflen - cnt;
				*offset += sglen;
			} else {


				*offset = 0;
				++*index;
				++sg;
			}

			/* Transfer the data for all the pages in this
			 * s-g entry.  For each page: call kmap(), do the
			 * transfer, and call kunmap() immediately after. */
			while (sglen > 0) {
				unsigned int plen = min(sglen, (unsigned int)
						PAGE_SIZE - poff);
				unsigned char *ptr = kmap(page);

				if (dir == TO_XFER_BUF)
					memcpy(ptr + poff, buffer + cnt, plen);
				else
					memcpy(buffer + cnt, ptr + poff, plen);
				kunmap(page);


				poff = 0;
				++page;
				cnt += plen;
				sglen -= plen;
			}
		}
	}


	return cnt;
}

/* Store the contents of buffer into srb's transfer buffer and set the
* SCSI residue. */
void rtsx_stor_set_xfer_buf(unsigned char *buffer,
       unsigned int buflen, struct scsi_cmnd *srb)
{
       unsigned int index = 0, offset = 0;

       rtsx_stor_access_xfer_buf(buffer, buflen, srb, &index, &offset,
		       TO_XFER_BUF);
       if (buflen < scsi_bufflen(srb))
	       scsi_set_resid(srb, scsi_bufflen(srb) - buflen);
}

void rtsx_stor_get_xfer_buf(unsigned char *buffer,
       unsigned int buflen, struct scsi_cmnd *srb)
{
       unsigned int index = 0, offset = 0;

       rtsx_stor_access_xfer_buf(buffer, buflen, srb, &index, &offset,
		       FROM_XFER_BUF);
       if (buflen < scsi_bufflen(srb))
	       scsi_set_resid(srb, scsi_bufflen(srb) - buflen);
}


/***********************************************************************
 * Transport routines
 ***********************************************************************/

/* Invoke the transport and basic error-handling/recovery methods
 *
 * This is used to send the message to the device and receive the response.
 */
void rtsx_invoke_transport(struct scsi_cmnd *srb, struct rtsx_chip *chip)
{
	int result;

	result = rtsx_scsi_handler(srb, chip);

	/* if the command gets aborted by the higher layers, we need to
	 * short-circuit all other processing
	 */
	if (rtsx_chk_stat(chip, RTSX_STAT_ABORT)) {
		RTSX_DEBUGP(("-- command was aborted\n"));
		srb->result = DID_ABORT << 16;
		goto Handle_Errors;
	}


	if (result == TRANSPORT_ERROR) {
		RTSX_DEBUGP(("-- transport indicates error, resetting\n"));
		srb->result = DID_ERROR << 16;
		goto Handle_Errors;
	}

	srb->result = SAM_STAT_GOOD;

	/*
	 * If we have a failure, we're going to do a REQUEST_SENSE
	 * automatically.  Note that we differentiate between a command
	 * "failure" and an "error" in the transport mechanism.
	 */
	if (result == TRANSPORT_FAILED) {

		srb->result = SAM_STAT_CHECK_CONDITION;
		memcpy(srb->sense_buffer, (unsigned char *)&(chip->sense_buffer[SCSI_LUN(srb)]),
				sizeof(struct sense_data_t));
	}

	return;

	/* Error and abort processing: try to resynchronize with the device
	 * by issuing a port reset.  If that fails, try a class-specific
	 * device reset. */
Handle_Errors:
	return;
}

/**
 * rtsx_add_cmd - add a command to command buffer.
 * @chip: Realtek's card reader chip
 * @cmd_type: command type, including read/write/check register
 * @reg_addr: internal card controller register address
 * @mask: bit mask
 * @data: register data
 *
 * Add a command to command buffer.
 *
 * Usually, this function is called after rtsx_init_cmd, which
 * intializes the command index to zero. After all commands are added,
 * rtsx_send_cmd or rtsx_send_cmd_no_wait should be called to send those
 * commands to card reader chip.
 */
void rtsx_add_cmd(struct rtsx_chip *chip,
		u8 cmd_type, u16 reg_addr, u8 mask, u8 data)
{
	u32 *cb = (u32 *)(chip->host_cmds_ptr);
	u32 val = 0;

	val |= (u32)(cmd_type & 0x03) << 30;
	val |= (u32)(reg_addr & 0x3FFF) << 16;
	val |= (u32)mask << 8;
	val |= (u32)data;

	spin_lock_irq(&chip->rtsx->reg_lock);
	if (chip->ci < (HOST_CMDS_BUF_LEN / 4)) {
		cb[(chip->ci) ++] = cpu_to_le32(val);
	}
	spin_unlock_irq(&chip->rtsx->reg_lock);
}

/**
 * rtsx_send_cmd_no_wait - send commands to chip.
 * @chip: Realtek's card reader chip
 *
 * Trigger card reader chip to fetch commands from command buffer.
 * This funtion returns immediately.
 */
void rtsx_send_cmd_no_wait(struct rtsx_chip *chip)
{
	u32 val = 1 << 31;

	rtsx_writel(chip, RTSX_HCBAR, chip->host_cmds_addr);

	val |= (u32)(chip->ci * 4) & 0x00FFFFFF;
	val |= 0x40000000;
	rtsx_writel(chip, RTSX_HCBCTLR, val);
}

/**
 * rtsx_send_cmd - send commands to chip.
 * @chip: Realtek's card reader chip
 * @card: this command is relevant to card or not
 * @timeout: time out in millisecond
 *
 * Trigger card reader chip to fetch commands from command buffer.
 * This funtion will wait for transfer-finished interrupt.
 *
 * Returns zero if successful, or a negative error code on failure.
 */
int rtsx_send_cmd(struct rtsx_chip *chip, u8 card, int timeout)
{
	struct rtsx_dev *rtsx = chip->rtsx;
	struct completion trans_done;
	u32 val = 1 << 31;
	long timeleft;
	int err = 0;

	if (card == SD_CARD) {
		rtsx->check_card_cd = SD_EXIST;
	} else if (card == MS_CARD) {
		rtsx->check_card_cd = MS_EXIST;
	} else {
		rtsx->check_card_cd = 0;
	}

	spin_lock_irq(&rtsx->reg_lock);


	rtsx->done = &trans_done;
	rtsx->trans_result = TRANS_NOT_READY;
	init_completion(&trans_done);
	rtsx->trans_state = STATE_TRANS_CMD;

	rtsx_writel(chip, RTSX_HCBAR, chip->host_cmds_addr);

	val |= (u32)(chip->ci * 4) & 0x00FFFFFF;
	val |= 0x40000000;
	rtsx_writel(chip, RTSX_HCBCTLR, val);

	spin_unlock_irq(&rtsx->reg_lock);

	timeleft = wait_for_completion_interruptible_timeout(&trans_done, timeout * HZ / 1000);
	if (timeleft <= 0) {
		RTSX_DEBUGP(("chip->int_reg = 0x%x\n", chip->int_reg));
		err = -ETIMEDOUT;
		TRACE_GOTO(chip, finish_send_cmd);
	}

	spin_lock_irq(&rtsx->reg_lock);
	if (rtsx->trans_result == TRANS_RESULT_FAIL) {
		err = -EIO;
	} else if (rtsx->trans_result == TRANS_RESULT_OK) {
		err = 0;
	}
	spin_unlock_irq(&rtsx->reg_lock);


finish_send_cmd:
	rtsx->done = NULL;
	rtsx->trans_state = STATE_TRANS_NONE;

	if (err < 0) {
		rtsx_stop_cmd(chip, card);
	}

	return err;
}

/**
 * rtsx_add_sg_tbl - add a sg entry to sg table.
 * @chip: Realtek's card reader chip
 * @addr: address of host DMA buffer to transfer data
 * @len: buffer length in bytes
 * @option: option
 *
 * Add a sg entry to sg table.
 *
 * Note: The length field is 20-bit long. So if the buffer length is
 * longer than 0x80000, this function will divide the buffer into
 * several small buffers to ensure the length field won't overflow.
 */
static inline void rtsx_add_sg_tbl(struct rtsx_chip *chip, u32 addr, u32 len, u8 option)
{
	u64 *sgb = (u64 *)(chip->host_sg_tbl_ptr);
	u64 val = 0;
	u32 temp_len = 0;
	u8  temp_opt = 0;

	do {
		if (len > 0x80000) {
			temp_len = 0x80000;
			temp_opt = option & (~SG_END);
		} else {
			temp_len = len;
			temp_opt = option;
		}
		val = ((u64)addr << 32) | ((u64)temp_len << 12) | temp_opt;

		if (chip->sgi < (HOST_SG_TBL_BUF_LEN / 8)) {
			sgb[(chip->sgi) ++] = cpu_to_le64(val);
		}

		len -= temp_len;
		addr += temp_len;
	} while (len);
}

/**
 * rtsx_transfer_sglist_adma_partial - transfer sg list partially in adma mode
 * @chip: Realtek's card reader chip
 * @card: this command is relevant to card or not
 * @sg: scatter-gather list
 * @num_sg: entry count of sg list
 * @index: next transfer will pick up from which sg entry
 * @offset: next transfer will pick up from the offset in the sg entry
 * @size: transfer size in bytes
 * @dma_dir: transfer direction (DMA_FROM_DEVICE or DMA_TO_DEVICE)
 * @timeout: time out in millisecond
 *
 * Transfer partial data in scatter-gather mode. In this mode,
 * ADMA option will be turned on.
 *
 * This function is usually called in MS card flow. In MS
 * read/write function, one transfer stage will be divided to several stages.
 * The *index and *offset variables are used to record the postion in
 * scatter-gather list that the next transfer will pick up.
 */
static int rtsx_transfer_sglist_adma_partial(struct rtsx_chip *chip, u8 card, struct scatterlist *sg,
		int num_sg, unsigned int *index, unsigned int *offset, int size,
	       	enum dma_data_direction dma_dir, int timeout)
{
	struct rtsx_dev *rtsx = chip->rtsx;
	struct completion trans_done;
	u8 dir;
	int sg_cnt, i, resid;
	int err = 0;
	long timeleft;
	struct scatterlist *sg_ptr;
	u32 val = TRIG_DMA;

	if ((sg == NULL) || (num_sg <= 0) || !offset || !index) {
		return -EIO;
	}

	if (dma_dir == DMA_TO_DEVICE) {
		dir = HOST_TO_DEVICE;
	} else if (dma_dir == DMA_FROM_DEVICE) {
		dir = DEVICE_TO_HOST;
	} else {
		return -ENXIO;
	}

	if (card == SD_CARD) {
		rtsx->check_card_cd = SD_EXIST;
	} else if (card == MS_CARD) {
		rtsx->check_card_cd = MS_EXIST;
	} else {
		rtsx->check_card_cd = 0;
	}

	spin_lock_irq(&rtsx->reg_lock);

	rtsx->done = &trans_done;

	rtsx->trans_state = STATE_TRANS_SG;
	rtsx->trans_result = TRANS_NOT_READY;

	spin_unlock_irq(&rtsx->reg_lock);

	sg_cnt = dma_map_sg(&(rtsx->pci->dev), sg, num_sg, dma_dir);

	resid = size;
	sg_ptr = sg;
	chip->sgi = 0;
	for (i = 0; i < *index; i++) {
		sg_ptr = sg_next(sg_ptr);
	}
	for (i = *index; i < sg_cnt; i++) {
		dma_addr_t addr;
		unsigned int len;
		u8 option;

		addr = sg_dma_address(sg_ptr);
		len = sg_dma_len(sg_ptr);

		RTSX_DEBUGP(("DMA addr: 0x%x, Len: 0x%x\n", (unsigned int)addr, len));
		RTSX_DEBUGP(("*index = %d, *offset = %d\n", *index, *offset));

		addr += *offset;

		if ((len - *offset) > resid) {
			*offset += resid;
			len = resid;
			resid = 0;
		} else {
			resid -= (len - *offset);
			len -= *offset;
			*offset = 0;
			*index = *index + 1;
		}
		if ((i == (sg_cnt - 1)) || !resid) {
			option = SG_VALID | SG_END | SG_TRANS_DATA;
		} else {
			option = SG_VALID | SG_TRANS_DATA;
		}

		rtsx_add_sg_tbl(chip, (u32)addr, (u32)len, option);

		if (!resid) {
			break;
		}

		sg_ptr = sg_next(sg_ptr);
	}

	RTSX_DEBUGP(("SG table count = %d\n", chip->sgi));

	val |= (u32)(dir & 0x01) << 29;
	val |= ADMA_MODE;

	spin_lock_irq(&rtsx->reg_lock);

	init_completion(&trans_done);

	rtsx_writel(chip, RTSX_HDBAR, chip->host_sg_tbl_addr);
	rtsx_writel(chip, RTSX_HDBCTLR, val);

	spin_unlock_irq(&rtsx->reg_lock);

	timeleft = wait_for_completion_interruptible_timeout(&trans_done, timeout * HZ / 1000);
	if (timeleft <= 0) {
		RTSX_DEBUGP(("Timeout (%s %d)\n", __FUNCTION__, __LINE__));
		RTSX_DEBUGP(("chip->int_reg = 0x%x\n", chip->int_reg));
		err = -ETIMEDOUT;
		goto out;
	}

	spin_lock_irq(&rtsx->reg_lock);
	if (rtsx->trans_result == TRANS_RESULT_FAIL) {
		err = -EIO;
		spin_unlock_irq(&rtsx->reg_lock);
		goto out;
	}
	spin_unlock_irq(&rtsx->reg_lock);


	spin_lock_irq(&rtsx->reg_lock);
	if (rtsx->trans_result == TRANS_NOT_READY) {
		init_completion(&trans_done);
		spin_unlock_irq(&rtsx->reg_lock);
		timeleft = wait_for_completion_interruptible_timeout(&trans_done, timeout * HZ / 1000);
		if (timeleft <= 0) {
			RTSX_DEBUGP(("Timeout (%s %d)\n", __FUNCTION__, __LINE__));
			RTSX_DEBUGP(("chip->int_reg = 0x%x\n", chip->int_reg));
			err = -ETIMEDOUT;
			goto out;
		}
	} else {
		spin_unlock_irq(&rtsx->reg_lock);
	}

	spin_lock_irq(&rtsx->reg_lock);
	if (rtsx->trans_result == TRANS_RESULT_FAIL) {
		err = -EIO;
	} else if (rtsx->trans_result == TRANS_RESULT_OK) {
		err = 0;
	}
	spin_unlock_irq(&rtsx->reg_lock);

out:
	rtsx->done = NULL;
	rtsx->trans_state = STATE_TRANS_NONE;
	dma_unmap_sg(&(rtsx->pci->dev), sg, num_sg, dma_dir);

	if (err < 0) {
		rtsx_stop_cmd(chip, card);
	}

	if (err == -ETIMEDOUT) {
		CATCH_TRIGGER1(chip);
	}

	return err;
}

/**
 * rtsx_transfer_sglist_adma - transfer sg list in adma mode
 * @chip: Realtek's card reader chip
 * @card: this command is relevant to card or not
 * @sg: scatter-gather list
 * @num_sg: entry count of sg list
 * @dma_dir: transfer direction (DMA_FROM_DEVICE or DMA_TO_DEVICE)
 * @timeout: time out in millisecond
 *
 * Transfer data in scatter-gather mode. In this mode, ADMA option will be turned on.
 */
static int rtsx_transfer_sglist_adma(struct rtsx_chip *chip, u8 card, struct scatterlist *sg,
		int num_sg, enum dma_data_direction dma_dir, int timeout)
{
	struct rtsx_dev *rtsx = chip->rtsx;
	struct completion trans_done;
	u8 dir;
	int buf_cnt, i;
	int err = 0;
	long timeleft;
	struct scatterlist *sg_ptr;

	if ((sg == NULL) || (num_sg <= 0)) {
		return -EIO;
	}

	if (dma_dir == DMA_TO_DEVICE) {
		dir = HOST_TO_DEVICE;
	} else if (dma_dir == DMA_FROM_DEVICE) {
		dir = DEVICE_TO_HOST;
	} else {
		return -ENXIO;
	}

	if (card == SD_CARD) {
		rtsx->check_card_cd = SD_EXIST;
	} else if (card == MS_CARD) {
		rtsx->check_card_cd = MS_EXIST;
	} else {
		rtsx->check_card_cd = 0;
	}

	spin_lock_irq(&rtsx->reg_lock);

	rtsx->done = &trans_done;

	rtsx->trans_state = STATE_TRANS_SG;
	rtsx->trans_result = TRANS_NOT_READY;

	spin_unlock_irq(&rtsx->reg_lock);

	buf_cnt = dma_map_sg(&(rtsx->pci->dev), sg, num_sg, dma_dir);

	sg_ptr = sg;

	for (i = 0; i <= buf_cnt / (HOST_SG_TBL_BUF_LEN / 8); i++) {
		u32 val = TRIG_DMA;
		int sg_cnt, j;

		if (i == buf_cnt / (HOST_SG_TBL_BUF_LEN / 8)) {
			sg_cnt = buf_cnt % (HOST_SG_TBL_BUF_LEN / 8);
		} else {
			sg_cnt = (HOST_SG_TBL_BUF_LEN / 8);
		}

		chip->sgi = 0;
		for (j = 0; j < sg_cnt; j++) {
			dma_addr_t addr = sg_dma_address(sg_ptr);
			unsigned int len = sg_dma_len(sg_ptr);
			u8 option;

			RTSX_DEBUGP(("DMA addr: 0x%x, Len: 0x%x\n", (unsigned int)addr, len));

			if (j == (sg_cnt - 1)) {
				option = SG_VALID | SG_END | SG_TRANS_DATA;
			} else {
				option = SG_VALID | SG_TRANS_DATA;
			}

			rtsx_add_sg_tbl(chip, (u32)addr, (u32)len, option);

			sg_ptr = sg_next(sg_ptr);
		}

		RTSX_DEBUGP(("SG table count = %d\n", chip->sgi));

		val |= (u32)(dir & 0x01) << 29;
		val |= ADMA_MODE;

		spin_lock_irq(&rtsx->reg_lock);

		init_completion(&trans_done);

		rtsx_writel(chip, RTSX_HDBAR, chip->host_sg_tbl_addr);
		rtsx_writel(chip, RTSX_HDBCTLR, val);

		spin_unlock_irq(&rtsx->reg_lock);

		timeleft = wait_for_completion_interruptible_timeout(&trans_done, timeout * HZ / 1000);
		if (timeleft <= 0) {
			RTSX_DEBUGP(("Timeout (%s %d)\n", __FUNCTION__, __LINE__));
			RTSX_DEBUGP(("chip->int_reg = 0x%x\n", chip->int_reg));
			err = -ETIMEDOUT;
			goto out;
		}

		spin_lock_irq(&rtsx->reg_lock);
		if (rtsx->trans_result == TRANS_RESULT_FAIL) {
			err = -EIO;
			spin_unlock_irq(&rtsx->reg_lock);
			goto out;
		}
		spin_unlock_irq(&rtsx->reg_lock);

		sg_ptr += sg_cnt;
	}

	spin_lock_irq(&rtsx->reg_lock);
	if (rtsx->trans_result == TRANS_NOT_READY) {
		init_completion(&trans_done);
		spin_unlock_irq(&rtsx->reg_lock);
		timeleft = wait_for_completion_interruptible_timeout(&trans_done, timeout * HZ / 1000);
		if (timeleft <= 0) {
			RTSX_DEBUGP(("Timeout (%s %d)\n", __FUNCTION__, __LINE__));
			RTSX_DEBUGP(("chip->int_reg = 0x%x\n", chip->int_reg));
			err = -ETIMEDOUT;
			goto out;
		}
	} else {
		spin_unlock_irq(&rtsx->reg_lock);
	}

	spin_lock_irq(&rtsx->reg_lock);
	if (rtsx->trans_result == TRANS_RESULT_FAIL) {
		err = -EIO;
	} else if (rtsx->trans_result == TRANS_RESULT_OK) {
		err = 0;
	}
	spin_unlock_irq(&rtsx->reg_lock);

out:
	rtsx->done = NULL;
	rtsx->trans_state = STATE_TRANS_NONE;
	dma_unmap_sg(&(rtsx->pci->dev), sg, num_sg, dma_dir);

	if (err < 0) {
		rtsx_stop_cmd(chip, card);
	}

	if (err == -ETIMEDOUT) {
		CATCH_TRIGGER1(chip);
	}

	return err;
}

/**
 * rtsx_transfer_buf - transfer data in linear buffer.
 * @chip: Realtek's card reader chip
 * @card: this command is relevant to card or not
 * @buf: data buffer
 * @len: buffer length
 * @dma_dir: transfer direction (DMA_FROM_DEVICE or DMA_TO_DEVICE)
 * @timeout: time out in millisecond
 *
 * Transfer data in linear buffer.
 */
static int rtsx_transfer_buf(struct rtsx_chip *chip, u8 card, void *buf, size_t len,
		enum dma_data_direction dma_dir, int timeout)
{
	struct rtsx_dev *rtsx = chip->rtsx;
	struct completion trans_done;
	dma_addr_t addr;
	u8 dir;
	int err = 0;
	u32 val = (1 << 31);
	long timeleft;

	if ((buf == NULL) || (len <= 0)) {
		return -EIO;
	}

	if (dma_dir == DMA_TO_DEVICE) {
		dir = HOST_TO_DEVICE;
	} else if (dma_dir == DMA_FROM_DEVICE) {
		dir = DEVICE_TO_HOST;
	} else {
		return -ENXIO;
	}

	addr = dma_map_single(&(rtsx->pci->dev), buf, len, dma_dir);
	if (!addr) {
		return -ENOMEM;
	}

	if (card == SD_CARD) {
		rtsx->check_card_cd = SD_EXIST;
	} else if (card == MS_CARD) {
		rtsx->check_card_cd = MS_EXIST;
	} else {
		rtsx->check_card_cd = 0;
	}

	val |= (u32)(dir & 0x01) << 29;
	val |= (u32)(len & 0x00FFFFFF);

	spin_lock_irq(&rtsx->reg_lock);

	rtsx->done = &trans_done;

	init_completion(&trans_done);

	rtsx->trans_state = STATE_TRANS_BUF;
	rtsx->trans_result = TRANS_NOT_READY;

	rtsx_writel(chip, RTSX_HDBAR, addr);
	rtsx_writel(chip, RTSX_HDBCTLR, val);

	spin_unlock_irq(&rtsx->reg_lock);

	timeleft = wait_for_completion_interruptible_timeout(&trans_done, timeout * HZ / 1000);
	if (timeleft <= 0) {
		RTSX_DEBUGP(("Timeout (%s %d)\n", __FUNCTION__, __LINE__));
		RTSX_DEBUGP(("chip->int_reg = 0x%x\n", chip->int_reg));
		err = -ETIMEDOUT;
		goto out;
	}

	spin_lock_irq(&rtsx->reg_lock);
	if (rtsx->trans_result == TRANS_RESULT_FAIL) {
		err = -EIO;
	} else if (rtsx->trans_result == TRANS_RESULT_OK) {
		err = 0;
	}
	spin_unlock_irq(&rtsx->reg_lock);

out:
	rtsx->done = NULL;
	rtsx->trans_state = STATE_TRANS_NONE;
	dma_unmap_single(&(rtsx->pci->dev), addr, len, dma_dir);

	if (err < 0) {
		rtsx_stop_cmd(chip, card);
	}

	if (err == -ETIMEDOUT) {
		CATCH_TRIGGER1(chip);
	}

	return err;
}

int rtsx_transfer_data_partial(struct rtsx_chip *chip, u8 card, void *buf, size_t len,
		int use_sg, unsigned int *index, unsigned int *offset,
		enum dma_data_direction dma_dir, int timeout)
{
	int err = 0;


	if (rtsx_chk_stat(chip, RTSX_STAT_ABORT)) {
		return -EIO;
	}

	if (use_sg) {
		err = rtsx_transfer_sglist_adma_partial(chip, card, (struct scatterlist *)buf,
				use_sg, index, offset, (int)len, dma_dir, timeout);
	} else {
		err = rtsx_transfer_buf(chip, card, buf, len, dma_dir, timeout);
	}

	if (err < 0) {
		if (RTSX_TST_DELINK(chip)) {
			RTSX_CLR_DELINK(chip);
			chip->need_reinit = SD_CARD | MS_CARD;
			rtsx_reinit_cards(chip, 1);
		}
	}

	return err;
}

int rtsx_transfer_data(struct rtsx_chip *chip, u8 card, void *buf, size_t len,
		int use_sg, enum dma_data_direction dma_dir, int timeout)
{
	int err = 0;

	RTSX_DEBUGP(("use_sg = %d\n", use_sg));


	if (rtsx_chk_stat(chip, RTSX_STAT_ABORT)) {
		return -EIO;
	}

	if (use_sg) {
		err = rtsx_transfer_sglist_adma(chip, card, (struct scatterlist *)buf,
				use_sg, dma_dir, timeout);
	} else {
		err = rtsx_transfer_buf(chip, card, buf, len, dma_dir, timeout);
	}

	if (err < 0) {
		if (RTSX_TST_DELINK(chip)) {
			RTSX_CLR_DELINK(chip);
			chip->need_reinit = SD_CARD | MS_CARD;
			rtsx_reinit_cards(chip, 1);
		}
	}

	return err;
}


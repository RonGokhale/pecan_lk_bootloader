/*
 * Copyright (C) 2008 The Android Open Source Project
 * All rights reserved.
 * Copyright (c) 2009, Code Aurora Forum. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <app.h>
#include <debug.h>
#include <lib/ptable.h>
#include <malloc.h>
#include <dev/flash.h>
#include <string.h>
#include <jtag.h>
#include <kernel/thread.h>
#include <smem.h>
#include "bootimg.h"

#define FLASH_PAGE_SIZE 2048
#define FLASH_PAGE_BITS 11

unsigned page_size = 0;
unsigned page_mask = 0;

static unsigned load_addr = 0xffffffff;

#define ROUND_TO_PAGE(x,y) (((x) + (y)) & (~(y)))

void acpu_clock_init(void);
void platform_uninit_timer(void);

int startswith(const char *str, const char *prefix)
{
    while(*prefix){
        if(*prefix++ != *str++) return 0;
    }
    return 1;
}

unsigned set_load_address(unsigned addr)
{
    if (load_addr != 0xffffffff)
    {
        /* Already assigned */
        return load_addr;
    }

    load_addr = addr;

#ifdef PLATFORM_MSM7X30
    /* For 7x30, override the destination RAM address based on memory type */
    /* so the image is loaded to the larger memory segment.                */
    struct smem_board_info_v4 board_info_v4;
    unsigned int board_info_len = 0;
    unsigned smem_status;
    char *build_type;
    unsigned format = 0;

    smem_status = smem_read_alloc_entry_offset(SMEM_BOARD_INFO_LOCATION,
					       &format, sizeof(format), 0);
    if(smem_status)
    {
      dprintf(CRITICAL, "ERROR: unable to read shared memory for offset entry\n");
    }

    if ((format == 3) || (format == 4))
    {
        if (format == 4)
	    board_info_len = sizeof(board_info_v4);
	else
	    board_info_len = sizeof(board_info_v4.board_info_v3);

        smem_status = smem_read_alloc_entry(SMEM_BOARD_INFO_LOCATION,
					&board_info_v4, board_info_len);
        if(smem_status)
        {
            dprintf(CRITICAL, "ERROR: unable to read shared memory for Hardware Platform\n");
        }
    }

    if(!smem_status)
    {
        build_type  = (char *)(board_info_v4.board_info_v3.build_id) + 8;
        if (*build_type == 'A')
        {
            /* LPDDR2 configuration */
            load_addr = 0x40000000;
        }
        else
        {
            /* LPDDR1 configuration */
            load_addr = 0x08000000;
        }
    }
#endif

    return load_addr;
}

/* XXX */
void verify_flash(struct ptentry *p, void *addr, unsigned len, int extra)
{
    uint32_t offset = 0;
    void *buf = malloc(FLASH_PAGE_SIZE + extra);
    int verify_extra = extra;
    if(verify_extra > 4)
        verify_extra = 16;
    while(len > 0) {
        flash_read_ext(p, extra, offset, buf, FLASH_PAGE_SIZE);
        if(memcmp(addr, buf, FLASH_PAGE_SIZE + verify_extra)) {
            dprintf(CRITICAL, "verify failed at 0x%08x\n", offset);
            jtag_fail("verify failed");
            return;
        }
        offset += FLASH_PAGE_SIZE;
        addr += FLASH_PAGE_SIZE;
        len -= FLASH_PAGE_SIZE;
        if(extra) {
            addr += extra;
            len -= extra;
        }
    }
    dprintf(INFO, "verify done %d extra bytes\n", verify_extra);
    jtag_okay("verify done");
}

void handle_flash(const char *name, unsigned addr, unsigned sz)
{
	struct ptentry *ptn;
	struct ptable *ptable;
        void *data = (void *) addr;
	unsigned extra = 0;

	ptable = flash_get_ptable();
	if (ptable == NULL) {
		jtag_fail("partition table doesn't exist");
		return;
	}

	ptn = ptable_find(ptable, name);
	if (ptn == NULL) {
		jtag_fail("unknown partition name");
		return;
	}

	if (!strcmp(ptn->name, "boot") || !strcmp(ptn->name, "recovery")) {
		if (memcmp((void *)data, BOOT_MAGIC, BOOT_MAGIC_SIZE)) {
			jtag_fail("image is not a boot image");
			return;
		}
	}

	if (!strcmp(ptn->name, "system") || !strcmp(ptn->name, "userdata"))
		extra = ((page_size >> 9) * 16);
	else
		sz = ROUND_TO_PAGE(sz, page_mask);

	/* Override load address if needed */
	data = (void *)set_load_address(addr);

	dprintf(INFO, "writing %d bytes to '%s'\n", sz, ptn->name);
	if (flash_write(ptn, extra, data, sz)) {
		jtag_fail("flash write failure");
		return;
	}
	dprintf(INFO, "partition '%s' updated\n", ptn->name);
        jtag_okay("Done");
        enter_critical_section();
        platform_uninit_timer();
        arch_disable_cache(UCACHE);
        arch_disable_mmu();
}

static unsigned char *tmpbuf = 0;


/*XXX*/
void handle_dump(const char *name, unsigned offset)
{
    struct ptentry *p;
    struct ptable *ptable;

    if(tmpbuf == 0) {
        tmpbuf = malloc(4096);
    }

    dprintf(INFO, "dump '%s' partition\n", name);

    ptable = flash_get_ptable();

    if (ptable == NULL) {
        jtag_fail("partition table doesn't exist");
        return;
    }

    p = ptable_find(ptable, name);

    if(p == 0) {
        jtag_fail("partition not found");
        return;
    } else {

#if 0
        /* XXX reimpl */
        if(flash_read_page(p->start * 64, tmpbuf, tmpbuf + 2048)){
            jtag_fail("flash_read() failed");
            return;
        }
#endif
        dprintf(INFO, "page %d data:\n", p->start * 64);
        hexdump(tmpbuf, 256);
        dprintf(INFO, "page %d extra:\n", p->start * 64);
        hexdump(tmpbuf, 16);
        jtag_okay("done");
        enter_critical_section();
        platform_uninit_timer();
        arch_disable_cache(UCACHE);
        arch_disable_mmu();
    }
}

void handle_query_load_address(unsigned addr)
{
    unsigned *return_addr = (unsigned *)addr;

    if (return_addr)
        *return_addr = set_load_address(addr);

    jtag_okay("done");
}

void handle_command(const char *cmd, unsigned a0, unsigned a1, unsigned a2)
{
    if(startswith(cmd,"flash:")){
        handle_flash(cmd + 6, a0, a1);
        return;
    }

    if(startswith(cmd,"dump:")){
        handle_dump(cmd + 5, a0);
        return;
    }

    if(startswith(cmd,"loadaddr:")){
        handle_query_load_address(a0);
        return;
    }

    jtag_fail("unknown command");
}

void nandwrite_init(void)
{
    page_size = flash_page_size();
    page_mask = page_size - 1;
    jtag_cmd_loop(handle_command);
}



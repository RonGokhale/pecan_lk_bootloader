/*
 * Copyright (c) 2009, Google Inc.
 * All rights reserved.
 * Copyright (c) 2009-2010, Code Aurora Forum. All rights reserved.
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
 *  * Neither the name of Google, Inc. nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
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

#include <debug.h>
#include <dev/keys.h>
#include <dev/gpio_keypad.h>
#include <lib/ptable.h>
#include <dev/flash.h>
#include <smem.h>

#define LINUX_MACHTYPE_7x30_SURF   1007016
#define LINUX_MACHTYPE_7x30_FFA    1007017
#define LINUX_MACHTYPE_7x30_FLUID  1007018
#define LINUX_MACHTYPE_8x55_SURF   2768
#define LINUX_MACHTYPE_8x55_FFA    2769

#define MSM8255_ID                 74
#define MSM8655_ID                 75
#define APQ8055_ID                 85

#define VARIABLE_LENGTH        0x10101010
#define DIFF_START_ADDR        0xF0F0F0F0
#define NUM_PAGES_PER_BLOCK    0x40

static struct ptable flash_ptable;
static int hw_platform_type = -1;

/* for these partitions, start will be offset by either what we get from
 * smem, or from the above offset if smem is not useful. Also, we should
 * probably have smem_ptable code populate our flash_ptable.
 *
 * When smem provides us with a full partition table, we can get rid of
 * this altogether.
 *
 */
static struct ptentry board_part_list[] = {
	{
		.start = 0,
		.length = 20  /* 5MB */,
		.name = "boot",
	},
	{
		.start = 20,
		.length = 480 /* 120MB */,
		.name = "system",
	},
	{
		.start = 500,
		.length = 120 /* 30MB */,
		.name = "cache",
	},
	{
		.start = 620,
		.length = 4, /* 1MB */
		.name = "misc",
	},
	{
		.start = 624,
		.length = VARIABLE_LENGTH,
		.name = "userdata",
	},
	{
		.start = DIFF_START_ADDR,
		.length = 12 /* 3MB */,
		.name = "persist",
	},
	{
		.start = DIFF_START_ADDR,
		.length = 20 /* 5MB */,
		.name = "recovery",
	},
};
static int num_parts = sizeof(board_part_list)/sizeof(struct ptentry);

void smem_ptable_init(void);
unsigned smem_get_apps_flash_start(void);

void keypad_init(void);

static int emmc_boot = -1;  /* set to uninitialized */
int target_is_emmc_boot(void);
static int platform_version = -1;
static int target_msm_id = -1;

void target_init(void)
{
	unsigned offset;
	struct flash_info *flash_info;
	unsigned total_num_of_blocks;
	bool  start_addr_changed = false;
	unsigned next_ptr_start_adr = 0;
	int i;

	dprintf(INFO, "target_init()\n");

#if (!ENABLE_NANDWRITE)
	keys_init();
	keypad_init();
#endif

	if (target_is_emmc_boot())
		return;

	ptable_init(&flash_ptable);
	smem_ptable_init();

	flash_init();
	flash_info = flash_get_info();
	ASSERT(flash_info);

	offset = smem_get_apps_flash_start();
	if (offset == 0xffffffff)
	        while(1);

	total_num_of_blocks = flash_info->num_blocks;

	for (i = 0; i < num_parts; i++) {
		struct ptentry *ptn = &board_part_list[i];
		unsigned len = ptn->length;

		if(len == VARIABLE_LENGTH)
		{
		        start_addr_changed = true;
			unsigned length_for_prt = 0;
			unsigned j;
			for (j = i+1; j < num_parts; j++)
			{
			        struct ptentry *temp_ptn = &board_part_list[j];
			        ASSERT(temp_ptn->length != VARIABLE_LENGTH);
			        length_for_prt += temp_ptn->length;
			}
		        len = (total_num_of_blocks - 1) - (offset + ptn->start + length_for_prt);
			ASSERT(len >= 0);
		        next_ptr_start_adr = ptn->start + len;
		}
		if((ptn->start == DIFF_START_ADDR) && (start_addr_changed))
		{
		        ASSERT(next_ptr_start_adr);
			ptn->start = next_ptr_start_adr;
			next_ptr_start_adr = ptn->start + ptn->length;
		}
		ptable_add(&flash_ptable, ptn->name, offset + ptn->start,
			   len, ptn->flags, TYPE_APPS_PARTITION, PERM_WRITEABLE);
	}

	smem_add_modem_partitions(&flash_ptable);

	ptable_dump(&flash_ptable);
	flash_set_ptable(&flash_ptable);
}

int target_platform_version(void)
{
    return platform_version;
}

int target_is_msm8x55(void)
{
    if ((target_msm_id == MSM8255_ID) ||
	          (target_msm_id == MSM8655_ID) ||
	          (target_msm_id == APQ8055_ID))
        return 1;
    else
        return 0;
}

unsigned board_machtype(void)
{
    struct smem_board_info_v4 board_info_v4;
    unsigned int board_info_len = 0;
    enum platform platform_type = 0;
    unsigned smem_status;
    unsigned format = 0;
    if(hw_platform_type != -1)
        return hw_platform_type;

    smem_status = smem_read_alloc_entry_offset(SMEM_BOARD_INFO_LOCATION,
					       &format, sizeof(format), 0);
    if(!smem_status)
    {
	if ((format == 3) || (format == 4))
	{
	    if (format == 4)
		board_info_len = sizeof(board_info_v4);
	    else
		board_info_len = sizeof(board_info_v4.board_info_v3);

	    smem_status = smem_read_alloc_entry(SMEM_BOARD_INFO_LOCATION,
					    &board_info_v4, board_info_len);
	    if(!smem_status)
	    {
		if(format == 4)
		    platform_version = board_info_v4.platform_version;

		platform_type = board_info_v4.board_info_v3.hw_platform;
		target_msm_id = board_info_v4.board_info_v3.msm_id;
		switch (platform_type)
		{
		case HW_PLATFORM_SURF:
		    hw_platform_type = ((target_is_msm8x55()) ?
				      LINUX_MACHTYPE_8x55_SURF : LINUX_MACHTYPE_7x30_SURF);    break;
		case HW_PLATFORM_FFA:
		    hw_platform_type = ((target_is_msm8x55()) ?
				      LINUX_MACHTYPE_8x55_FFA : LINUX_MACHTYPE_7x30_FFA);      break;
		case HW_PLATFORM_FLUID:
		    hw_platform_type = LINUX_MACHTYPE_7x30_FLUID;                              break;
		default:
		    hw_platform_type = ((target_is_msm8x55()) ?
				      LINUX_MACHTYPE_8x55_SURF : LINUX_MACHTYPE_7x30_SURF);    break;
		}
		return hw_platform_type;
	    }
	}
    }
    hw_platform_type = LINUX_MACHTYPE_7x30_SURF;
    return hw_platform_type;
}

void reboot_device(unsigned reboot_reason)
{
    reboot(reboot_reason);
}

unsigned check_reboot_mode(void)
{
    unsigned mode[2] = {0, 0};
    unsigned int mode_len = sizeof(mode);
    unsigned smem_status;

    smem_status = smem_read_alloc_entry(SMEM_APPS_BOOT_MODE,
					&mode, mode_len );
    if(smem_status)
    {
      dprintf(CRITICAL, "ERROR: unable to read shared memory for reboot mode\n");
      return 0;
    }
    return mode[0];
}

void target_battery_charging_enable(unsigned enable, unsigned disconnect)
{
}

/*
 * Copyright (c) 2009, Google Inc.
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

#include <dev/keys.h>
#include <dev/gpio_keypad.h>

#define ARRAY_SIZE(x) (sizeof(x)/sizeof((x)[0]))

/* don't turn this on without updating the ffa support */
#define SCAN_FUNCTION_KEYS 0

static unsigned int halibut_col_gpios[] = { 34, 35, 36 };
static unsigned int halibut_row_gpios[] = { 37, 38, 39 };

#define KEYMAP_INDEX(row, col) ((row)*ARRAY_SIZE(halibut_col_gpios) + (col))

static const unsigned short halibut_keymap[ARRAY_SIZE(halibut_col_gpios) * ARRAY_SIZE(halibut_row_gpios)] = {
	[KEYMAP_INDEX(0, 0)] = KEY_END,
	[KEYMAP_INDEX(0, 1)] = KEY_VOLUMEUP,
	[KEYMAP_INDEX(1, 0)] = KEY_SEND,
	[KEYMAP_INDEX(1, 1)] = KEY_VOLUMEDOWN,
};

static struct gpio_keypad_info halibut_keypad_info = {
	.keymap		= halibut_keymap,
	.output_gpios	= halibut_col_gpios,
	.input_gpios	= halibut_row_gpios,
	.noutputs	= ARRAY_SIZE(halibut_col_gpios),
	.ninputs	= ARRAY_SIZE(halibut_row_gpios),
	.settle_time	= 5 /* msec */,
	.poll_time	= 20 /* msec */,
	.flags		= GPIOKPF_DRIVE_INACTIVE,
};

void keypad_init(void)
{
	gpio_keypad_init(&halibut_keypad_info);
}

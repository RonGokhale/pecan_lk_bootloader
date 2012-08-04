/*
 * Copyright (c) 2008, Google Inc.
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

#ifndef _PLATFORM_MSM7X30_IOMAP_H_
#define _PLATFORM_MSM7X30_IOMAP_H_

#define MSM_UART1_BASE 0xACA00000
#define MSM_UART2_BASE 0xACB00000
#define MSM_UART3_BASE 0xACC00000

#define MSM_VIC_BASE	0xC0080000
#define MSM_TMR_BASE 	0xC0100000
#define MSM_GPT_BASE    (MSM_TMR_BASE + 0x04)
#define MSM_CSR_BASE    0xC0100000
#define MSM_GCC_BASE 	0xC0182000

#define MSM_SDC1_BASE   0xA0400000
#define MSM_SDC2_BASE   0xA0500000
#define MSM_SDC3_BASE   0xA3000000
#define MSM_SDC4_BASE   0xA3100000

#define MSM_SHARED_BASE      0x00100000

#define MSM_CLK_CTL_BASE        0xAB800000
#define MSM_CLK_CTL_SH2_BASE    0xABA01000
#define SCSS_CLK_CTL            0xC0101004
#define SCSS_CLK_SEL            0xC0101008

#define MSM_USB_BASE			0xA3600000
#define SH2_USBH_MD_REG			0xABA012BC
#define SH2_USBH_NS_REG			0xABA012C0
#define SH2_GLBL_CLK_ENA_SC		0xABA013BC

#define MSM_SAW_BASE            0xC0102000
#endif

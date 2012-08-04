/*
 * Copyright (c) 2009, Code Aurora Forum. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Code Aurora nor
 *     the names of its contributors may be used to endorse or promote
 *     products derived from this software without specific prior written
 *     permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdint.h>
#include <kernel/thread.h>
#include <platform/iomap.h>
#include <reg.h>

#define REG_BASE(off)           (MSM_CLK_CTL_BASE + (off))
#define REG(off)                (MSM_CLK_CTL_SH2_BASE + (off))

#define PLL_ENA_REG             REG(0x0264)
#define PLL2_STATUS_BASE_REG    REG_BASE(0x0350)

#define SH2_OWN_ROW2_BASE_REG	REG_BASE(0x0424)

/* Macros to select PLL2 with divide by 1 */
#define ACPU_SRC_SEL       3
#define ACPU_SRC_DIV       0

#define BIT(n)  (1 << (n))
#define VREG_CONFIG (BIT(7) | BIT(6)) /* Enable VREG, pull-down if disabled. */
#define VREG_DATA   (VREG_CONFIG | (VREF_SEL << 5))
#define VREF_SEL    1 /* 0: 0.625V (50mV step), 1: 0.3125V (25mV step). */
#define V_STEP      (25 * (2 - VREF_SEL)) /* Minimum voltage step size. */
#define MV(mv)          ((mv) / (!((mv) % V_STEP)))
/* mv = (750mV + (raw * 25mV)) * (2 - VREF_SEL) */
#define VDD_RAW(mv)     (((MV(mv) / V_STEP) - 30) | VREG_DATA)

void spm_init(void)
{
    writel(0x05, MSM_SAW_BASE + 0x10); /* MSM_SPM_REG_SAW_CFG */
    writel(0x18, MSM_SAW_BASE + 0x14); /* MSM_SPM_REG_SAW_SPM_CTL */
    writel(0x00006666, MSM_SAW_BASE + 0x18); /* MSM_SPM_REG_SAW_SPM_SLP_TMR_DLY */
    writel(0xFF000666, MSM_SAW_BASE + 0x1C); /* MSM_SPM_REG_SAW_SPM_WAKE_TMR_DLY */

    writel(0x01, MSM_SAW_BASE + 0x24); /* MSM_SPM_REG_SAW_SLP_CLK_EN */
    writel(0x03, MSM_SAW_BASE + 0x28); /* MSM_SPM_REG_SAW_SLP_HSFS_PRECLMP_EN */
    writel(0x00, MSM_SAW_BASE + 0x2C); /* MSM_SPM_REG_SAW_SLP_HSFS_POSTCLMP_EN */

    writel(0x01, MSM_SAW_BASE + 0x30); /* MSM_SPM_REG_SAW_SLP_CLMP_EN */
    writel(0x00, MSM_SAW_BASE + 0x34); /* MSM_SPM_REG_SAW_SLP_RST_EN */
    writel(0x00, MSM_SAW_BASE + 0x38); /* MSM_SPM_REG_SAW_SPM_MPM_CFG */
}

/* Configures msmc2 voltage. vlevel is in mV */
void msmc2_config(unsigned vlevel)
{
    unsigned val;
    val = readl(MSM_SAW_BASE + 0x08); /* MSM_SPM_REG_SAW_VCTL */
    val &= ~0xFF;
    val |= VDD_RAW(vlevel);
    writel(val, MSM_SAW_BASE + 0x08); /* MSM_SPM_REG_SAW_VCTL */

    /* Wait for PMIC state to return to idle and for VDD to stabilize */
    while(((readl(MSM_SAW_BASE + 0x0C) >> 0x20) & 0x3) != 0);
    udelay(160);
}

void enable_pll(unsigned num)
{
    unsigned reg_val;
    reg_val = readl(PLL_ENA_REG);
    reg_val |= (1 << num);
    writel(reg_val, PLL_ENA_REG);
    /* Wait until PLL is enabled */
    while ((readl(PLL2_STATUS_BASE_REG) & (1 << 16)) == 0);
}

void acpu_clock_init(void)
{
    unsigned reg_clksel, reg_clkctl, src_sel;
    /* Fixing msmc2 voltage */
    spm_init();
    msmc2_config(1200); /* Setting msmc2 1.2V */

    /* Enable pll 2 */
    enable_pll(2);

    reg_clksel = readl(SCSS_CLK_SEL);

    /* CLK_SEL_SRC1NO */
    src_sel = reg_clksel & 1;

    /* Program clock source and divider. */
    reg_clkctl = readl(SCSS_CLK_CTL);
    reg_clkctl &= ~(0xFF << (8 * src_sel));
    reg_clkctl |= ACPU_SRC_SEL<< (4 + 8 * src_sel);
    reg_clkctl |= ACPU_SRC_DIV<< (0 + 8 * src_sel);
    writel(reg_clkctl, SCSS_CLK_CTL);

    /* Toggle clock source. */
    reg_clksel ^= 1;

    /* Program clock source selection. */
    writel(reg_clksel, SCSS_CLK_SEL);
}

void hsusb_clock_init(void)
{
	int val = 0;
	unsigned sh2_own_row2;
	unsigned sh2_own_row2_hsusb_mask = (1 << 11);

	sh2_own_row2 = readl(SH2_OWN_ROW2_BASE_REG);
	if(sh2_own_row2 & sh2_own_row2_hsusb_mask)
	{
		/* USB local clock control enabled */
	    /* Set value in MD register */
	    val = 0x5DF;
	    writel(val, SH2_USBH_MD_REG);

	    /* Set value in NS register */
	    val = 1 << 8;
	    val = val | readl(SH2_USBH_NS_REG);
	    writel(val, SH2_USBH_NS_REG);

	    val = 1 << 11;
	    val = val | readl(SH2_USBH_NS_REG);
	    writel(val, SH2_USBH_NS_REG);

	    val = 1 << 9;
	    val = val | readl(SH2_USBH_NS_REG);
	    writel(val, SH2_USBH_NS_REG);

	    val = 1 << 13;
	    val = val | readl(SH2_USBH_NS_REG);
	    writel(val, SH2_USBH_NS_REG);

	    /* Enable USBH_P_CLK */
	    val = 1 << 25;
	    val = val | readl(SH2_GLBL_CLK_ENA_SC);
	    writel(val, SH2_GLBL_CLK_ENA_SC);
	}
	else
	{
		/* USB local clock control not enabled; use proc comm */
		usb_clock_init();
	}
}

void adm_enable_clock(void)
{
	unsigned int val=0;

	/* Enable ADM_CLK */
	val = 1 << 5;
	val = val | readl(SH2_GLBL_CLK_ENA_SC);
	writel(val, SH2_GLBL_CLK_ENA_SC);
}

/*
 * Jz4760 common routines
 *
 *  Copyright (c) 2006
 *  Ingenic Semiconductor, <jlwei@ingenic.cn>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <config.h>
#include <asm/mipsregs.h>

#if defined(CONFIG_JZ4760)
#include <common.h>
#include <command.h>
#include <asm/jz4760.h>

//#define DEBUG
#undef DEBUG
#ifdef DEBUG
#define dprintf(fmt,args...)	printf(fmt, ##args)
#else
#define dprintf(fmt,args...)
#endif
extern void board_early_init(void);
static int ddr_dma_test(void);

void jzmemset(void *dest,int ch,int len)
{
	unsigned int *d = (unsigned int *)dest;
	int i;
	int wd;

	wd = (ch << 24) | (ch << 16) | (ch << 8) | ch;

	for(i = 0;i < len / 32;i++)
	{
		*d++ = wd;
		*d++ = wd;
		*d++ = wd;
		*d++ = wd;
		*d++ = wd;
		*d++ = wd;
		*d++ = wd;
		*d++ = wd;
	}
}

/* PLL output clock = EXTAL * NF / (NR * NO)
 *
 * NF = FD + 2, NR = RD + 2
 * NO = 1 (if OD = 0), NO = 2 (if OD = 1 or 2), NO = 4 (if OD = 3)
 */
void pll_init(void)
{
	register unsigned int cfcr, plcr1;
	int n2FR[33] = {
		0, 0, 1, 2, 3, 0, 4, 0, 5, 0, 0, 0, 6, 0, 0, 0,
		7, 0, 0, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 0,
		9
	};

        /** divisors, 
	 *  for jz4760 ,I:H0:P:M:H1.
         */
	int div[5] = {1, 3, 3, 3, 3}; 
	int nf, pllout2;

	cfcr = 	CPM_CPCCR_PCS |
		(n2FR[div[0]] << CPM_CPCCR_CDIV_BIT) | 
		(n2FR[div[1]] << CPM_CPCCR_HDIV_BIT) | 
		(n2FR[div[2]] << CPM_CPCCR_PDIV_BIT) |
		(n2FR[div[3]] << CPM_CPCCR_MDIV_BIT) |
		(n2FR[div[4]] << CPM_CPCCR_H1DIV_BIT);

	if (CFG_EXTAL > 16000000)
		cfcr |= CPM_CPCCR_ECS;
	else
		cfcr &= ~CPM_CPCCR_ECS;

	pllout2 = (cfcr & CPM_CPCCR_PCS) ? CFG_CPU_SPEED : (CFG_CPU_SPEED / 2);

	nf = CFG_CPU_SPEED * 2 / CFG_EXTAL;
	plcr1 = ((nf - 2) << CPM_CPPCR_PLLM_BIT) | /* FD */
		(0 << CPM_CPPCR_PLLN_BIT) |	/* RD=0, NR=2 */
		(0 << CPM_CPPCR_PLLOD_BIT) |    /* OD=0, NO=1 */
		(0x20 << CPM_CPPCR_PLLST_BIT) | /* PLL stable time */
		CPM_CPPCR_PLLEN;                /* enable PLL */          

	/* init PLL */
	REG_CPM_CPCCR = cfcr;
	REG_CPM_CPPCR = plcr1;
}

void pll_add_test(int new_freq)
{
	register unsigned int cfcr, plcr1;
	int n2FR[33] = {
		0, 0, 1, 2, 3, 0, 4, 0, 5, 0, 0, 0, 6, 0, 0, 0,
		7, 0, 0, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 0,
		9
	};
	int div[5] = {1, 6, 6, 6, 6}; /* divisors of I:S:P:M:L */
	int nf, pllout2;

	cfcr = 	(n2FR[div[0]] << CPM_CPCCR_CDIV_BIT) | 
		(n2FR[div[1]] << CPM_CPCCR_HDIV_BIT) | 
		(n2FR[div[2]] << CPM_CPCCR_PDIV_BIT) |
		(n2FR[div[3]] << CPM_CPCCR_MDIV_BIT) |
		(n2FR[div[4]] << CPM_CPCCR_H1DIV_BIT);

	if (CFG_EXTAL > 16000000)
		cfcr |= CPM_CPCCR_ECS;
	else
		cfcr &= ~CPM_CPCCR_ECS;

	pllout2 = (cfcr & CPM_CPCCR_PCS) ? new_freq : (new_freq / 2);

	//nf = new_freq * 2 / CFG_EXTAL;
	nf = new_freq / 1000000; //step length is 1M
	plcr1 = ((nf - 2) << CPM_CPPCR_PLLM_BIT) | /* FD */
		(22 << CPM_CPPCR_PLLN_BIT) |	/* RD=0, NR=2 */
		(0 << CPM_CPPCR_PLLOD_BIT) |    /* OD=0, NO=1 */
		(0x20 << CPM_CPPCR_PLLST_BIT) | /* PLL stable time */
		CPM_CPPCR_PLLEN;                /* enable PLL */          

	/* init PLL */
	REG_CPM_CPCCR = cfcr;
	REG_CPM_CPPCR = plcr1;
}

void calc_clocks_add_test(void)
{
	DECLARE_GLOBAL_DATA_PTR;

#ifndef CONFIG_FPGA
	unsigned int pllout;
	unsigned int div[10] = {1, 2, 3, 4, 6, 8, 12, 16, 24, 32};
	pllout = __cpm_get_pllout();

	gd->cpu_clk = pllout / div[__cpm_get_cdiv()];
	gd->sys_clk = pllout / div[__cpm_get_hdiv()];
	gd->per_clk = pllout / div[__cpm_get_pdiv()];
	gd->mem_clk = pllout / div[__cpm_get_mdiv()];
	gd->dev_clk = CFG_EXTAL;
#else
	gd->cpu_clk = gd->sys_clk = gd->per_clk = 
		gd->mem_clk = gd->dev_clk = CFG_EXTAL;
#endif
}


//----------------------------------------------------------------------
// U-Boot common routines
#ifndef CONFIG_DDRC
long int initdram(int board_type)
{
	u32 dmcr;
	u32 rows, cols, dw, banks;
	ulong size;

	dmcr = REG_EMC_DMCR;
	rows = 11 + ((dmcr & EMC_DMCR_RA_MASK) >> EMC_DMCR_RA_BIT);
	cols = 8 + ((dmcr & EMC_DMCR_CA_MASK) >> EMC_DMCR_CA_BIT);
	dw = (dmcr & EMC_DMCR_BW) ? 2 : 4;
	banks = (dmcr & EMC_DMCR_BA) ? 4 : 2;

	size = (1 << (rows + cols)) * dw * banks;
	size *= CONFIG_NR_DRAM_BANKS;

	return size;
}
#else
long int initdram(int board_type)
{
	u32 ddr_cfg;
	u32 rows, cols, dw, banks;
	ulong size;
	ddr_cfg = REG_DDRC_CFG;
	rows = 12 + ((ddr_cfg & DDRC_CFG_ROW_MASK) >> DDRC_CFG_ROW_BIT);
	cols = 8 + ((ddr_cfg & DDRC_CFG_COL_MASK) >> DDRC_CFG_COL_BIT);
	
	dw = (ddr_cfg & DDRC_CFG_DW) ? 4 : 2;
	banks = (ddr_cfg & DDRC_CFG_BA) ? 8 : 4;
	
	size = (1 << (rows + cols)) * dw * banks;
	size *= (DDR_CS1EN + DDR_CS0EN);

	return size;
}
#endif
#ifdef DEBUG
static void ddrc_regs_print(void)
{
	dprintf("\nDDRC REGS:\n");
	dprintf("REG_DDRC_ST \t\t= 0x%08x\n", REG_DDRC_ST);
	dprintf("REG_DDRC_CFG \t\t= 0x%08x\n", REG_DDRC_CFG);
	dprintf("REG_DDRC_CTRL \t\t= 0x%08x\n", REG_DDRC_CTRL);
	dprintf("REG_DDRC_LMR \t\t= 0x%08x\n", REG_DDRC_LMR);
	dprintf("REG_DDRC_TIMING1 \t= 0x%08x\n", REG_DDRC_TIMING1);
	dprintf("REG_DDRC_TIMING2 \t= 0x%08x\n", REG_DDRC_TIMING2);
	dprintf("REG_DDRC_REFCNT \t\t= 0x%08x\n", REG_DDRC_REFCNT);
	dprintf("REG_DDRC_DQS \t\t= 0x%08x\n", REG_DDRC_DQS);
	dprintf("REG_DDRC_DQS_ADJ \t= 0x%08x\n", REG_DDRC_DQS_ADJ);
	dprintf("REG_DDRC_MMAP0 \t\t= 0x%08x\n", REG_DDRC_MMAP0);
	dprintf("REG_DDRC_MMAP1 \t\t= 0x%08x\n", REG_DDRC_MMAP1);
	dprintf("REG_DDRC_MDELAY \t\t= 0x%08x\n", REG_DDRC_MDELAY);
}
#ifndef CONFIG_DDRC
static void sdmr_regs_print(void)
{
	dprintf("\nSDMR REGS:-----------------------------\n");
	dprintf("REG_EMC_BCR \t\t= 0x%08x\n", REG_EMC_BCR);
	dprintf("REG_EMC_DMCR \t\t= 0x%08x\n", REG_EMC_DMCR);
	dprintf("REG_EMC_RTCSR \t\t= 0x%08x\n", REG_EMC_RTCSR);
	dprintf("REG_EMC_RTCNT \t\t= 0x%08x\n", REG_EMC_RTCNT);
	dprintf("REG_EMC_RTCOR \t\t= 0x%08x\n", REG_EMC_RTCOR);
	dprintf("REG_EMC_DMAR0 \t\t= 0x%08x\n", REG_EMC_DMAR0);
	dprintf("REG_EMC_DMAR1 \t\t= 0x%08x\n", REG_EMC_DMAR1);
	dprintf("---------------------------------------\n");
}
#endif

#define MEM_BASE  0xa0000000		/*un-cached*/
#define DDR_BANK_NUM	4
static void mem_test(void)
{
	volatile unsigned int *ptr;

	long int memsize;
	memsize = initdram(0);///1024;
	/*write data to bank0~3*/
	dprintf("Write data to SDRAM\n");
	for (ptr = (volatile unsigned int *)(MEM_BASE); (unsigned int)ptr < MEM_BASE + memsize; ptr++) {
		*ptr = (unsigned int)ptr;
		if (*ptr != (unsigned int)ptr) {
			dprintf("\nERROR: ");
			dprintf("--0x%08x\t", (unsigned int)ptr);
			dprintf("--0x%08x\n", *ptr);
		}
	}
	dprintf("0x%08x\n", (unsigned int)ptr);
	for (ptr = (volatile unsigned int *)(MEM_BASE); (unsigned int)ptr < MEM_BASE + memsize; ptr++) {
		if (*ptr != (unsigned int)ptr) {
			dprintf("\n SDRAM ERROR\n");
			dprintf("0x%08x\t", (unsigned int)ptr);
			dprintf("0x%08x\n", *ptr);
		}
	}
	dprintf("0x%08x\n", (unsigned int)ptr);
	dprintf("Read and compare finish\n");
	/*mobile test finish*/
}
void dump_jz_dma_channel(unsigned int dmanr)
{

	if (dmanr > MAX_DMA_NUM)
		return;

	dprintf("DMA%d Registers:\n", dmanr);
	dprintf("  DMACR  = 0x%08x\n", REG_DMAC_DMACR(dmanr/HALF_DMA_NUM));
	dprintf("  DSAR   = 0x%08x\n", REG_DMAC_DSAR(dmanr));
	dprintf("  DTAR   = 0x%08x\n", REG_DMAC_DTAR(dmanr));
	dprintf("  DTCR   = 0x%08x\n", REG_DMAC_DTCR(dmanr));
	dprintf("  DRSR   = 0x%08x\n", REG_DMAC_DRSR(dmanr));
	dprintf("  DCCSR  = 0x%08x\n", REG_DMAC_DCCSR(dmanr));
	dprintf("  DCMD  = 0x%08x\n", REG_DMAC_DCMD(dmanr));
	dprintf("  DDA  = 0x%08x\n", REG_DMAC_DDA(dmanr));
	dprintf("  DMADBR = 0x%08x\n", REG_DMAC_DMADBR(dmanr/HALF_DMA_NUM));
}
#define DDR_16M (16 * 1024 * 1024)
static void map_ddr_memory(unsigned long vbase, unsigned long pbase, unsigned long meg) {
	int i, entrys, pfn0, pfn1, vadd, padd;
	unsigned long entrylo0, entrylo1, entryhi, pagemask;

	entrys = meg / 16;
	pagemask = PM_16M;
	
	for (i = 0; i < entrys; i+=2) {
		vadd = vbase + i * DDR_16M;
		padd = pbase + i * DDR_16M;
		entryhi = vadd;
		pfn0 = (padd >> 6) | (2 << 3);
		pfn1 = (padd + DDR_16M) >> 6 | (2 << 3);
		entrylo0 = (pfn0 | 0x6) & 0x3ffffff;
		entrylo1 = (pfn1 | 0x6) & 0x3ffffff;
		add_wired_entry(entrylo0, entrylo1, entryhi, pagemask);
	}
}

#endif /* DEBUG */

static int dma_check_result(void *src, void *dst, int size)
{
	unsigned int addr1, addr2, i, err = 0;

	addr1 = (unsigned int)src;
	addr2 = (unsigned int)dst;
	dprintf("src 0x%08x,  dst 0x%08x, size 0x%08x\n", addr1, addr2, size);
	for (i = 0; i < size; i += 4) {
		if ((*(volatile unsigned int *)addr1 != *(volatile unsigned int *)addr2)
		    || (*(volatile unsigned int *)addr1 != (i/4*0x11111111))) {
#ifdef DEBUG
			err++;
			if (err < 10)
				dprintf("wrong data at 0x%08x: data 0x%08x src 0x%08x  dst 0x%08x\n", addr2, i/4*0x11111111, *(volatile unsigned int *)addr1, *(volatile unsigned int *)addr2);
#else
			return 1;
#endif
		}

		addr1 += 4;
		addr2 += 4;
	}
	return err;
}

void dma_nodesc_test(int dma_chan, int dma_src_addr, int dma_dst_addr, int size)
{
	int dma_src_phys_addr, dma_dst_phys_addr;

	dprintf("DMA channel = %d\n", dma_chan);
	/* Allocate DMA buffers */
	dma_src_phys_addr = dma_src_addr & ~0xa0000000;
	dma_dst_phys_addr = dma_dst_addr & ~0xa0000000;

	dprintf("DMA addresses: src-- 0x%08x 0x%08x, dst-- 0x%08x 0x%08x\n",
	       dma_src_addr, dma_src_phys_addr, dma_dst_addr, dma_dst_phys_addr);

	/* Init DMA module */
	dprintf("Starting DMA\n");
	REG_DMAC_DCCSR(dma_chan) = 0;
	REG_DMAC_DRSR(dma_chan) = DMAC_DRSR_RS_AUTO;
	REG_DMAC_DSAR(dma_chan) = dma_src_phys_addr;
	REG_DMAC_DTAR(dma_chan) = dma_dst_phys_addr;
	REG_DMAC_DTCR(dma_chan) = size/32;
	REG_DMAC_DCMD(dma_chan) = DMAC_DCMD_SAI | DMAC_DCMD_DAI | DMAC_DCMD_SWDH_32 | DMAC_DCMD_DWDH_32 | DMAC_DCMD_DS_32BYTE | DMAC_DCMD_TIE;
	REG_DMAC_DCCSR(dma_chan) = DMAC_DCCSR_NDES | DMAC_DCCSR_EN;
}

#define DDR_DMA_BASE  (0xa0000000)		/*un-cached*/
#define DMA_CHANNEL0_EN
#define DMA_CHANNEL1_EN
static int ddr_dma_test(void) {
	int i, err = 0, banks;
	unsigned int addr, DDR_DMA0_SRC, DDR_DMA0_DST, DDR_DMA1_SRC, DDR_DMA1_DST;
	volatile unsigned int tmp;
	register unsigned int cpu_clk;
	long int memsize, banksize, testsize;
#ifndef CONFIG_DDRC
	banks = (SDRAM_BANK4 ? 4 : 2) *(CONFIG_NR_DRAM_BANKS);
#else
	banks = (DDR_BANK8 ? 8 : 4) *(DDR_CS0EN + DDR_CS1EN);
#endif
	memsize = initdram(0);
	dprintf("memsize = 0x%08x\n", memsize);
	banksize = memsize/banks;
	testsize = 4096;
#if 0
	DDR_DMA0_SRC = DDR_DMA_BASE + banksize*0;
	DDR_DMA0_DST = DDR_DMA_BASE + banksize*(banks - 2) + testsize;
	DDR_DMA1_SRC = DDR_DMA_BASE + banksize*(banks - 1) + testsize;
	DDR_DMA1_DST = DDR_DMA_BASE + banksize*1;
#else
	DDR_DMA0_SRC = DDR_DMA_BASE + banksize*0;
	DDR_DMA0_DST = DDR_DMA_BASE + banksize*0 + testsize;
	DDR_DMA1_SRC = DDR_DMA_BASE + banksize*(banks - 1) + testsize*2;
	DDR_DMA1_DST = DDR_DMA_BASE + banksize*(banks - 1) + testsize*3;
#endif

	cpu_clk = CFG_CPU_SPEED;

#ifdef DMA_CHANNEL0_EN
	addr = DDR_DMA0_SRC;
	dprintf("DDR_DMA0_SRC =  0x%08x, DDR_DMA0_DST = 0x%08x\n", DDR_DMA0_SRC, DDR_DMA0_DST);
	for (i = 0; i < testsize; i += 4) {
		*(volatile unsigned int *)addr = (i/4*0x11111111);
		addr += 4;
	}
#endif
#ifdef DMA_CHANNEL1_EN
	addr = DDR_DMA1_SRC;
	for (i = 0; i < testsize; i += 4) {
		*(volatile unsigned int *)addr = (i/4*0x11111111);
		addr += 4;
	}
#endif
	dprintf("\nWrite finish\n");
	REG_DMAC_DMACR(0) = 0;
//	REG_DMAC_DMACR(1) = 0;
	/* Init target buffer */
#ifdef DMA_CHANNEL0_EN
	jzmemset((void *)DDR_DMA0_DST, 0, testsize);
	dma_nodesc_test(0, DDR_DMA0_SRC, DDR_DMA0_DST, testsize);
#endif
#ifdef DMA_CHANNEL1_EN
	jzmemset((void *)DDR_DMA1_DST, 0, testsize);
	dma_nodesc_test(1, DDR_DMA1_SRC, DDR_DMA1_DST, testsize);
#endif

	REG_DMAC_DMACR(0) = DMAC_DMACR_DMAE; /* global DMA enable bit */
	while(REG_DMAC_DTCR(0) || REG_DMAC_DTCR(1));

	tmp = (cpu_clk / 1000000) * 1;
	while (tmp--);
#ifdef DMA_CHANNEL0_EN
	err = dma_check_result((void *)DDR_DMA0_SRC, (void *)DDR_DMA0_DST, testsize);
	REG_DMAC_DCCSR(0) &= ~DMAC_DCCSR_EN;  /* disable DMA */
	if (err != 0) {
#ifdef DMA_CHANNEL1_EN
		REG_DMAC_DCCSR(1) &= ~DMAC_DCCSR_EN;  /* disable DMA */
#endif
		return err;
	}

#endif
#ifdef DMA_CHANNEL1_EN
	err += dma_check_result((void *)DDR_DMA1_SRC, (void *)DDR_DMA1_DST, testsize);
	REG_DMAC_DCCSR(1) &= ~DMAC_DCCSR_EN;  /* disable DMA */
#endif
	return err;
}

struct ddr_delay_sel_t {
	int msel;
	int hl;
};
#ifndef CONFIG_DDRC
void sdram_add_test(int new_freq)
{
	register unsigned int dmcr, sdmode, tmp, cpu_clk, mem_clk, ns;

	unsigned int cas_latency_sdmr[2] = {
		EMC_SDMR_CAS_2,
		EMC_SDMR_CAS_3,
	};

	unsigned int cas_latency_dmcr[2] = {
		1 << EMC_DMCR_TCL_BIT,	/* CAS latency is 2 */
		2 << EMC_DMCR_TCL_BIT	/* CAS latency is 3 */
	};

	int div[] = {1, 2, 3, 4, 6, 8, 12, 16, 24, 32};

	cpu_clk = new_freq;
	mem_clk = cpu_clk * div[__cpm_get_cdiv()] / div[__cpm_get_mdiv()];

	REG_EMC_RTCSR = EMC_RTCSR_CKS_DISABLE;
	REG_EMC_RTCOR = 0;
	REG_EMC_RTCNT = 0;

	/* Basic DMCR register value. */
	dmcr = ((SDRAM_ROW-11)<<EMC_DMCR_RA_BIT) |
		((SDRAM_COL-8)<<EMC_DMCR_CA_BIT) |
		(SDRAM_BANK4<<EMC_DMCR_BA_BIT) |
		(SDRAM_BW16<<EMC_DMCR_BW_BIT) |
		EMC_DMCR_EPIN |
		cas_latency_dmcr[((SDRAM_CASL == 3) ? 1 : 0)];

	/* SDRAM timimg parameters */
	ns = 1000000000 / mem_clk;

#if 0
	tmp = SDRAM_TRAS/ns;
	if (tmp < 4) tmp = 4;
	if (tmp > 11) tmp = 11;
	dmcr |= ((tmp-4) << EMC_DMCR_TRAS_BIT);

	tmp = SDRAM_RCD/ns;
	if (tmp > 3) tmp = 3;
	dmcr |= (tmp << EMC_DMCR_RCD_BIT);

	tmp = SDRAM_TPC/ns;
	if (tmp > 7) tmp = 7;
	dmcr |= (tmp << EMC_DMCR_TPC_BIT);

	tmp = SDRAM_TRWL/ns;
	if (tmp > 3) tmp = 3;
	dmcr |= (tmp << EMC_DMCR_TRWL_BIT);

	tmp = (SDRAM_TRAS + SDRAM_TPC)/ns;
	if (tmp > 14) tmp = 14;
	dmcr |= (((tmp + 1) >> 1) << EMC_DMCR_TRC_BIT);
#else
	dmcr |= 0xfffc;
#endif

	/* First, precharge phase */
	REG_EMC_DMCR = dmcr;

	/* Set refresh registers */
	tmp = SDRAM_TREF/ns;
	tmp = tmp/64 + 1;
	if (tmp > 0xff) tmp = 0xff;

	REG_EMC_RTCOR = tmp;
	REG_EMC_RTCSR = EMC_RTCSR_CKS_64;	/* Divisor is 64, CKO/64 */

	/* SDRAM mode values */
	sdmode = EMC_SDMR_BT_SEQ | 
		 EMC_SDMR_OM_NORMAL |
		 EMC_SDMR_BL_4 | 
		 cas_latency_sdmr[((SDRAM_CASL == 3) ? 1 : 0)];

	/* precharge all chip-selects */
	REG8(EMC_SDMR0|sdmode) = 0;

	/* wait for precharge, > 200us */
	tmp = (cpu_clk / 1000000) * 200;
	while (tmp--);

	/* enable refresh and set SDRAM mode */
	REG_EMC_DMCR = dmcr | EMC_DMCR_RFSH | EMC_DMCR_MRSET;

	/* write sdram mode register for each chip-select */
	REG8(EMC_SDMR0|sdmode) = 0;

	/* everything is ok now */
}

void sdram_init(void)
{
	register unsigned int dmcr, sdmode, tmp, cpu_clk, mem_clk, ns;

#ifdef CONFIG_MOBILE_SDRAM
	register unsigned int sdemode; /*SDRAM Extended Mode*/
#endif
	unsigned int cas_latency_sdmr[2] = {
		EMC_SDMR_CAS_2,
		EMC_SDMR_CAS_3,
	};

	unsigned int cas_latency_dmcr[2] = {
		1 << EMC_DMCR_TCL_BIT,	/* CAS latency is 2 */
		2 << EMC_DMCR_TCL_BIT	/* CAS latency is 3 */
	};
#ifndef CONFIG_FPGA
	int div[] = {1, 2, 3, 4, 6, 8, 12, 16, 24, 32};
#endif

	REG_DDRC_CFG = 0x80000000;
#ifdef DEBUG
	sdmr_regs_print();
#endif
	cpu_clk = CFG_CPU_SPEED;
#if defined(CONFIG_FPGA)
	mem_clk = CFG_EXTAL / CFG_DIV;
#else
	mem_clk = cpu_clk * div[__cpm_get_cdiv()] / div[__cpm_get_mdiv()];
#endif

	REG_EMC_BCR = 0;	/* Disable bus release */
	REG_EMC_RTCSR = 0;	/* Disable clock for counting */

	/* Basic DMCR value */
	dmcr = ((SDRAM_ROW-11)<<EMC_DMCR_RA_BIT) |
		((SDRAM_COL-8)<<EMC_DMCR_CA_BIT) |
		(SDRAM_BANK4<<EMC_DMCR_BA_BIT) |
		(SDRAM_BW16<<EMC_DMCR_BW_BIT) |
		EMC_DMCR_EPIN |
		cas_latency_dmcr[((SDRAM_CASL == 3) ? 1 : 0)];

	/* SDRAM timimg */
	ns = 1000000000 / mem_clk;
	tmp = SDRAM_TRAS/ns;
	if (tmp < 4) tmp = 4;
	if (tmp > 11) tmp = 11;
	dmcr |= ((tmp-4) << EMC_DMCR_TRAS_BIT);
	tmp = SDRAM_RCD/ns;
	if (tmp > 3) tmp = 3;
	dmcr |= (tmp << EMC_DMCR_RCD_BIT);
	tmp = SDRAM_TPC/ns;
	if (tmp > 7) tmp = 7;
	dmcr |= (tmp << EMC_DMCR_TPC_BIT);
	tmp = SDRAM_TRWL/ns;
	if (tmp > 3) tmp = 3;
	dmcr |= (tmp << EMC_DMCR_TRWL_BIT);
	tmp = (SDRAM_TRAS + SDRAM_TPC)/ns;
	if (tmp > 14) tmp = 14;
	dmcr |= (((tmp + 1) >> 1) << EMC_DMCR_TRC_BIT);

	/* SDRAM mode value */
	sdmode = EMC_SDMR_BT_SEQ | 
		 EMC_SDMR_OM_NORMAL |
		 EMC_SDMR_BL_4 | 
		 cas_latency_sdmr[((SDRAM_CASL == 3) ? 1 : 0)];

	/* Stage 1. Precharge all banks by writing SDMR with DMCR.MRSET=0 */
	REG_EMC_DMCR = dmcr;
	REG8(EMC_SDMR0|sdmode) = 0;

	/* Precharge Bank1 SDRAM */
#if CONFIG_NR_DRAM_BANKS == 2   
	REG_EMC_DMCR = dmcr | EMC_DMCR_MBSEL_B1;
	REG8(EMC_SDMR0|sdmode) = 0;
#endif

#ifdef CONFIG_MOBILE_SDRAM
	/* Mobile SDRAM Extended Mode Register */
	sdemode = EMC_SDMR_SET_BA1 | EMC_SDMR_DS_FULL | EMC_SDMR_PRSR_ALL;
#endif

	/* Wait for precharge, > 200us */
	tmp = (cpu_clk / 1000000) * 1000;
	while (tmp--);

	/* Stage 2. Enable auto-refresh */
	REG_EMC_DMCR = dmcr | EMC_DMCR_RFSH;

	tmp = SDRAM_TREF/ns;
	tmp = tmp/64 + 1;
	if (tmp > 0xff) tmp = 0xff;
	REG_EMC_RTCOR = tmp;
	REG_EMC_RTCNT = 0;
	REG_EMC_RTCSR = EMC_RTCSR_CKS_64;	/* Divisor is 64, CKO/64 */

	/* Wait for number of auto-refresh cycles */
	tmp = (cpu_clk / 1000000) * 1000;
	while (tmp--);

 	/* Stage 3. Mode Register Set */
	REG_EMC_DMCR = dmcr | EMC_DMCR_RFSH | EMC_DMCR_MRSET | EMC_DMCR_MBSEL_B0;
	REG8(EMC_SDMR0|sdmode) = 0;


#ifdef CONFIG_MOBILE_SDRAM
	REG8(EMC_SDMR0|sdemode) = 0;   	/* Set Mobile SDRAM Extended Mode Register */
#endif

#if CONFIG_NR_DRAM_BANKS == 2
	REG_EMC_DMCR = dmcr | EMC_DMCR_RFSH | EMC_DMCR_MRSET | EMC_DMCR_MBSEL_B1;
	REG8(EMC_SDMR0|sdmode) = 0;	/* Set Bank1 SDRAM Register */


#ifdef CONFIG_MOBILE_SDRAM
	REG8(EMC_SDMR0|sdemode) = 0;	/* Set Mobile SDRAM Extended Mode Register */
#endif

#endif   /*CONFIG_NR_DRAM_BANKS == 2*/

	/* Set back to basic DMCR value */
	REG_EMC_DMCR = dmcr | EMC_DMCR_RFSH | EMC_DMCR_MRSET;

	/* bank_size: 32M 64M 128M ... */
	unsigned int bank_size = initdram(0)/ CONFIG_NR_DRAM_BANKS;
	unsigned int mem_base0, mem_base1, mem_mask;

	mem_base0 = EMC_MEM_PHY_BASE >> EMC_MEM_PHY_BASE_SHIFT;
	mem_base1 = ((EMC_MEM_PHY_BASE + bank_size) >> EMC_MEM_PHY_BASE_SHIFT);
	mem_mask = EMC_DMAR_MASK_MASK & 
		(~(((bank_size) >> EMC_MEM_PHY_BASE_SHIFT)-1)&EMC_DMAR_MASK_MASK);

	REG_EMC_DMAR0 = (mem_base0 << EMC_DMAR_BASE_BIT) | mem_mask;
	REG_EMC_DMAR1 = (mem_base1 << EMC_DMAR_BASE_BIT) | mem_mask;

	/* everything is ok now */
#ifdef DEBUG
	sdmr_regs_print();
	ddr_dma_test();
	mem_test();
#endif
}

#else

void ddr_mem_init(int msel, int hl, int tsel, int cvt)
{
	volatile tmp_cnt;
	register unsigned int cpu_clk, ddr_twr;
	register unsigned int ddrc_cfg_reg=0, init_ddrc_mdelay=0;

	cpu_clk = CFG_CPU_SPEED;

#if defined(CONFIG_SDRAM_DDR2) // ddr2
	ddrc_cfg_reg = cvt << 30 | DDRC_CFG_TYPE_DDR2 | (DDR_ROW-12)<<10
		| (DDR_COL-8)<<8 | DDR_CS1EN<<7 | DDR_CS0EN<<6
		| ((DDR_CL-1) | 0x8)<<2 | DDR_BANK8<<1 | DDR_DW32;
#elif defined(CONFIG_SDRAM_DDR1) // ddr1
	ddrc_cfg_reg = cvt << 30 | DDRC_CFG_BTRUN |DDRC_CFG_TYPE_DDR1
		| (DDR_ROW-12)<<10 | (DDR_COL-8)<<8 | DDR_CS1EN<<7 | DDR_CS0EN<<6
		| ((DDR_CL_HALF?(DDR_CL&~0x8):((DDR_CL-1)|0x8))<<2)
		| DDR_BANK8<<1 | DDR_DW32;
#else // mobile ddr
	ddrc_cfg_reg = cvt << 30 | DDRC_CFG_BTRUN | DDRC_CFG_TYPE_MDDR
		| (DDR_ROW-12)<<10 | (DDR_COL-8)<<8 | DDR_CS1EN<<7 | DDR_CS0EN<<6
		| ((DDR_CL-1) | 0x8)<<2 | DDR_BANK8<<1 | DDR_DW32;
#endif

	ddrc_cfg_reg |= DDRC_CFG_MPRT;

	init_ddrc_mdelay= tsel<<18 | msel<<16 | hl<<15;
	ddr_twr = ((REG_DDRC_TIMING1 & DDRC_TIMING1_TWR_MASK) >> DDRC_TIMING1_TWR_BIT) + 1;
	REG_DDRC_CFG     = ddrc_cfg_reg;
	REG_DDRC_MDELAY = init_ddrc_mdelay;
	/***** init ddrc registers & ddr memory regs ****/

	/* Wait for number of auto-refresh cycles */
	tmp_cnt = (cpu_clk / 1000000) * 10;
	while (tmp_cnt--);

#if defined(CONFIG_SDRAM_DDR2) // ddr1 and mddr
	/* Set CKE High */
	REG_DDRC_CTRL = DDRC_CTRL_CKE; // ?

	/* Wait for number of auto-refresh cycles */
	tmp_cnt = (cpu_clk / 1000000) * 1;
	while (tmp_cnt--);

	/* PREA */
	REG_DDRC_LMR =  DDRC_LMR_CMD_PREC | DDRC_LMR_START; //0x1;

	/* Wait for DDR_tRP */
	tmp_cnt = (cpu_clk / 1000000) * 1;
	while (tmp_cnt--);

	/* EMR2: extend mode register2 */
	REG_DDRC_LMR = DDRC_LMR_BA_EMRS2 | DDRC_LMR_CMD_LMR | DDRC_LMR_START;//0x221;

	/* EMR3: extend mode register3 */
	REG_DDRC_LMR = DDRC_LMR_BA_EMRS3 | DDRC_LMR_CMD_LMR | DDRC_LMR_START;//0x321;

	/* EMR1: extend mode register1 */
	REG_DDRC_LMR = (DDR_EMRS1_DQS_DIS << 16) | DDRC_LMR_BA_EMRS1 | DDRC_LMR_CMD_LMR | DDRC_LMR_START;

	/* wait DDR_tMRD */
	tmp_cnt = (cpu_clk / 1000000) * 1;
	while (tmp_cnt--);

	/* MR - DLL Reset A1A0 burst 2 */
	REG_DDRC_LMR = ((ddr_twr-1)<<9 | DDR2_MRS_DLL_RST | DDR_CL<<4 | DDR_MRS_BL_4)<< 16
		| DDRC_LMR_BA_MRS | DDRC_LMR_CMD_LMR | DDRC_LMR_START;

	/* wait DDR_tMRD */
	tmp_cnt = (cpu_clk / 1000000) * 1;
	while (tmp_cnt--);

	/* PREA */
	REG_DDRC_LMR =  DDRC_LMR_CMD_PREC | DDRC_LMR_START; //0x1;

	/* Wait for DDR_tRP */
	tmp_cnt = (cpu_clk / 1000000) * 1;
	while (tmp_cnt--);

	/* AR: auto refresh */
	REG_DDRC_LMR = DDRC_LMR_CMD_AUREF | DDRC_LMR_START; //0x11;
	/* Wait for DDR_tRP */
	tmp_cnt = (cpu_clk / 1000000) * 1;
	while (tmp_cnt--);

	REG_DDRC_LMR = DDRC_LMR_CMD_AUREF | DDRC_LMR_START; //0x11;

	/* Wait for DDR_tRP */
	tmp_cnt = (cpu_clk / 1000000) * 1;
	while (tmp_cnt--);

	/* MR - DLL Reset End */
	REG_DDRC_LMR = ((ddr_twr-1)<<9 | DDR_CL<<4 | DDR_MRS_BL_4)<< 16
		| DDRC_LMR_BA_MRS | DDRC_LMR_CMD_LMR | DDRC_LMR_START;

	/* wait 200 tCK */
	tmp_cnt = (cpu_clk / 1000000) * 2;
	while (tmp_cnt--);

	/* EMR1 - OCD Default */
	REG_DDRC_LMR = (DDR_EMRS1_DQS_DIS | DDR_EMRS1_OCD_DFLT) << 16
		| DDRC_LMR_BA_EMRS1 | DDRC_LMR_CMD_LMR | DDRC_LMR_START;

	/* EMR1 - OCD Exit */
	REG_DDRC_LMR = (DDR_EMRS1_DQS_DIS << 16) | DDRC_LMR_BA_EMRS1 | DDRC_LMR_CMD_LMR | DDRC_LMR_START;

	/* wait DDR_tMRD */
	tmp_cnt = (cpu_clk / 1000000) * 1;
	while (tmp_cnt--);

#elif defined(CONFIG_SDRAM_DDR1) // ddr1 and mddr
	/* set cke high */
	REG_DDRC_CTRL = DDRC_CTRL_CKE; // ?

	/* Nop command */
	tmp_cnt = (cpu_clk / 1000000) * 1;
	while (tmp_cnt--);

	/* PREA  all */
	REG_DDRC_LMR =  DDRC_LMR_CMD_PREC | DDRC_LMR_START; //0x1;

	/* Wait for DDR_tRP */
	tmp_cnt = (cpu_clk / 1000000) * 1;
	while (tmp_cnt--);

	/* EMR: extend mode register: enable DLL */
	REG_DDRC_LMR = (DDR1_EMRS_OM_NORMAL | DDR1_EMRS_DS_FULL | DDR1_EMRS_DLL_EN) << 16
		| DDRC_LMR_BA_N_EMRS | DDRC_LMR_CMD_LMR | DDRC_LMR_START;

	/* MR DLL reset */
	REG_DDRC_LMR = (DDR1_MRS_OM_DLLRST | (DDR_CL_HALF?(DDR_CL|0x4):DDR_CL)<<4 | DDR_MRS_BL_4)<< 16
		| DDRC_LMR_BA_N_MRS | DDRC_LMR_CMD_LMR | DDRC_LMR_START;

	/* wait DDR_tXSRD, 200 tCK */
	tmp_cnt = (cpu_clk / 1000000) * 2;
	while (tmp_cnt--);
	/* PREA all */
	REG_DDRC_LMR =  DDRC_LMR_CMD_PREC | DDRC_LMR_START; //0x1;
	REG_DDRC_LMR = DDRC_LMR_CMD_AUREF | DDRC_LMR_START; //0x11;
	REG_DDRC_LMR = DDRC_LMR_CMD_AUREF | DDRC_LMR_START; //0x11;
	tmp_cnt = (cpu_clk / 1000000) * 15;
	while (tmp_cnt--);
	/* EMR: extend mode register, clear dll en */
	REG_DDRC_LMR = (DDR1_EMRS_OM_NORMAL | DDR1_EMRS_DS_FULL) << 16
		| DDRC_LMR_BA_N_EMRS | DDRC_LMR_CMD_LMR | DDRC_LMR_START;
	/* wait DDR_tMRD */
	tmp_cnt = (cpu_clk / 1000000) * 1;
	while (tmp_cnt--);

#elif defined(CONFIG_SDRAM_MDDR) // ddr1 and mddr
	REG_DDRC_CTRL = DDRC_CTRL_CKE; // ?

	/* Wait for number of auto-refresh cycles */
	tmp_cnt = (cpu_clk / 1000000) * 20;
	while (tmp_cnt--);

	/* PREA */
	REG_DDRC_LMR =  DDRC_LMR_CMD_PREC | DDRC_LMR_START; //0x1;

	/* Wait for DDR_tRP */
	tmp_cnt = (cpu_clk / 1000000) * 1;
	while (tmp_cnt--);

	/* AR: auto refresh */
	REG_DDRC_LMR = DDRC_LMR_CMD_AUREF | DDRC_LMR_START; //0x11;

	/* wait DDR_tRFC */
	tmp_cnt = (cpu_clk / 1000000) * 1;
	while (tmp_cnt--);

	/* AR: auto refresh */
	REG_DDRC_LMR = DDRC_LMR_CMD_AUREF | DDRC_LMR_START; //0x11;
	/* wait DDR_tRFC */
	tmp_cnt = (cpu_clk / 1000000) * 1;
	while (tmp_cnt--);

	/* MR */
	REG_DDRC_LMR = (DDR_CL<<4 | DDR_MRS_BL_4)<< 16
		| DDRC_LMR_BA_M_MRS | DDRC_LMR_CMD_LMR | DDRC_LMR_START;

	/* wait DDR_tMRD */
	tmp_cnt = (cpu_clk / 1000000) * 1;
	while (tmp_cnt--);

	/* EMR: extend mode register */
//	REG_DDRC_LMR = (DDR_EMRS_DS_FULL | DDR_EMRS_PRSR_ALL) << 16
	REG_DDRC_LMR = (DDR_EMRS_DS_HALF | DDR_EMRS_PRSR_ALL) << 16
		| DDRC_LMR_BA_M_EMRS | DDRC_LMR_CMD_LMR | DDRC_LMR_START;

	/* wait DDR_tMRD */
	tmp_cnt = (cpu_clk / 1000000) * 1;
	while (tmp_cnt--);

#endif
}

#define DEF_DDR_CVT 0
#define DDR_USE_FIRST_ARGS 0
/* DDR sdram init */
void sdram_init(void)
{
	int i, max = 0, num = 0, tsel = 0, msel, hl;
	int cvt = DEF_DDR_CVT, cvt_cnt0 = 0, cvt_cnt1 = 1, max0 = 0, max1 = 0, min0 = 0, min1 = 0, tsel0 = 0, tsel1 = 0;
	volatile unsigned int tmp_cnt;
	register unsigned int tmp, cpu_clk, mem_clk, ddr_twr, ns, ns_int, div;
	register unsigned int ddrc_timing1_reg=0, ddrc_timing2_reg=0, init_ddrc_refcnt=0, init_ddrc_dqs=0, init_ddrc_ctrl=0;
	struct ddr_delay_sel_t ddr_delay_sel[] = {
		{0, 1}, {0, 0},	{1, 1}, {1, 0},
		{2, 1}, {2, 0},	{3, 1}, {3, 0}
	};
	register unsigned int memsize, ddrc_mmap0_reg, ddrc_mmap1_reg, mem_base0, mem_base1, mem_mask0, mem_mask1;

//	int delay_index0[8], delay_index1[8];

#ifdef DEBUG
	ddrc_regs_print();
#endif

	cpu_clk = CFG_CPU_SPEED;

#if defined(CONFIG_FPGA)
	mem_clk = CFG_EXTAL / CFG_DIV;
#else
	mem_clk = cpu_clk * div[__cpm_get_cdiv()] / div[__cpm_get_mdiv()];
#endif
	dprintf("mem_clk = %d, cpu_clk = %d\n", mem_clk, cpu_clk);

#if defined(CONFIG_FPGA)
	ns = 7;
#else
	ns = 1000000000 / mem_clk; /* ns per tck ns <= real value */
#endif

	/* ACTIVE to PRECHARGE command period */
	tmp = (DDR_tRAS%ns == 0) ? (DDR_tRAS/ns) : (DDR_tRAS/ns+1);
	if (tmp < 1) tmp = 1;
	if (tmp > 31) tmp = 31;
	ddrc_timing1_reg = ((tmp/2) << DDRC_TIMING1_TRAS_BIT);

	/* READ to PRECHARGE command period. */
	tmp = (DDR_tRTP%ns == 0) ? (DDR_tRTP/ns) : (DDR_tRTP/ns+1);
	if (tmp < 1) tmp = 1;
	if (tmp > 4) tmp = 4;
	ddrc_timing1_reg |= ((tmp-1) << DDRC_TIMING1_TRTP_BIT);
	
	/* PRECHARGE command period. */
	tmp = (DDR_tRP%ns == 0) ? DDR_tRP/ns : (DDR_tRP/ns+1);
	if (tmp < 1) tmp = 1;
	if (tmp > 8) tmp = 8;
	ddrc_timing1_reg |= ((tmp-1) << DDRC_TIMING1_TRP_BIT);

	/* ACTIVE to READ or WRITE command period. */
	tmp = (DDR_tRCD%ns == 0) ? DDR_tRCD/ns : (DDR_tRCD/ns+1);
	if (tmp < 1) tmp = 1;
	if (tmp > 8) tmp = 8;
	ddrc_timing1_reg |= ((tmp-1) << DDRC_TIMING1_TRCD_BIT);

	/* ACTIVE to ACTIVE command period. */
	tmp = (DDR_tRC%ns == 0) ? DDR_tRC/ns : (DDR_tRC/ns+1);
	if (tmp < 3) tmp = 3;
	if (tmp > 31) tmp = 31;
	ddrc_timing1_reg |= ((tmp/2) << DDRC_TIMING1_TRC_BIT);

	/* ACTIVE bank A to ACTIVE bank B command period. */
	tmp = (DDR_tRRD%ns == 0) ? DDR_tRRD/ns : (DDR_tRRD/ns+1);
	if (tmp < 2) tmp = 2;
	if (tmp > 4) tmp = 4;
	ddrc_timing1_reg |= ((tmp-1) << DDRC_TIMING1_TRRD_BIT);


	/* WRITE Recovery Time defined by register MR of DDR2 memory */
	tmp = (DDR_tWR%ns == 0) ? DDR_tWR/ns : (DDR_tWR/ns+1);
	tmp = (tmp < 1) ? 1 : tmp;
	tmp = (tmp < 2) ? 2 : tmp;
	tmp = (tmp > 6) ? 6 : tmp;
	ddrc_timing1_reg |= ((tmp-1) << DDRC_TIMING1_TWR_BIT);
	ddr_twr = tmp; 

	/* WRITE to READ command delay. */ 
	tmp = (DDR_tWTR%ns == 0) ? DDR_tWTR/ns : (DDR_tWTR/ns+1);
	if (tmp > 4) tmp = 4;
	ddrc_timing1_reg |= ((tmp-1) << DDRC_TIMING1_TWTR_BIT);


	/* WRITE to READ command delay. */ 
	tmp = DDR_tWTR/ns;
	if (tmp < 1) tmp = 1;
	if (tmp > 4) tmp = 4;
	ddrc_timing1_reg |= ((tmp-1) << DDRC_TIMING1_TWTR_BIT);


	/* AUTO-REFRESH command period. */
	tmp = (DDR_tRFC%ns == 0) ? DDR_tRFC/ns : (DDR_tRFC/ns+1);
	if (tmp > 31) tmp = 31;
	ddrc_timing2_reg = ((tmp/2) << DDRC_TIMING2_TRFC_BIT);

	/* Minimum Self-Refresh / Deep-Power-Down time */
	tmp = DDR_tMINSR/ns;
	if (tmp < 9) tmp = 9;
	if (tmp > 129) tmp = 129;
	ddrc_timing2_reg |= (((tmp-1)/8-1) << DDRC_TIMING2_TMINSR_BIT);
	ddrc_timing2_reg |= (DDR_tXP-1)<<4 | (DDR_tMRD-1);

	init_ddrc_refcnt = DDR_CLK_DIV << 1 | DDRC_REFCNT_REF_EN;

	ns_int = (1000000000%mem_clk == 0) ?
		(1000000000/mem_clk) : (1000000000/mem_clk+1);
	tmp = DDR_tREFI/ns_int;
	tmp = tmp / (16 * (1 << DDR_CLK_DIV)) - 1;
	if (tmp > 0xfff)
		tmp = 0xfff;
	if (tmp < 1)
		tmp = 1;

	init_ddrc_refcnt |= tmp << DDRC_REFCNT_CON_BIT;
	init_ddrc_dqs = DDRC_DQS_AUTO | DDRC_DQS_DET;

	/* precharge power down, disable power down */
	/* precharge power down, if set active power down, |= DDRC_CTRL_ACTPD */
	init_ddrc_ctrl = DDRC_CTRL_PDT_DIS | DDRC_CTRL_PRET_8 | DDRC_CTRL_UNALIGN | DDRC_CTRL_CKE;
#if 0
	if (mem_clk > 60000000)
		init_ddrc_ctrl |= DDRC_CTRL_RDC;
#endif
#if defined(CONFIG_FPGA)
__convert:
	do {
		num = 0;
		for (i = 0; i < 8; i++) {
			/* reset ddrc_controller */
			REG_DDRC_CTRL = DDRC_CTRL_RESET;

			/* Wait for precharge, > 200us */
			tmp_cnt = (cpu_clk / 1000000) * 200;
			while (tmp_cnt--);
			
			REG_DDRC_CTRL = 0x0;
			REG_DDRC_TIMING1 = ddrc_timing1_reg;
			REG_DDRC_TIMING2 = ddrc_timing2_reg;

			ddr_mem_init(ddr_delay_sel[i].msel, ddr_delay_sel[i].hl, tsel, cvt);

			dprintf("msel = %d, hl = %d, tsel = %d, cvt = %d\n", ddr_delay_sel[i].msel, ddr_delay_sel[i].hl, tsel, cvt);
			memsize = initdram(0);
			mem_base0 = DDR_MEM_PHY_BASE >> 24;
			mem_base1 = (DDR_MEM_PHY_BASE + memsize / (DDR_CS1EN + DDR_CS0EN)) >> 24;
			mem_mask1 = mem_mask0 = 0xff &
				~(((memsize/(DDR_CS1EN+DDR_CS0EN) >> 24)
				   - 1) & DDRC_MMAP_MASK_MASK);
			
			ddrc_mmap0_reg = mem_base0 << DDRC_MMAP_BASE_BIT | mem_mask0;
			ddrc_mmap1_reg = mem_base1 << DDRC_MMAP_BASE_BIT | mem_mask1;

			REG_DDRC_MMAP0 = ddrc_mmap0_reg;
			REG_DDRC_MMAP1 = ddrc_mmap1_reg;
#ifdef DEBUG
			ddrc_regs_print();
#endif

			REG_DDRC_REFCNT = init_ddrc_refcnt;

			/* Enable DLL Detect */
			REG_DDRC_DQS    = init_ddrc_dqs;

			/* Set CKE High */
			REG_DDRC_CTRL = init_ddrc_ctrl;

			/* Wait for number of auto-refresh cycles */
			tmp_cnt = (cpu_clk / 1000000) * 10;
			while (tmp_cnt--);

			/* Auto Refresh */
			REG_DDRC_LMR = DDRC_LMR_CMD_AUREF | DDRC_LMR_START; //0x11;

			/* Wait for number of auto-refresh cycles */
			tmp_cnt = (cpu_clk / 1000000) * 10;
			while (tmp_cnt--);


			tmp_cnt = (cpu_clk / 1000000) * 10;
			while (tmp_cnt--);

			if(ddr_dma_test() != 0) {
				if (num > 0) break;
				num =0;
				continue;
			}
			else {
				num++; max = i;
			}
		}
		if (num > 0) break;
		tsel++;
	} while(tsel < 4);
	/* if can't find a right value, try to convert bit 30: only for FPGA */
	if (cvt == DEF_DDR_CVT) {
		cvt_cnt0 = num;
		max0 = max;
		tsel0 = tsel;
		cvt = !cvt;num = 0;max = 0;tsel=0;
		goto __convert;
	}

	cvt_cnt1 = num;
	max1 = max;
	tsel1 = tsel;

	cvt = (cvt_cnt1 > cvt_cnt0) ? !DEF_DDR_CVT : DEF_DDR_CVT;

	dprintf("num0 = %d, max0 = %d\n", cvt_cnt0, max0);
	dprintf("num1 = %d, max1 = %d\n", cvt_cnt1, max1);
	dprintf("===> cvt = %d\n", cvt);
	if (cvt_cnt1 == 0 && cvt_cnt0 == 0) {
		serial_puts("\n\nDDR INIT ERROR: Might because memory clock is too low.\n");
		return;
	}
	if (cvt_cnt1 > cvt_cnt0) {
		cvt = !DEF_DDR_CVT;
#if DDR_USE_FIRST_ARGS == 1
		msel = ddr_delay_sel[(2*max1 - cvt_cnt1 + 1)/2].msel;
		hl = ddr_delay_sel[(2*max1 - cvt_cnt1 + 1)/2].hl;
#else
		msel = ddr_delay_sel[(2*max1 - cvt_cnt1 + 2)/2].msel;
		hl = ddr_delay_sel[(2*max1 - cvt_cnt1 + 2)/2].hl;
#endif
		tsel = tsel1;
	} else {
		cvt = DEF_DDR_CVT;
#if DDR_USE_FIRST_ARGS == 1
		msel = ddr_delay_sel[(2*max0 - cvt_cnt0 + 1)/2].msel;
		hl = ddr_delay_sel[(2*max0 - cvt_cnt0 + 1)/2].hl;
#else
		msel = ddr_delay_sel[(2*max0 - cvt_cnt0 + 2)/2].msel;
		hl = ddr_delay_sel[(2*max0 - cvt_cnt0 + 2)/2].hl;
#endif
		tsel = tsel0;
	}
	dprintf("DDR Args: msel = %d, hl = %d, tsel = %d, cvt = %d\n", msel, hl, tsel, cvt);

	/* reset ddrc_controller */
	REG_DDRC_CTRL = DDRC_CTRL_RESET;
	
	/* Wait for precharge, > 200us */
	tmp_cnt = (cpu_clk / 1000000) * 200;
	while (tmp_cnt--);
	
	REG_DDRC_CTRL = 0x0;
	REG_DDRC_TIMING1 = ddrc_timing1_reg;
	REG_DDRC_TIMING2 = ddrc_timing2_reg;

	ddr_mem_init(msel, hl, tsel, cvt);
#else
	/* Add Jz4760 chip here. Jz4760 chip have no cvt */
#endif //defined(CONFIG_FPGA)
	memsize = initdram(0);
	mem_base0 = DDR_MEM_PHY_BASE >> 24;
	mem_base1 = (DDR_MEM_PHY_BASE + memsize / (DDR_CS1EN + DDR_CS0EN)) >> 24;
	mem_mask1 = mem_mask0 = 0xff &
		~(((memsize/(DDR_CS1EN+DDR_CS0EN) >> 24)
		   - 1) & DDRC_MMAP_MASK_MASK);
	
	ddrc_mmap0_reg = mem_base0 << DDRC_MMAP_BASE_BIT | mem_mask0;
	ddrc_mmap1_reg = mem_base1 << DDRC_MMAP_BASE_BIT | mem_mask1;

	REG_DDRC_MMAP0 = ddrc_mmap0_reg;
	REG_DDRC_MMAP1 = ddrc_mmap1_reg;
	REG_DDRC_REFCNT = init_ddrc_refcnt;

	/* Enable DLL Detect */
	REG_DDRC_DQS    = init_ddrc_dqs;
			
	/* Set CKE High */
	REG_DDRC_CTRL = init_ddrc_ctrl;

	/* Wait for number of auto-refresh cycles */
	tmp_cnt = (cpu_clk / 1000000) * 10;
	while (tmp_cnt--);
	
	/* Auto Refresh */
	REG_DDRC_LMR = DDRC_LMR_CMD_AUREF | DDRC_LMR_START; //0x11;
	
	/* Wait for number of auto-refresh cycles */
	tmp_cnt = (cpu_clk / 1000000) * 10;
	while (tmp_cnt--);
#ifdef DEBUG
	ddrc_regs_print();
	ddr_dma_test();
#endif
}
#endif

#if !defined(CONFIG_NAND_SPL) && !defined(CONFIG_SPI_SPL) && !defined(CONFIG_MSC_SPL)
static void calc_clocks(void)
{
	DECLARE_GLOBAL_DATA_PTR;

#ifndef CONFIG_FPGA
	unsigned int pllout;
	unsigned int div[10] = {1, 2, 3, 4, 6, 8, 12, 16, 24, 32};

	pllout = __cpm_get_pllout();

	gd->cpu_clk = pllout / div[__cpm_get_cdiv()];
	gd->sys_clk = pllout / div[__cpm_get_hdiv()];
	gd->per_clk = pllout / div[__cpm_get_pdiv()];
	gd->mem_clk = pllout / div[__cpm_get_mdiv()];
	gd->dev_clk = CFG_EXTAL;
#else
	gd->cpu_clk = CFG_CPU_SPEED;
	gd->sys_clk = gd->per_clk = gd->mem_clk = gd->dev_clk 
		= CFG_EXTAL / CFG_DIV;
#endif
}
#ifndef CONFIG_FPGA
static void rtc_init(void)
{

	while ( !__rtc_write_ready()) ;
	__rtc_enable_alarm();	/* enable alarm */

	while ( !__rtc_write_ready()) 
		;
	REG_RTC_RGR   = 0x00007fff; /* type value */

	while ( !__rtc_write_ready()) 
		;
	REG_RTC_HWFCR = 0x0000ffe0; /* Power on delay 2s */

	while ( !__rtc_write_ready()) 
		;
	REG_RTC_HRCR  = 0x00000fe0; /* reset delay 125ms */

}
#endif

//----------------------------------------------------------------------
// jz4760 board init routine

int jz_board_init(void)
{
	board_early_init();  /* init gpio, pll etc. */

#if !defined(CONFIG_FPGA) && !defined(CONFIG_NAND_U_BOOT) && !defined(CONFIG_SPI_U_BOOT)
	pll_init();          /* init PLL, do it when nor boot or defined(CONFIG_MSC_U_BOOT) */
#endif
#if !defined(CONFIG_NAND_U_BOOT) && !defined(CONFIG_SPI_U_BOOT) && !defined(CONFIG_MSC_U_BOOT)
	serial_init();
	sdram_init();        /* init sdram memory */
#endif

	calc_clocks();       /* calc the clocks */
#ifndef CONFIG_FPGA
	rtc_init();		/* init rtc on any reset: */
#endif
	return 0;
}

//----------------------------------------------------------------------
// Timer routines

#define TIMER_CHAN  0
#define TIMER_FDATA 0xffff  /* Timer full data value */
#define TIMER_HZ    CFG_HZ

#define READ_TIMER  REG_TCU_TCNT(TIMER_CHAN)  /* macro to read the 16 bit timer */

static ulong timestamp;
static ulong lastdec;

void	reset_timer_masked	(void);
ulong	get_timer_masked	(void);
void	udelay_masked		(unsigned long usec);

/*
 * timer without interrupts
 */

int timer_init(void)
{
	REG_TCU_TCSR(TIMER_CHAN) = TCU_TCSR_PRESCALE256 | TCU_TCSR_EXT_EN;
	REG_TCU_TCNT(TIMER_CHAN) = 0;
	REG_TCU_TDHR(TIMER_CHAN) = 0;
	REG_TCU_TDFR(TIMER_CHAN) = TIMER_FDATA;

	REG_TCU_TMSR = (1 << TIMER_CHAN) | (1 << (TIMER_CHAN + 16)); /* mask irqs */
	REG_TCU_TSCR = (1 << TIMER_CHAN); /* enable timer clock */
	REG_TCU_TESR = (1 << TIMER_CHAN); /* start counting up */

	lastdec = 0;
	timestamp = 0;

	return 0;
}

void reset_timer(void)
{
	reset_timer_masked ();
}

ulong get_timer(ulong base)
{
	return get_timer_masked () - base;
}

void set_timer(ulong t)
{
	timestamp = t;
}

void udelay (unsigned long usec)
{
	ulong tmo,tmp;

	/* normalize */
	if (usec >= 1000) {
		tmo = usec / 1000;
		tmo *= TIMER_HZ;
		tmo /= 1000;
	}
	else {
		if (usec >= 1) {
			tmo = usec * TIMER_HZ;
			tmo /= (1000*1000);
		}
		else
			tmo = 1;
	}

	/* check for rollover during this delay */
	tmp = get_timer (0);
	if ((tmp + tmo) < tmp )
		reset_timer_masked();  /* timer would roll over */
	else
		tmo += tmp;

	while (get_timer_masked () < tmo);
}

void reset_timer_masked (void)
{
	/* reset time */
	lastdec = READ_TIMER;
	timestamp = 0;
}

ulong get_timer_masked (void)
{
	ulong now = READ_TIMER;

	if (lastdec <= now) {
		/* normal mode */
		timestamp += (now - lastdec);
	} else {
		/* we have an overflow ... */
		timestamp += TIMER_FDATA + now - lastdec;
	}
	lastdec = now;

	return timestamp;
}

void udelay_masked (unsigned long usec)
{
	ulong tmo;
	ulong endtime;
	signed long diff;

	/* normalize */
	if (usec >= 1000) {
		tmo = usec / 1000;
		tmo *= TIMER_HZ;
		tmo /= 1000;
	} else {
		if (usec > 1) {
			tmo = usec * TIMER_HZ;
			tmo /= (1000*1000);
		} else {
			tmo = 1;
		}
	}

	endtime = get_timer_masked () + tmo;

	do {
		ulong now = get_timer_masked ();
		diff = endtime - now;
	} while (diff >= 0);
}

/*
 * This function is derived from PowerPC code (read timebase as long long).
 * On MIPS it just returns the timer value.
 */
unsigned long long get_ticks(void)
{
	return get_timer(0);
}

/*
 * This function is derived from PowerPC code (timebase clock frequency).
 * On MIPS it returns the number of timer ticks per second.
 */
ulong get_tbclk (void)
{
	return TIMER_HZ;
}

#endif /* !defined(CONFIG_NAND_SPL) && !defined(CONFIG_SPI_SPL) && !defined(CONFIG_MSC_SPL) */

//---------------------------------------------------------------------
// End of timer routine.
//---------------------------------------------------------------------

#endif /* CONFIG_JZ4760 */

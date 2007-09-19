/*
 * Copyright 2007  Luc Verhaegen <lverhaegen@novell.com>
 * Copyright 2007  Matthias Hopf <mhopf@novell.com>
 * Copyright 2007  Egbert Eich   <eich@novell.com>
 * Copyright 2007  Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * This tool is here to help create a connector mapping table.
 *
 */

#include <stdio.h>
#include <sys/io.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/mman.h>
#include <pci/pci.h>
#include <unistd.h>

#define Bool int
#define FALSE 0
#define TRUE 1

/* Some register names */
enum {
    /* DAC A */
    DACA_ENABLE                    = 0x7800,
    DACA_SOURCE_SELECT             = 0x7804,
    DACA_AUTODETECT_CONTROL        = 0x7828,
    DACA_FORCE_OUTPUT_CNTL         = 0x783C,
    DACA_FORCE_DATA                = 0x7840,
    DACA_POWERDOWN                 = 0x7850,
    DACA_CONTROL1                  = 0x7854,
    DACA_CONTROL2                  = 0x7858,
    DACA_COMPARATOR_ENABLE         = 0x785C,
    DACA_COMPARATOR_OUTPUT         = 0x7860,

    /* DAC B */
    DACB_ENABLE                    = 0x7A00,
    DACB_SOURCE_SELECT             = 0x7A04,
    DACB_AUTODETECT_CONTROL        = 0x7A28,
    DACB_FORCE_OUTPUT_CNTL         = 0x7A3C,
    DACB_FORCE_DATA                = 0x7A40,
    DACB_POWERDOWN                 = 0x7A50,
    DACB_CONTROL1                  = 0x7A54,
    DACB_CONTROL2                  = 0x7A58,
    DACB_COMPARATOR_ENABLE         = 0x7A5C,
    DACB_COMPARATOR_OUTPUT         = 0x7A60,

    /* TMDSA */
    TMDSA_CNTL                     = 0x7880,
    TMDSA_SOURCE_SELECT            = 0x7884,
    TMDSA_COLOR_FORMAT             = 0x7888,
    TMDSA_FORCE_OUTPUT_CNTL        = 0x788C,
    TMDSA_BIT_DEPTH_CONTROL        = 0x7894,
    TMDSA_DCBALANCER_CONTROL       = 0x78D0,
    TMDSA_DATA_SYNCHRONIZATION_R500 = 0x78D8,
    TMDSA_DATA_SYNCHRONIZATION_R600 = 0x78DC,
    TMDSA_TRANSMITTER_ENABLE       = 0x7904,
    TMDSA_LOAD_DETECT              = 0x7908,
    TMDSA_MACRO_CONTROL            = 0x790C, /* r5x0 and r600: 3 for pll and 1 for TX */
    TMDSA_PLL_ADJUST               = 0x790C, /* rv6x0: pll only */
    TMDSA_TRANSMITTER_CONTROL      = 0x7910,
    TMDSA_TRANSMITTER_ADJUST       = 0x7920, /* rv6x0: TX part of macro control */

    /* LVTMA */
    LVTMA_CNTL                     = 0x7A80,
    LVTMA_SOURCE_SELECT            = 0x7A84,
    LVTMA_BIT_DEPTH_CONTROL        = 0x7A94,
    LVTMA_DATA_SYNCHRONIZATION     = 0x7AD8,
    LVTMA_PWRSEQ_REF_DIV           = 0x7AE4,
    LVTMA_PWRSEQ_DELAY1            = 0x7AE8,
    LVTMA_PWRSEQ_DELAY2            = 0x7AEC,
    LVTMA_PWRSEQ_CNTL              = 0x7AF0,
    LVTMA_PWRSEQ_STATE             = 0x7AF4,
    LVTMA_LVDS_DATA_CNTL           = 0x7AFC,
    LVTMA_MODE                     = 0x7B00,
    LVTMA_TRANSMITTER_ENABLE       = 0x7B04,
    LVTMA_MACRO_CONTROL            = 0x7B0C,
    LVTMA_TRANSMITTER_CONTROL      = 0x7B10,

       /* I2C */
    DC_I2C_CONTROL		   = 0x7D30,  /* (RW) */
    DC_I2C_ARBITRATION             = 0x7D34,  /* (RW) */
    DC_I2C_INTERRUPT_CONTROL       = 0x7D38,  /* (RW) */
    DC_I2C_SW_STATUS	           = 0x7d3c,  /* (RW) */
    DC_I2C_DDC1_SPEED              = 0x7D4C,  /* (RW) */
    DC_I2C_DDC1_SETUP              = 0x7D50,  /* (RW) */
    DC_I2C_DDC2_SPEED              = 0x7D54,  /* (RW) */
    DC_I2C_DDC2_SETUP              = 0x7D58,  /* (RW) */
    DC_I2C_DDC3_SPEED              = 0x7D5C,  /* (RW) */
    DC_I2C_DDC3_SETUP              = 0x7D60,  /* (RW) */
    DC_I2C_TRANSACTION0            = 0x7D64,  /* (RW) */
    DC_I2C_TRANSACTION1            = 0x7D68,  /* (RW) */
    DC_I2C_DATA                    = 0x7D74,  /* (RW) */
    DC_I2C_DDC4_SPEED              = 0x7DB4,  /* (RW) */
    DC_I2C_DDC4_SETUP              = 0x7DBC,  /* (RW) */
    DC_GPIO_DDC4_MASK              = 0x7E00,  /* (RW) */
    DC_GPIO_DDC4_A                 = 0x7E04,  /* (RW) */
    DC_GPIO_DDC4_EN                = 0x7E08,  /* (RW) */
    DC_GPIO_DDC1_MASK              = 0x7E40,  /* (RW) */
    DC_GPIO_DDC1_A                 = 0x7E44,  /* (RW) */
    DC_GPIO_DDC1_EN                = 0x7E48,  /* (RW) */
    DC_GPIO_DDC1_Y                 = 0x7E4C,  /* (RW) */
    DC_GPIO_DDC2_MASK              = 0x7E50,  /* (RW) */
    DC_GPIO_DDC2_A                 = 0x7E54,  /* (RW) */
    DC_GPIO_DDC2_EN                = 0x7E58,  /* (RW) */
    DC_GPIO_DDC2_Y                 = 0x7E5C,  /* (RW) */
    DC_GPIO_DDC3_MASK              = 0x7E60,  /* (RW) */
    DC_GPIO_DDC3_A                 = 0x7E64,  /* (RW) */
    DC_GPIO_DDC3_EN                = 0x7E68,  /* (RW) */
    DC_GPIO_DDC3_Y                 = 0x7E6C,  /* (RW) */

    /* HPD */
    DC_GPIO_HPD_Y                  = 0x7E9C
};


/* for RHD_R500/R600 */
char ChipType;

/*
 * Match pci ids against data and some callbacks
 */
struct rhd_device {
    unsigned short vendor;
    unsigned short device;
    unsigned char bar;
#define RHD_R500 1
#define RHD_R600 2
    unsigned char type;
} rhd_devices[] = {

    { 0x1002, 0x7100, 2, RHD_R500},
    { 0x1002, 0x7101, 2, RHD_R500},
    { 0x1002, 0x7102, 2, RHD_R500},
    { 0x1002, 0x7103, 2, RHD_R500},
    { 0x1002, 0x7104, 2, RHD_R500},
    { 0x1002, 0x7105, 2, RHD_R500},
    { 0x1002, 0x7106, 2, RHD_R500},
    { 0x1002, 0x7108, 2, RHD_R500},
    { 0x1002, 0x7109, 2, RHD_R500},
    { 0x1002, 0x710A, 2, RHD_R500},
    { 0x1002, 0x710B, 2, RHD_R500},
    { 0x1002, 0x710C, 2, RHD_R500},
    { 0x1002, 0x710E, 2, RHD_R500},
    { 0x1002, 0x710F, 2, RHD_R500},
    { 0x1002, 0x7140, 2, RHD_R500},
    { 0x1002, 0x7142, 2, RHD_R500},
    { 0x1002, 0x7143, 2, RHD_R500},
    { 0x1002, 0x7145, 2, RHD_R500},
    { 0x1002, 0x7146, 2, RHD_R500},
    { 0x1002, 0x7147, 2, RHD_R500},
    { 0x1002, 0x7149, 2, RHD_R500},
    { 0x1002, 0x714A, 2, RHD_R500},
    { 0x1002, 0x714B, 2, RHD_R500},
    { 0x1002, 0x714C, 2, RHD_R500},
    { 0x1002, 0x714D, 2, RHD_R500},
    { 0x1002, 0x714E, 2, RHD_R500},
    { 0x1002, 0x7152, 2, RHD_R500},
    { 0x1002, 0x7153, 2, RHD_R500},
    { 0x1002, 0x715E, 2, RHD_R500},
    { 0x1002, 0x715F, 2, RHD_R500},
    { 0x1002, 0x7180, 2, RHD_R500},
    { 0x1002, 0x7181, 2, RHD_R500},
    { 0x1002, 0x7183, 2, RHD_R500},
    { 0x1002, 0x7186, 2, RHD_R500},
    { 0x1002, 0x7187, 2, RHD_R500},
    { 0x1002, 0x7188, 2, RHD_R500},
    { 0x1002, 0x718A, 2, RHD_R500},
    { 0x1002, 0x718B, 2, RHD_R500},
    { 0x1002, 0x718C, 2, RHD_R500},
    { 0x1002, 0x718D, 2, RHD_R500},
    { 0x1002, 0x718F, 2, RHD_R500},
    { 0x1002, 0x7193, 2, RHD_R500},
    { 0x1002, 0x7196, 2, RHD_R500},
    { 0x1002, 0x719B, 2, RHD_R500},
    { 0x1002, 0x719F, 2, RHD_R500},
    { 0x1002, 0x71C0, 2, RHD_R500},
    { 0x1002, 0x71C1, 2, RHD_R500},
    { 0x1002, 0x71C2, 2, RHD_R500},
    { 0x1002, 0x71C3, 2, RHD_R500},
    { 0x1002, 0x71C4, 2, RHD_R500},
    { 0x1002, 0x71C5, 2, RHD_R500},
    { 0x1002, 0x71C6, 2, RHD_R500},
    { 0x1002, 0x71C7, 2, RHD_R500},
    { 0x1002, 0x71CD, 2, RHD_R500},
    { 0x1002, 0x71CE, 2, RHD_R500},
    { 0x1002, 0x71D2, 2, RHD_R500},
    { 0x1002, 0x71D4, 2, RHD_R500},
    { 0x1002, 0x71D5, 2, RHD_R500},
    { 0x1002, 0x71D6, 2, RHD_R500},
    { 0x1002, 0x71DA, 2, RHD_R500},
    { 0x1002, 0x71DE, 2, RHD_R500},
    { 0x1002, 0x7210, 2, RHD_R500},
    { 0x1002, 0x7211, 2, RHD_R500},
    { 0x1002, 0x7240, 2, RHD_R500},
    { 0x1002, 0x7243, 2, RHD_R500},
    { 0x1002, 0x7244, 2, RHD_R500},
    { 0x1002, 0x7245, 2, RHD_R500},
    { 0x1002, 0x7246, 2, RHD_R500},
    { 0x1002, 0x7247, 2, RHD_R500},
    { 0x1002, 0x7248, 2, RHD_R500},
    { 0x1002, 0x7249, 2, RHD_R500},
    { 0x1002, 0x724A, 2, RHD_R500},
    { 0x1002, 0x724B, 2, RHD_R500},
    { 0x1002, 0x724C, 2, RHD_R500},
    { 0x1002, 0x724D, 2, RHD_R500},
    { 0x1002, 0x724E, 2, RHD_R500},
    { 0x1002, 0x724F, 2, RHD_R500},
    { 0x1002, 0x7280, 2, RHD_R500},
    { 0x1002, 0x7284, 2, RHD_R500},
    { 0x1002, 0x7288, 2, RHD_R500},
    { 0x1002, 0x7291, 2, RHD_R500},
    { 0x1002, 0x7293, 2, RHD_R500},
    { 0x1002, 0x791E, 2, RHD_R500},
    { 0x1002, 0x791F, 2, RHD_R500},
    { 0x1002, 0x9400, 2, RHD_R600},
    { 0x1002, 0x94C1, 2, RHD_R600},
    { 0x1002, 0x94C3, 2, RHD_R600},
    { 0x1002, 0x94C8, 2, RHD_R600},
    { 0x1002, 0x94C9, 2, RHD_R600},
    { 0x1002, 0x9581, 2, RHD_R600},
    { 0x1002, 0x9583, 2, RHD_R600},
    { 0x1002, 0x9588, 2, RHD_R600},
    { 0x1002, 0x9589, 2, RHD_R600},
    { 0, 0, 0, 0 }
};

/*
 *
 */
static struct pci_dev *
device_locate(struct pci_dev *devices, int bus, int dev, int func)
{
    struct pci_dev *device;

    for (device = devices; device; device = device->next)
	if ((device->bus == bus) && (device->dev == dev) &&
	    (device->func == func))
	    return device;
    return NULL;
}

/*
 *
 */
static struct rhd_device *
device_match(struct pci_dev *device)
{
    int i;

    for (i = 0; rhd_devices[i].vendor; i++)
	if ((rhd_devices[i].vendor == device->vendor_id) &&
	    (rhd_devices[i].device == device->device_id))
	    return (rhd_devices + i);

    return NULL;
}

/*
 *
 */
static void *
map_bar(struct pci_dev *device, int io_bar, int dev_mem)
{
    void *map;

    if (!device->base_addr[io_bar] || !device->size[io_bar])
	return NULL;

    map = mmap(0, device->size[io_bar], PROT_WRITE | PROT_READ, MAP_SHARED,
	       dev_mem, device->base_addr[io_bar]);
    /* printf("Mapped IO at 0x%08llX (BAR %1d: 0x%08llX)\n",
       device->base_addr[io_bar], io_bar, device->size[io_bar]); */

    return map;
}

/*
 *
 */
unsigned int
RegRead(void *map, int offset)
{
    return *(volatile unsigned int *)((unsigned char *) map + offset);
}

/*
 *
 */
void
RegWrite(void *map, int offset, unsigned int value)
{
    *(volatile unsigned int *)((unsigned char *) map + offset) = value;
}

/*
 *
 */
void
RegMask(void *map, int offset, unsigned int value, unsigned int mask)
{
    unsigned int tmp;

    tmp = RegRead(map, offset);
    tmp &= ~mask;
    tmp |= (value & mask);
    RegWrite(map, offset, tmp);
}

/*
 *
 */
static void
HPDReport(void *map)
{
    int HPD = RegRead(map, DC_GPIO_HPD_Y);

    printf("  HotPlug:");
    if (!(HPD & 0x00101) && !((ChipType == RHD_R600) && (HPD & 0x10000)))
	printf(" RHD_HPD_NONE ");
    else {
	if (HPD & 0x00001)
	    printf(" RHD_HPD_0");

	if (HPD & 0x00100)
	    printf(" RHD_HPD_1");

	if ((ChipType == RHD_R600) && (HPD & 0x10000))
	    printf(" RHD_HPD_2");
    }
    printf("\n");
}

/*
 *
 */
static Bool
DACALoadDetect(void *map)
{
    unsigned int CompEnable, Control1, Control2, DetectControl, Enable;
    unsigned char ret;

    CompEnable = RegRead(map, DACA_COMPARATOR_ENABLE);
    Control1 = RegRead(map, DACA_CONTROL1);
    Control2 = RegRead(map, DACA_CONTROL2);
    DetectControl = RegRead(map, DACA_AUTODETECT_CONTROL);
    Enable = RegRead(map, DACA_ENABLE);

    RegWrite(map, DACA_ENABLE, 1);
    RegMask(map, DACA_AUTODETECT_CONTROL, 0, 0x00000003);
    RegMask(map, DACA_CONTROL2, 0, 0x00000001);

    RegMask(map, DACA_CONTROL2, 0, 0x00000100);

    RegWrite(map, DACA_FORCE_DATA, 0);
    RegMask(map, DACA_CONTROL2, 0x00000001, 0x0000001);

    RegMask(map, DACA_COMPARATOR_ENABLE, 0x00070000, 0x00070000);
    RegWrite(map, DACA_CONTROL1, 0x00050802);
    RegMask(map, DACA_POWERDOWN, 0, 0x00000001); /* Shut down Bandgap Voltage Reference Power */
    usleep(5);

    RegMask(map, DACA_POWERDOWN, 0, 0x01010100); /* Shut down RGB */

    RegWrite(map, DACA_FORCE_DATA, 0x1e6); /* 486 out of 1024 */
    usleep(200);

    RegMask(map, DACA_POWERDOWN, 0x01010100, 0x01010100); /* Enable RGB */
    usleep(88);

    RegMask(map, DACA_POWERDOWN, 0, 0x01010100); /* Shut down RGB */

    RegMask(map, DACA_COMPARATOR_ENABLE, 0x00000100, 0x00000100);
    usleep(100);

    /* Get RGB detect values
     * If only G is detected, we could have a monochrome monitor,
     * but we don't bother with this at the moment.
     */
    ret = (RegRead(map, DACA_COMPARATOR_OUTPUT) & 0x0E) >> 1;

    RegMask(map, DACA_COMPARATOR_ENABLE, CompEnable, 0x00FFFFFF);
    RegWrite(map, DACA_CONTROL1, Control1);
    RegMask(map, DACA_CONTROL2, Control2, 0x000001FF);
    RegMask(map, DACA_AUTODETECT_CONTROL, DetectControl, 0x000000FF);
    RegMask(map, DACA_ENABLE, Enable, 0x000000FF);

    return (ret & 0x07);
}

/*
 *
 */
static Bool
DACBLoadDetect(void *map)
{
    unsigned int CompEnable, Control1, Control2, DetectControl, Enable;
    unsigned char ret;

    CompEnable = RegRead(map, DACB_COMPARATOR_ENABLE);
    Control1 = RegRead(map, DACB_CONTROL1);
    Control2 = RegRead(map, DACB_CONTROL2);
    DetectControl = RegRead(map, DACB_AUTODETECT_CONTROL);
    Enable = RegRead(map, DACB_ENABLE);

    RegWrite(map, DACB_ENABLE, 1);
    RegMask(map, DACB_AUTODETECT_CONTROL, 0, 0x00000003);
    RegMask(map, DACB_CONTROL2, 0, 0x00000001);

    RegMask(map, DACB_CONTROL2, 0, 0x00000100);

    RegWrite(map, DACB_FORCE_DATA, 0);
    RegMask(map, DACB_CONTROL2, 0x00000001, 0x0000001);

    RegMask(map, DACB_COMPARATOR_ENABLE, 0x00070000, 0x00070000);
    RegWrite(map, DACB_CONTROL1, 0x00050802);
    RegMask(map, DACB_POWERDOWN, 0, 0x00000001); /* Shut down Bandgap Voltage Reference Power */
    usleep(5);

    RegMask(map, DACB_POWERDOWN, 0, 0x01010100); /* Shut down RGB */

    RegWrite(map, DACB_FORCE_DATA, 0x1e6); /* 486 out of 1024 */
    usleep(200);

    RegMask(map, DACB_POWERDOWN, 0x01010100, 0x01010100); /* Enable RGB */
    usleep(88);

    RegMask(map, DACB_POWERDOWN, 0, 0x01010100); /* Shut down RGB */

    RegMask(map, DACB_COMPARATOR_ENABLE, 0x00000100, 0x00000100);
    usleep(100);

    /* Get RGB detect values
     * If only G is detected, we could have a monochrome monitor,
     * but we don't bother with this at the moment.
     */
    ret = (RegRead(map, DACB_COMPARATOR_OUTPUT) & 0x0E) >> 1;

    RegMask(map, DACB_COMPARATOR_ENABLE, CompEnable, 0x00FFFFFF);
    RegWrite(map, DACB_CONTROL1, Control1);
    RegMask(map, DACB_CONTROL2, Control2, 0x000001FF);
    RegMask(map, DACB_AUTODETECT_CONTROL, DetectControl, 0x000000FF);
    RegMask(map, DACB_ENABLE, Enable, 0x000000FF);

    return (ret & 0x07);
}

/*
 *
 */
static Bool
TMDSALoadDetect(void *map)
{
    unsigned int Enable, Control, Detect;
    Bool ret;

    Enable = RegRead(map, TMDSA_TRANSMITTER_ENABLE);
    Control = RegRead(map, TMDSA_TRANSMITTER_CONTROL);
    Detect = RegRead(map, TMDSA_LOAD_DETECT);

    /* r500 needs a tiny bit more work :) */
    if (ChipType < RHD_R600) {
	RegMask(map, TMDSA_TRANSMITTER_ENABLE, 0x00000003, 0x00000003);
	RegMask(map, TMDSA_TRANSMITTER_CONTROL, 0x00000001, 0x00000003);
    }

    RegMask(map, TMDSA_LOAD_DETECT, 0x00000001, 0x00000001);
    usleep(1);
    ret = RegRead(map, TMDSA_LOAD_DETECT) & 0x00000010;
    RegMask(map, TMDSA_LOAD_DETECT, Detect, 0x00000001);

    if (ChipType < RHD_R600) {
	RegWrite(map, TMDSA_TRANSMITTER_ENABLE, Enable);
	RegWrite(map, TMDSA_TRANSMITTER_CONTROL, Control);
    }

    return ret;
}

/*
 *
 */
static void
LoadReport(void *map)
{
    Bool DACA, DACB, TMDSA;

    DACA = DACALoadDetect(map);
    DACB = DACBLoadDetect(map);
    TMDSA =TMDSALoadDetect(map);

    printf("  Load Detection:");
    if (!DACA && !DACB && !TMDSA)
	printf(" RHD_OUTPUT_NONE ");
    else {
	if (DACA)
	    printf(" RHD_OUTPUT_DACA");

	if (DACB)
	    printf(" RHD_OUTPUT_DACB");

	if (TMDSA)
	    printf(" RHD_OUTPUT_TMDSA");
    }
    printf("\n");
}

/*
 * R600 DDC defines.
 */
enum {
    /* DC_I2C_TRANSACTION0 */
    DC_I2C_RW0   = (0x1 << 0),
    DC_I2C_STOP_ON_NACK0         = (0x1 << 8),
    DC_I2C_ACK_ON_READ0  = (0x1 << 9),
    DC_I2C_START0        = (0x1 << 12),
    DC_I2C_STOP0         = (0x1 << 13),
    DC_I2C_COUNT0        = (0xff << 16),
    /* DC_I2C_TRANSACTION1 */
    DC_I2C_RW1   = (0x1 << 0),
    DC_I2C_STOP_ON_NACK1         = (0x1 << 8),
    DC_I2C_ACK_ON_READ1  = (0x1 << 9),
    DC_I2C_START1        = (0x1 << 12),
    DC_I2C_STOP1         = (0x1 << 13),
    DC_I2C_COUNT1        = (0xff << 16),
    /* DC_I2C_DATA */
    DC_I2C_DATA_RW       = (0x1 << 0),
    DC_I2C_DATA_BIT      = (0xff << 8),
    DC_I2C_INDEX         = (0xff << 16),
    DC_I2C_INDEX_WRITE   = (0x1 << 31),
    /* DC_I2C_CONTROL */
    DC_I2C_GO    = (0x1 << 0),
    DC_I2C_SOFT_RESET    = (0x1 << 1),
    DC_I2C_SEND_RESET    = (0x1 << 2),
    DC_I2C_SW_STATUS_RESET       = (0x1 << 3),
    DC_I2C_SDVO_EN       = (0x1 << 4),
    DC_I2C_SDVO_ADDR_SEL         = (0x1 << 6),
    DC_I2C_DDC_SELECT    = (0x7 << 8),
    DC_I2C_TRANSACTION_COUNT     = (0x3 << 20),
    DC_I2C_SW_DONE_INT   = (0x1 << 0),
    DC_I2C_SW_DONE_ACK   = (0x1 << 1),
    DC_I2C_SW_DONE_MASK  = (0x1 << 2),
    DC_I2C_DDC1_HW_DONE_INT      = (0x1 << 4),
    DC_I2C_DDC1_HW_DONE_ACK      = (0x1 << 5),
    DC_I2C_DDC1_HW_DONE_MASK     = (0x1 << 6),
    DC_I2C_DDC2_HW_DONE_INT      = (0x1 << 8),
    DC_I2C_DDC2_HW_DONE_ACK      = (0x1 << 9),
    DC_I2C_DDC2_HW_DONE_MASK     = (0x1 << 10),
    DC_I2C_DDC3_HW_DONE_INT      = (0x1 << 12),
    DC_I2C_DDC3_HW_DONE_ACK      = (0x1 << 13),
    DC_I2C_DDC3_HW_DONE_MASK     = (0x1 << 14),
    DC_I2C_DDC4_HW_DONE_INT      = (0x1 << 16),
    DC_I2C_DDC4_HW_DONE_ACK      = (0x1 << 17),
    DC_I2C_DDC4_HW_DONE_MASK     = (0x1 << 18),
    /* DC_I2C_SW_STATUS */
    DC_I2C_SW_STATUS_BIT         = (0x3 << 0),
    DC_I2C_SW_DONE       = (0x1 << 2),
    DC_I2C_SW_ABORTED    = (0x1 << 4),
    DC_I2C_SW_TIMEOUT    = (0x1 << 5),
    DC_I2C_SW_INTERRUPTED        = (0x1 << 6),
    DC_I2C_SW_BUFFER_OVERFLOW    = (0x1 << 7),
    DC_I2C_SW_STOPPED_ON_NACK    = (0x1 << 8),
    DC_I2C_SW_SDVO_NACK  = (0x1 << 10),
    DC_I2C_SW_NACK0      = (0x1 << 12),
    DC_I2C_SW_NACK1      = (0x1 << 13),
    DC_I2C_SW_NACK2      = (0x1 << 14),
    DC_I2C_SW_NACK3      = (0x1 << 15),
    DC_I2C_SW_REQ        = (0x1 << 18)
};

#define EDID_SLAVE 0xA0
#define I2C_ENGINE_RATE 0x3FF

/*
 *
 */
static Bool
R600I2CSetupStatus(void *map, int channel, int speed)
{
    channel &= 0xf;

    switch (channel) {
    case 0:
	RegMask(map, DC_GPIO_DDC1_MASK, 0x0, 0xffff);
	RegMask(map, DC_GPIO_DDC1_A, 0x0, 0xffff);
	RegMask(map, DC_GPIO_DDC1_EN, 0x0, 0xffff);
	RegMask(map, DC_I2C_DDC1_SPEED, (speed << 16) | 2,
		0xFFFF00FF);
	RegWrite(map, DC_I2C_DDC1_SETUP, 0x30000000);
	break;
    case 1:
	RegMask(map, DC_GPIO_DDC2_MASK, 0x0, 0xffff);
	RegMask(map, DC_GPIO_DDC2_A, 0x0, 0xffff);
	RegMask(map, DC_GPIO_DDC2_EN, 0x0, 0xffff);
	RegMask(map, DC_I2C_DDC2_SPEED, (speed << 16) | 2,
		0xffff00ff);
	RegWrite(map, DC_I2C_DDC2_SETUP, 0x30000000);
	break;
    case 2:
	RegMask(map, DC_GPIO_DDC3_MASK, 0x0, 0xffff);
	RegMask(map, DC_GPIO_DDC3_A, 0x0, 0xffff);
	RegMask(map, DC_GPIO_DDC3_EN, 0x0, 0xffff);
	RegMask(map, DC_I2C_DDC3_SPEED, (speed << 16) | 2,
		0xffff00ff);
	RegWrite(map, DC_I2C_DDC3_SETUP, 0x30000000);
	break;
    case 3:
	RegMask(map, DC_GPIO_DDC4_MASK, 0x0, 0xffff);
	RegMask(map, DC_GPIO_DDC4_A, 0x0, 0xffff);
	RegMask(map, DC_GPIO_DDC4_EN, 0x0, 0xffff);
	RegMask(map, DC_I2C_DDC4_SPEED, (speed << 16) | 2,
		0xffff00ff);
	RegWrite(map, DC_I2C_DDC4_SETUP, 0x30000000);
	break;
    default:
	return FALSE;
    }
    RegWrite(map, DC_I2C_CONTROL, channel << 8);
    RegMask(map, DC_I2C_INTERRUPT_CONTROL, 0x00000002, 0x00000002);
    RegMask(map, DC_I2C_ARBITRATION, 0, 0x00);
    return TRUE;
}

/*
 *
 */
static Bool
R600I2CStatus(void *map)
{
    int count = 0x1388;
    unsigned int val;

    while (--count) {

	usleep(1000);
	val = RegRead(map, DC_I2C_SW_STATUS);

	if (val & DC_I2C_SW_DONE)
	    break;
    }
    RegMask(map, DC_I2C_INTERRUPT_CONTROL, DC_I2C_SW_DONE_ACK, DC_I2C_SW_DONE_ACK);

    if (!count || (val & 0x7b3))
	return FALSE; /* 2 */
    return TRUE; /* 1 */
}

/*
 *
 */
static Bool
R600DDCProbe(void *map, int Channel)
{
    Bool ret = FALSE;

    if (!R600I2CSetupStatus(map, Channel, I2C_ENGINE_RATE))
	return FALSE;

    RegMask(map, DC_I2C_CONTROL, 0, 0x00300000);

    /* 1 Transaction */
    RegMask(map, DC_I2C_TRANSACTION0, /* only slave */
	    DC_I2C_STOP_ON_NACK0 | DC_I2C_START0
	    | DC_I2C_STOP0 | (0 << 16), 0xffffff);

    RegWrite(map, DC_I2C_DATA,
	     DC_I2C_INDEX_WRITE | ( EDID_SLAVE << 8 ) | (0 << 16));
    RegMask(map, DC_I2C_CONTROL, DC_I2C_GO, DC_I2C_GO);

    ret = R600I2CStatus(map);

    RegMask(map, DC_I2C_CONTROL, 0x2, 0xff);
    usleep(1000);
    RegWrite(map, DC_I2C_CONTROL, 0);

    return ret;
}

enum {
    R5XX_DC_I2C_STATUS1           = 0x7D30,
    R5XX_DC_I2C_RESET             = 0x7D34,
    R5XX_DC_I2C_CONTROL1          = 0x7D38,
    R5XX_DC_I2C_CONTROL2          = 0x7D3C,
    R5XX_DC_I2C_CONTROL3          = 0x7D40,
    R5XX_DC_I2C_DATA              = 0x7D44,
    R5XX_DC_I2C_INTERRUPT_CONTROL = 0x7D48,
    R5XX_DC_I2C_ARBITRATION       = 0x7D50,
};

/*
 *
 */
static Bool
R500I2CStatus(void *map)
{
    int count = 32;
    unsigned int res;

    while (count-- != 0) {
	usleep (1000);

	if (((RegRead(map, R5XX_DC_I2C_STATUS1)) & 0x08) != 0)
	    continue;
	res = RegRead(map, R5XX_DC_I2C_STATUS1);

	if (res & 0x1)
	    return TRUE;
	else
	    return FALSE;
    }

    RegMask(map, R5XX_DC_I2C_RESET, 0x1 << 8, 0xff00);
    return FALSE;
}

/*
 *
 */
static Bool
R500DDCProbe(void *map, int Channel)
{
    Bool ret;
    unsigned int SaveControl1, save_494;

    RegMask(map, 0x28, 0x00000200, 0x00000200);

    SaveControl1 = RegRead(map, R5XX_DC_I2C_CONTROL1);
    save_494 = RegRead(map, 0x494);

    RegMask(map, 0x494, 1, 1); /* ???? */
    RegMask(map, R5XX_DC_I2C_ARBITRATION, 1, 1);

    RegMask(map, R5XX_DC_I2C_STATUS1, 0x7, 0xff);
    RegMask(map, R5XX_DC_I2C_RESET, 0x1, 0xffff);
    RegWrite(map, R5XX_DC_I2C_RESET, 0);

    RegMask(map, R5XX_DC_I2C_CONTROL1,
	    (Channel & 0x0f) << 16 | 0x100, 0xff0100);
    RegWrite(map, R5XX_DC_I2C_CONTROL2, I2C_ENGINE_RATE << 16 | 0x101);
    RegMask(map, R5XX_DC_I2C_CONTROL3, 0x30 << 24, 0xff << 24);

    RegWrite(map, R5XX_DC_I2C_DATA, EDID_SLAVE);  /* slave */
    RegWrite(map, R5XX_DC_I2C_DATA, 0);  /* slave */

    RegMask(map, R5XX_DC_I2C_CONTROL1, 0x3, 0xff);
    RegMask(map, R5XX_DC_I2C_STATUS1, 0x8, 0xff);
    ret = R500I2CStatus(map);

    RegMask(map, R5XX_DC_I2C_STATUS1, 0x07, 0xff);
    RegMask(map, R5XX_DC_I2C_RESET, 0x01, 0xff);
    RegWrite(map,R5XX_DC_I2C_RESET, 0);

    RegMask(map, R5XX_DC_I2C_ARBITRATION, 0x100, 0xff00);

    RegWrite(map, R5XX_DC_I2C_CONTROL1, SaveControl1);
    RegWrite(map, 0x494, save_494);
    RegMask(map, 0x28, 0, 0x00000200);

    return ret;
}

/*
 *
 */
static Bool
DDCProbe(void *map, int Channel)
{
    if (ChipType == RHD_R600)
	return R600DDCProbe(map, Channel);
    else
	return R500DDCProbe(map, Channel);
}

/*
 *
 */
static void
DDCReport(void *map)
{
    Bool Chan0, Chan1, Chan2, Chan3;

    Chan0 = DDCProbe(map, 0);
    Chan1 = DDCProbe(map, 1);
    Chan2 = DDCProbe(map, 2);
    Chan3 = DDCProbe(map, 3);

    printf("  DDC:");
    if (!Chan0 && !Chan1 && !Chan2 && !Chan3)
	printf(" RHD_DDC_NONE ");
    else {
	if (Chan0)
	    printf(" RHD_DDC_0");

	if (Chan1)
	    printf(" RHD_DDC_1");

	if (Chan2)
	    printf(" RHD_DDC_2");

	if (Chan3)
	    printf(" RHD_DDC_3");
    }
    printf("\n");

}

/*
 *
 */
static void
LVDSReport(void *map)
{
    Bool Bits24 = FALSE, DualLink = FALSE, Fpdi = FALSE;

    if (ChipType == RHD_R600) {
	/* printf("No information for LVTMA on R600 has been made available yet.\n"); */
	return;
    }

    if (!(RegRead(map, LVTMA_CNTL) & 0x00000001) &&
	(RegRead(map, LVTMA_MODE) & 0x00000001))
	return;

    printf("  LVDS Info:\n");

    DualLink = RegRead(map, LVTMA_CNTL) & 0x01000000;
    Bits24 = RegRead(map, LVTMA_LVDS_DATA_CNTL) & 0x00000001;
    Fpdi = RegRead(map, LVTMA_LVDS_DATA_CNTL) & 0x00000001;

    printf("\t%dbits, %s link, %s Panel found.\n",
	   Bits24 ? 24 : 18,
	   DualLink ? "dual" : "single",
	   Fpdi ? "FPDI" : "LDI");

    printf("\tPower Timing: 0x%03X, 0x%03X, 0x%02X, 0x%02X, 0x%03X\n",
	   RegRead(map, LVTMA_PWRSEQ_REF_DIV) & 0x00000FFF,
	   (RegRead(map, LVTMA_PWRSEQ_REF_DIV) >> 16) & 0x00000FFF,
	   ((RegRead(map, LVTMA_PWRSEQ_DELAY1) & 0xFF) * 2 + 1) / 5,
	   (((RegRead(map, LVTMA_PWRSEQ_DELAY1) >> 8) & 0xFF) * 2 + 1)/ 5,
	   (RegRead(map, LVTMA_PWRSEQ_DELAY2) & 0xFFF) << 2);

    printf("\tMacro: 0x%08X, Clock Pattern: 0x%04X\n",
	   RegRead(map, LVTMA_MACRO_CONTROL),
	   (RegRead(map, LVTMA_TRANSMITTER_CONTROL) >> 16) & 0x3FF);
}

/*
 *
 */
int
main(int argc, char *argv[])
{
    struct pci_dev *device;
    struct pci_access *pci_access;
    struct rhd_device *rhd_device;
    int dev_mem;
    void *io;
    unsigned int bus, dev, func;
    int ret;

    /* init libpci */
    pci_access = pci_alloc();
    pci_init(pci_access);
    pci_scan_bus(pci_access);

    if (argc < 2) {
	fprintf(stderr, "Missing argument: please provide a PCI tag "
		"of the form: bus:dev.func\n");
	return 1;
    }

    ret = sscanf(argv[1], "%x:%x.%x", &bus, &dev, &func);
    if (ret != 3) {
	ret = sscanf(argv[1], "%x:%x:%x", &bus, &dev, &func);
	if (ret != 3) {
	    ret = sscanf(argv[1], "%d:%d.%d", &bus, &dev, &func);
	    if (ret != 3)
		ret = sscanf(argv[1], "%d:%d:%d", &bus, &dev, &func);
	}
    }

    if (ret != 3) {
	fprintf(stderr, "Unable to parse the PCI tag argument (%s)."
		" Please use: bus:dev.func\n", argv[1]);
	return 1;
    }

    /* find our toy */
    device = device_locate(pci_access->devices, bus, dev, func);
    if (!device) {
	fprintf(stderr, "Unable to find PCI device at %02X:%02X.%02X.\n",
		bus, dev, func);
	return 1;
    }

    rhd_device = device_match(device);
    if (!rhd_device) {
	fprintf(stderr, "Unknown device: 0x%04X:0x%04X (%02X:%02X.%02X).\n",
		device->vendor_id, device->device_id, bus, dev, func);
	return 1;
    }

    if (rhd_device->bar > 5) {
	fprintf(stderr, "Program error: No acceptable BAR defined for this device.\n");
	return 1;
    }

    printf("Checking connectors on 0x%04X, 0x%04X, 0x%04X  (@%02X:%02X:%02X):\n",
	   device->device_id, pci_read_word(device, PCI_SUBSYSTEM_VENDOR_ID),
	   pci_read_word(device, PCI_SUBSYSTEM_ID),
	   device->bus, device->dev, device->func);

    /* make sure we can actually read /dev/mem before we do anything else */
    dev_mem = open("/dev/mem", O_RDWR);
    if (dev_mem < 0) {
	fprintf(stderr, "Unable to open /dev/mem: %s.\n", strerror(errno));
	return errno;
    }

    /* find the G-Spot */
    io = map_bar(device, rhd_device->bar, dev_mem);
    if (!io) {
	fprintf(stderr, "Unable to map IO memory.\n");
	return 1;
    }

    ChipType = rhd_device->type;

    LoadReport(io);
    HPDReport(io);
    DDCReport(io);

    LVDSReport(io);

    return 0;
}

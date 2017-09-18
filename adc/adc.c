/*
 * MAX11632 - Maxim 12-Bit SPI ADC Linux driver
 *
 * Yarib Nevárez <yarib_007@hotmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/kernel.h>
#include <asm/uaccess.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <asm/uaccess.h>    /* Needed for copy_from_user */
#include <asm/io.h>         /* Needed for IO Read/Write Functions */
#include <linux/proc_fs.h>  /* Needed for Proc File System Functions */
#include <linux/seq_file.h> /* Needed for Sequence File Operations */
#include <linux/platform_device.h>  /* Needed for Platform Driver Functions */
#include <asm/uaccess.h>

#include <linux/ctype.h>
#include <linux/fs.h>         
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fcntl.h>

#include "../../Hardware/ip_repo/ESPI_1.0/src/espi_hw.h"
#include "/home/yarib/ZYBO_projects/app-workspace/framework/framework/iodef.hpp"
#include "/home/yarib/ZYBO_projects/app-workspace/framework/framework/deviceid.hpp"

/* Define Driver Name */
#define DRIVER_NAME "adc"

#define SUCCESS     0


static unsigned long * base_addr = NULL;
static struct resource * res = NULL;
static unsigned long remap_size = 0;
static int major;       /* major number we get from the kernel */
static u8 ADC_chanel = 0;
static DeviceID device_ID = 0;

static void outport(u8 offset, u32 data)
{
    wmb();
    iowrite32(data, base_addr + offset);
}

static u32 inport(u8 offset)
{
    wmb();
    return ioread32(base_addr + offset);
}


///////////////////////////////////////////////////////////////////////////////

static void spi_set_baud_rate_divider(u8 devider)
{
    u32 reg;
    do
    {
        reg = inport(SPI_CONTROL_REGISTER_INDEX);
    }
    while(!(reg & SPI_TRANSMISSION_DONE_MASK));
    REGISTER_SET(reg, SPI_BAUD_RATE_DIVIDER_MASK, SPI_BAUD_RATE_DIVIDER_SHIFT, devider);
    outport(SPI_CONTROL_REGISTER_INDEX, reg);
}

static void spi_set_clock_phase(u8 val)
{
    u32 reg;
    do
    {
        reg = inport(SPI_CONTROL_REGISTER_INDEX);
    }
    while(!(reg & SPI_TRANSMISSION_DONE_MASK));
    REGISTER_SET(reg, SPI_CLOCK_PHASE_MASK, SPI_CLOCK_PHASE_SHIFT, val);
    outport(SPI_CONTROL_REGISTER_INDEX, reg);
}

static void spi_set_clock_polarity(u8 val)
{
    u32 reg;
    do
    {
        reg = inport(SPI_CONTROL_REGISTER_INDEX);
    }
    while(!(reg & SPI_TRANSMISSION_DONE_MASK));
    REGISTER_SET(reg, SPI_CLOCK_POLARITY_MASK, SPI_CLOCK_POLARITY_SHIFT, val);
    outport(SPI_CONTROL_REGISTER_INDEX, reg);
}

static void spi_set_data_length(u8 val)
{
    u32 reg;
    do
    {
        reg = inport(SPI_CONTROL_REGISTER_INDEX);
    }
    while(!(reg & SPI_TRANSMISSION_DONE_MASK));
    REGISTER_SET(reg, SPI_DATA_LENGTH_MASK, SPI_DATA_LENGTH_SHIFT, val);
    outport(SPI_CONTROL_REGISTER_INDEX, reg);
}

static void spi_set_cs_force(u8 val)
{
    u32 reg;
    do
    {
        reg = inport(SPI_CONTROL_REGISTER_INDEX);
    }
    while(!(reg & SPI_TRANSMISSION_DONE_MASK));
    REGISTER_SET(reg, SPI_CS_FORCE_MASK, SPI_CS_FORCE_SHIFT, val);
    outport(SPI_CONTROL_REGISTER_INDEX, reg);
}

static void spi_set_settle_time(u8 val)
{
    u32 reg;
    do
    {
        reg = inport(SPI_CONTROL_REGISTER_INDEX);
    }
    while(!(reg & SPI_TRANSMISSION_DONE_MASK));
    REGISTER_SET(reg, SPI_SETTLE_TIME_MASK, SPI_SETTLE_TIME_SHIFT, val);
    outport(SPI_CONTROL_REGISTER_INDEX, reg);
}

static u8 spi_get_transmission_done(void)
{
    u32 reg = inport(SPI_CONTROL_REGISTER_INDEX);
    return REGISTER_GET(reg, SPI_TRANSMISSION_DONE_MASK, SPI_TRANSMISSION_DONE_SHIFT);
}

static void spi_set_slave_select(u8 val)
{
    u32 reg;
    do
    {
        reg = inport(SPI_CONTROL_REGISTER_INDEX);
    }
    while(!(reg & SPI_TRANSMISSION_DONE_MASK));
    REGISTER_SET(reg, SPI_SLAVE_SELECT_MASK, SPI_SLAVE_SELECT_SHIFT, val);
    outport(SPI_CONTROL_REGISTER_INDEX, reg);
}

static u32 spi_receive_data(void)
{
    u32 reg;
    do
    {
        reg = inport(SPI_CONTROL_REGISTER_INDEX);
    }
    while(!(reg & SPI_TRANSMISSION_DONE_MASK));
    return inport(SPI_DATA_REGISTER_INDEX);
}

static void spi_send_data(u32 data)
{
    u32 reg;
    do
    {
        reg = inport(SPI_CONTROL_REGISTER_INDEX);
    }
    while(!(reg & SPI_TRANSMISSION_DONE_MASK));
    outport(SPI_DATA_REGISTER_INDEX, data);
}
///////////////////////////////////////////////////////////////////////////////
void spi_initialize(void)
{
    spi_set_baud_rate_divider(0x10); // AXI_CLK_FREC = 50000000
    spi_set_clock_phase(0);
    spi_set_clock_polarity(0);

    spi_set_cs_force(0);
    spi_set_settle_time(0);
    spi_set_slave_select(0);

    spi_set_data_length(SPI_DATA_LENGTH_8_BITS);

    spi_send_data(0x10);
    spi_send_data(0x20);
    spi_send_data(0x78);

    spi_set_data_length(SPI_DATA_LENGTH_24_BITS);
}

u32 adc_read(u8 channel)
{
    spi_send_data((0x80 | channel << 3 | 0x6)<<16);
    return spi_receive_data();
}
///////////////////////////////////////////////////////////////////////////////

/* Write operation for /proc/driver
* -----------------------------------
*/
static ssize_t proc_driver_write(struct file *file,
                                 const char __user * buffer,
                                 size_t buffer_size,
                                 loff_t * position)
{
    ssize_t written_size = 0;

    if ((buffer != NULL) && (*buffer != 0) && (buffer_size == sizeof(DeviceID))
        && (ADC_DEVICE_ID_BEGIN < *buffer) && (*buffer < ADC_DEVICE_ID_END))
    {
        written_size = buffer_size - copy_from_user((void *) &device_ID, buffer, sizeof(device_ID));

        switch (device_ID)
        {
            case LEVELSENSOR_0:
            ADC_chanel = 0;
            break;

            case LEVELSENSOR_1:
            ADC_chanel = 1;
            break;

            case WASTETLEVEL:
            ADC_chanel = 2;
            break;

            case LEAKSENSOR_0:
            ADC_chanel = 3;
            break;

            case LEAKSENSOR_1:
            ADC_chanel = 4;
            break;

            case ADC_CHANNEL_5:
            ADC_chanel = 5;
            break;

            case ADC_CHANNEL_6:
            ADC_chanel = 6;
            break;

            case ADC_CHANNEL_7:
            ADC_chanel = 7;
            break;

            case ADC_CHANNEL_8:
            ADC_chanel = 8;
            break;

            case ADC_CHANNEL_9:
            ADC_chanel = 9;
            break;

            case ADC_CHANNEL_10:
            ADC_chanel = 10;
            break;

            default:;
        }
    }
    return buffer_size;
}

/* Read operation for /proc/driver
* -----------------------------------
*/
static ssize_t proc_driver_read(struct file *file,
                                 char __user * buffer,
                                 size_t buffer_size,
                                 loff_t * position)
{
    ssize_t read_size = 0;

    if ((buffer != NULL) && (*buffer != 0) && (sizeof(IOPacket) <= buffer_size))
    {
        IOPacket packet;
        packet.device_ID = device_ID;
        packet.data = adc_read(ADC_chanel);
        read_size = buffer_size - copy_to_user(buffer, (void *) &packet, sizeof(IOPacket));
    }
    return read_size;
}


/* Callback function when opening file /proc/driver
* ------------------------------------------------------
*  Read the register value of driver file controller, print the value to
*  the sequence file struct seq_file *p. In file open operation for /proc/driver
*  this callback function will be called first to fill up the seq_file,
*  and seq_read function will print whatever in seq_file to the terminal.
*/
static int proc_driver_show(struct seq_file *p, void *v)
{
    u32 driverValue;
    driverValue = adc_read(ADC_chanel);
    seq_printf(p, "ADC chanel %d = 0x%X", ADC_chanel, driverValue);
    printk(KERN_INFO DRIVER_NAME "ADC chanel %d = 0x%08X\n", ADC_chanel, driverValue);
    return SUCCESS;
}

/* Open function for /proc/driver
* ------------------------------------
*  When user want to read /proc/driver (i.e. cat /proc/driver), the open function 
*  will be called first. In the open function, a seq_file will be prepared and the 
*  status of driver will be filled into the seq_file by proc_driver_show function.
*/
static int driver_open(struct inode *inode, struct file *file)
{
    unsigned int size = 16;
    char * buf;
    struct seq_file * m;
    int rc = SUCCESS;

    buf = (char *)kmalloc(size * sizeof(char), GFP_KERNEL);
    if (buf != NULL)
    {
        rc = single_open(file, proc_driver_show, NULL);

        if (rc == SUCCESS)
        {
            m = file->private_data;
            m->buf = buf;
            m->size = size;
        }
        else
        {
            kfree(buf);
        }
    }
    else
    {
        printk(KERN_ALERT DRIVER_NAME "%s No memory resource\n", __FUNCTION__);
        rc = -ENOMEM;
    }

    return rc;
}

/* File Operations for /proc/driver */
static const struct file_operations proc_driver_operations =
{
    .open           = driver_open,
    .write          = proc_driver_write,
    .read           = proc_driver_read,
    .llseek         = seq_lseek,
    .release        = single_release
};

/* Shutdown function for driver
* -----------------------------------
*  Before driver shutdown, turn-off all the stuff
*/
static void driver_shutdown(struct platform_device *pdev)
{
    spi_set_slave_select(3);
}

/* Remove function for driver
* ----------------------------------
*  When driver module is removed, turn off all the stuff first,
*  release virtual address and the memory region requested.
*/
static int driver_remove(struct platform_device *pdev)
{
    driver_shutdown(pdev);

    /* Remove /proc/driver entry */
    remove_proc_entry(DRIVER_NAME, NULL);

    unregister_chrdev(major, DRIVER_NAME);

    /* Release mapped virtual address */
    iounmap(base_addr);

    /* Release the region */
    release_mem_region(res->start, remap_size);

    return SUCCESS;
}

/* Device Probe function for driver
* ------------------------------------
*  Get the resource structure from the information in device tree.
*  request the memory regioon needed for the controller, and map it into
*  kernel virtual memory space. Create an entry under /proc file system
*  and register file operations for that entry.
*/
static int driver_probe(struct platform_device *pdev)
{
    struct proc_dir_entry * driver_proc_entry;
    int rc = SUCCESS;

    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (!res)
    {
        dev_err(&pdev->dev, "No memory resource\n");
        rc = -ENODEV;
    }

    if (rc == SUCCESS)
    {
        remap_size = res->end - res->start + 1;
        if (!request_mem_region(res->start, remap_size, pdev->name))
        {
            dev_err(&pdev->dev, "Cannot request IO\n");
            rc = -ENXIO;
        }
    }

    if (rc == SUCCESS)
    {
        base_addr = ioremap(res->start, remap_size);
        if (base_addr == NULL)
        {
            dev_err(&pdev->dev, "Couldn't ioremap memory at 0x%08lx\n", (unsigned long)res->start);
            release_mem_region(res->start, remap_size);
            rc = -ENOMEM;
        }
    }

    if (rc == SUCCESS)
    {
        driver_proc_entry = proc_create(DRIVER_NAME, 666, NULL, &proc_driver_operations);
        if (driver_proc_entry == NULL)
        {
            dev_err(&pdev->dev, "Couldn't create proc entry\n");
            iounmap(base_addr);
            release_mem_region(res->start, remap_size);
            rc = -ENOMEM;
        }
    }

    if (rc == SUCCESS)
    {
        printk(KERN_INFO DRIVER_NAME " Maped at virtual address 0x%08lx\n", (unsigned long) base_addr);

        major = register_chrdev(0, DRIVER_NAME, &proc_driver_operations);

        spi_initialize();
    }

    return rc;
}

/* device match table to match with device node in device tree */
static const struct of_device_id zynq_adc_match[] =
{
    {.compatible = "yarib-adc-1.00.a"},
    { },
};

MODULE_DEVICE_TABLE(of, zynq_adc_match);

/* platform driver structure for device driver */
static struct platform_driver zynq_adc_driver =
{
    .driver =
    {
        .name = DRIVER_NAME,
        .owner = THIS_MODULE,
        .of_match_table = zynq_adc_match
    },
    .probe = driver_probe,
    .remove = driver_remove,
    .shutdown = driver_shutdown
};

/* Register device platform driver */
module_platform_driver(zynq_adc_driver);

/* Module Informations */
MODULE_AUTHOR("Yarib, Inc.");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION(DRIVER_NAME ": ADC module");
MODULE_ALIAS(DRIVER_NAME);

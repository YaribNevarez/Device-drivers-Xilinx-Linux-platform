/*
 * AXI GPIO v2.0 - Linux driver
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

#define KERNEL_MODULE
#include "../../app-workspace/base_framework/device/iodef.hpp"
#include "../../app-workspace/extension_framework/deviceid.hpp"

/* Define Driver Name */
#define DRIVER_NAME "controller"

#define SUCCESS     0


static unsigned long *   base_addr   = NULL;
static struct resource * res         = NULL;
static unsigned long     remap_size  = 0;
static DeviceID          device_ID   = 0;
static u32               output_data = 0;

static int major;       /* major number we get from the kernel */

static void configport(u32 data)
{
    static u32 current_data = 0;

    if (current_data != data)
    {
        wmb();
        iowrite32(data, base_addr + 1);
        current_data = data;
    }
}

static void outport(u32 data)
{
    static u32 current_data = 0;

    if (current_data != data)
    {
        wmb();
        iowrite32(data, base_addr);
        current_data = data;
    }
}

static u32 inport(void)
{
    wmb();
    return ioread32(base_addr);
}

#define SET_SLICE_32(register, value, offset, mask) (register) = (((register) & ~((mask) << (offset))) | (((value) & (mask)) << (offset)))
#define GET_SLICE_32(register, offset, mask) (((register) & ((mask) << (offset))) >> (offset))

/* Write operation for /proc/driver
* -----------------------------------
*/
static ssize_t proc_driver_write(struct file *file,
                                 const char __user * buffer,
                                 size_t buffer_size,
                                 loff_t * position)
{
    ssize_t written_size = 0;

    if (buffer != NULL)
    {
        if (buffer_size == sizeof(device_ID))
        {
            written_size = buffer_size - copy_from_user((void *) &device_ID, buffer, sizeof(device_ID));
        }
        else if (buffer_size == sizeof(IOPacket))
        {
            IOPacket packet;
            written_size = buffer_size - copy_from_user((void *) &packet, buffer, sizeof(IOPacket));

            if (written_size == buffer_size )
            {
                device_ID = packet.device_ID;
                switch (packet.device_ID)
                {
                    case IRSENSOR:
                        break;
                    case FLUSHVALVE:
                        SET_SLICE_32(output_data, packet.data, 16, 0x1);
                        break;
                    case DRAINVALVE:
                        SET_SLICE_32(output_data, packet.data, 17, 0x1);
                        break;
                    case SHUTOFFVALVE:
                        SET_SLICE_32(output_data, packet.data, 18, 0x1);
                        break;
                    case DRAINLOCAL:
                        break;
                    case EMERGENCY:
                        break;
                    case APPSELECTION:
                        break;
                    case DRAINDELAY:
                        break;
                    case VACUUMGEN:
                        SET_SLICE_32(output_data, packet.data, 19, 0x1);
                        break;
                    case DRAININDICATOR:
                        SET_SLICE_32(output_data, packet.data, 20, 0x1);
                        break;
                    case LEAKINDICATOR:
                        SET_SLICE_32(output_data, packet.data, 21, 0x1);
                        break;
                    case RELAY_0:
                        SET_SLICE_32(output_data, packet.data, 22, 0x1);
                        break;
                    case RELAY_1:
                        SET_SLICE_32(output_data, packet.data, 23, 0x1);
                        break;
                    default:;
                }
                outport(output_data);
                * position += written_size;
            }
        }
    }
    return written_size;
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

    if ((buffer != NULL) && (sizeof(IOPacket) <= buffer_size))
    {
        IOPacket packet;
        u32 input_data = inport();
        packet.device_ID = device_ID;

        switch (device_ID)
        {
            case FLUSHVALVE:
                packet.data = GET_SLICE_32(input_data, 0, 1);
                break;
            case DRAINVALVE:
                packet.data = GET_SLICE_32(input_data, 1, 1);
                break;
            case SHUTOFFVALVE:
                packet.data = GET_SLICE_32(input_data, 2, 1);
                break;
            case VACUUMGEN:
                packet.data = GET_SLICE_32(input_data, 3, 1);
                break;
            case IRSENSOR:
                packet.data = GET_SLICE_32(input_data, 5, 1);
                break;
            case DRAINLOCAL:
                packet.data = GET_SLICE_32(input_data, 6, 1);
                break;
            case EMERGENCY:
                packet.data = GET_SLICE_32(input_data, 7, 3);
                break;
            case APPSELECTION:
                packet.data = GET_SLICE_32(input_data, 9, 1);
                break;
            case DRAINDELAY:
                packet.data = GET_SLICE_32(input_data, 10, 7);
                break;
            case DRAININDICATOR:
                packet.data = GET_SLICE_32(input_data, 20, 1);
                break;
            case LEAKINDICATOR:
                packet.data = GET_SLICE_32(input_data, 21, 1);
                break;
            case RELAY_0:
                packet.data = GET_SLICE_32(input_data, 22, 1);
                break;
            case RELAY_1:
                packet.data = GET_SLICE_32(input_data, 23, 1);
                break;
            default:;
        }
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
    seq_printf(p, "\nController kernel module\n");
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
    iowrite32(0, base_addr);
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
        driver_proc_entry = proc_create(DRIVER_NAME, 0, NULL, &proc_driver_operations);
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
        printk(KERN_INFO DRIVER_NAME " Mapped at virtual address 0x%08lx\n", (unsigned long) base_addr);

        major = register_chrdev(0, DRIVER_NAME, &proc_driver_operations);

        configport(0x0000FFFF);
        iowrite32(0x000000, base_addr);
    }

    return rc;
}

/* device match table to match with device node in device tree */
static const struct of_device_id zynq_pmod_match[] =
{
    {.compatible = "yarib-controller-1.00.a"},
    { },
};

MODULE_DEVICE_TABLE(of, zynq_pmod_match);

/* platform driver structure for device driver */
static struct platform_driver zynq_pmod_driver =
{
    .driver =
    {
        .name = DRIVER_NAME,
        .owner = THIS_MODULE,
        .of_match_table = zynq_pmod_match
    },
    .probe = driver_probe,
    .remove = driver_remove,
    .shutdown = driver_shutdown
};

/* Register device platform driver */
module_platform_driver(zynq_pmod_driver);

/* Module Informations */
MODULE_AUTHOR("Yarib, Inc.");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION(DRIVER_NAME ": Controller module");
MODULE_ALIAS(DRIVER_NAME);

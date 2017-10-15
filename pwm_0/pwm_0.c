/*
 * AXI Timer v2.0 - PWM Linux driver
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
#define DRIVER_NAME     "pwm_0"

static unsigned long   * base_addr  = NULL;
static struct resource * res        = NULL;
static unsigned long     remap_size = 0;
static int               major;     // major number we get from the kernel
static DeviceID          device_ID = 0;

#define SUCCESS 0

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

// *** Register index
typedef enum
{
    TCSR0 = 0x0,    // Timer 0 Control and Status Register
    TLR0,           // Timer 0 Load Register
    TCR0,           // Timer 0 Counter Register
    RSVD0,          // Reserved
    TCSR1,          // Timer 1 Control and Status Register
    TLR1,           // Timer 1 Load Register
    TCR1,           // Timer 1 Counter Register
    RSVD1           // Reserved
} AXI_Timer_Register_Index;

// *** Control/Status Register 0 flags (TCSR0)
typedef enum
{
    MDT0   = 0x001, // Timer 0 Mode (generate/capture)
    UDT0   = 0x002, // Up/Down Count Timer 0
    GENT0  = 0x004, // Enable External Generate Signal Timer 0
    CAPT0  = 0x008, // Enable External Capture Trigger Timer 0
    ARHT0  = 0x010, // Auto Reload/Hold Timer 0
    LOAD0  = 0x020, // Load Timer 0
    ENIT0  = 0x040, // Enable Interrupt for Timer 0
    ENT0   = 0x080, // Enable Timer 0
    T0INT  = 0x100, // Timer 0 Interrupt
    PWMA0  = 0x200, // Enable Pulse Width Modulation for Timer 0
    ENALL0 = 0x400, // Enable All Timers
    CASC   = 0x800  // Enable cascade mode of timers
} TCR0_Flags;

// *** Control/Status Register 1 flags (TCSR1)
typedef enum
{
    MDT1   = 0x001, // Timer 1 Mode (generate/capture)
    UDT1   = 0x002, // Up/Down Count Timer1
    GENT1  = 0x004, // Enable External Generate Signal Timer1
    CAPT1  = 0x008, // Enable External Capture Trigger Timer1
    ARHT1  = 0x010, // Auto Reload/Hold Timer1
    LOAD1  = 0x020, // Load Timer1
    ENIT1  = 0x040, // Enable Interrupt for Timer1
    ENT1   = 0x080, // Enable Timer1
    T1INT  = 0x100, // Timer1 Interrupt
    PWMB0  = 0x200, // Enable Pulse Width Modulation for Timer1
    ENALL1 = 0x400  // Enable All Timers
} TCR1_Flags;

static void pwm_set_period(u32 period)
{
    outport(TLR0, period);
}

static u32  pwm_get_period(void)
{
    return inport(TLR0);
}

static void pwm_set_high_time(u32 high_time)
{
    outport(TLR1, high_time);
}

static u32  pwm_get_high_time(void)
{
    return inport(TLR1);
}

///////////////////////////////////////////////////////////////////////////////
#define AXI_CLK_FRECUENCY_HZ  50000000 // <--- Define AXI clk frecuency !
#define PWM_FRECUENCY         1000     // <--- Define desired PWM frecuency !
///////////////////////////////////////////////////////////////////////////////

#define CALCULATE_PERIOD_REGISTER(AXI_FREC, PWM_FREC)   (((AXI_FREC) / (PWM_FREC)) - 2)
#define AXI_PERIOD_REGISTER                             CALCULATE_PERIOD_REGISTER(AXI_CLK_FRECUENCY_HZ, PWM_FRECUENCY)

static void pwm_initialize(void)
{
    outport(TCSR0, ENALL0 | PWMA0 | GENT0 | UDT0);
    outport(TCSR1, ENALL1 | PWMB0 | GENT1 | UDT1);

    pwm_set_period(AXI_PERIOD_REGISTER);
    pwm_set_high_time(0);
}


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
                    case PWM_0:
                        pwm_set_high_time(packet.data);
                        break;
                    default:;
                }
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
        packet.device_ID = device_ID;
        packet.data = pwm_get_high_time();
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
    seq_printf(p, " PWM_0, Period = 0x%08X, High time = 0x%08X\n", pwm_get_period(), pwm_get_high_time());
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
    unsigned int size = 64;
    char * buf;
    struct seq_file * m;
    int rc = SUCCESS;

    buf = (char *)kmalloc(size * sizeof(char), GFP_KERNEL);
    if (buf != NULL)
    {
        rc = single_open(file, proc_driver_show, NULL);

        if (!rc)
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
    pwm_set_high_time(0x00000000);
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

        pwm_initialize();
    }

    return rc;
}

/* device match table to match with device node in device tree */
static const struct of_device_id zynq_pwm_0_match[] =
{
    {.compatible = "yarib-pwm_0-1.00.a"},
    { },
};

MODULE_DEVICE_TABLE(of, zynq_pwm_0_match);

/* platform driver structure for device driver */
static struct platform_driver zynq_pwm_0_driver =
{
    .driver =
    {
        .name = DRIVER_NAME,
        .owner = THIS_MODULE,
        .of_match_table = zynq_pwm_0_match
    },
    .probe = driver_probe,
    .remove = driver_remove,
    .shutdown = driver_shutdown
};

/* Register device platform driver */
module_platform_driver(zynq_pwm_0_driver);

/* Module Informations */
MODULE_AUTHOR("Yarib, Inc.");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION(DRIVER_NAME ": PWM module");
MODULE_ALIAS(DRIVER_NAME);

#include <linux/device.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/io.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/ioctl.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/spi/spi.h>
#include <linux/string.h>
#include <linux/miscdevice.h>
#include "nokia5110.h"
#include "testnokia5110.h"

#define DRV_LICENSE "GPL"
#define DRV_AUTHOR "Ton"
#define DRV_DESC "LCD NOKIA5110"
#define IMG_SIZE    3200

static struct nokia5110
{
    uint8_t* recv_buffer;
    struct miscdevice m_dev;
    struct spi_device *nokia5110_spi;
    uint8_t current_X;
    uint8_t current_Y;
};

static int re_pin = 30;
static int dc_pin = 31;
static int be_pin = 48;

static int nokia5110_spi_open(struct inode *inodes, struct file *filp);
static int nokia5110_spi_release(struct inode *inodes, struct file *filp);
static long nokia5110_spi_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
static ssize_t nokia5110_spi_write(struct file *filp, const char __user *buffer, size_t size, loff_t *f_pos);
static int nokia5110_probe(struct spi_device *spi);
static int nokia5110_remove(struct spi_device *spi);

void nokia5110_init(struct spi_device *spi);
void nokia5110_clear_screen(struct nokia5110 *nokia5110);
void nokia5110_send_byte(struct spi_device *spi, bool, uint8_t);
void nokia5110_write_char(struct spi_device *spi, uint8_t);
void nokia5110_write_string(struct spi_device *spi, uint8_t *);
void nokia5110_set_XY_coor(struct nokia5110 *nokia5110, uint8_t, uint8_t);
void nokia5110_next_line(struct nokia5110 *nokia5110);
void nokia5110_print_image(struct nokia5110* nokia5110, uint8_t);

static const struct file_operations fops = {
    .owner = THIS_MODULE,
    .write = nokia5110_spi_write,
    .open = nokia5110_spi_open,
    .release = nokia5110_spi_release,
    .unlocked_ioctl = nokia5110_spi_ioctl,
};

static int nokia5110_spi_open(struct inode *inodes, struct file *filp)
{
    struct nokia5110* nokia5110 = container_of(filp->private_data, struct nokia5110, m_dev);
    if(nokia5110)
        filp->private_data = nokia5110;
    else
        return -ENODEV;
    pr_info("Open device file successfully!");
    return 0;
}

static int nokia5110_spi_release(struct inode *inodes, struct file *filp)
{
    filp->private_data = NULL;
    pr_info("Close device file successfully!");
    return 0;
}

static long nokia5110_spi_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    struct nokia5110* nokia5110 = file->private_data;
    struct position pos;
    if(!nokia5110)
        return -EFAULT;
    switch(cmd) {
        case NEXT_LINE:
            nokia5110_next_line(nokia5110);
            break;
        case GOTO_XY:
            if(!copy_from_user(&pos, (struct position* )arg, sizeof(struct position)))
                nokia5110_set_XY_coor(nokia5110, pos.x, pos.y);
            break;
        case CLEAR:
            nokia5110_clear_screen(nokia5110);
            break;
        case PRINT_IMAGE:
            nokia5110->recv_buffer = kzalloc(IMG_SIZE, GFP_KERNEL);
            if(copy_from_user(nokia5110->recv_buffer, (uint8_t*)arg, IMG_SIZE) != 0) {
                kfree(nokia5110->recv_buffer);
                return -EFAULT;
            }
            nokia5110_print_image(nokia5110, nokia5110->recv_buffer);
            kfree(nokia5110->recv_buffer);
            return IMG_SIZE;
        default:
            printk("No valid cmd!");
            break;
    };
    printk("nokia5110 ioctl function\n");
    return 0;
}

static ssize_t nokia5110_spi_write(struct file *filp, const char __user *buffer, size_t size, loff_t *f_pos)
{
    struct nokia5110 *nokia5110 = filp->private_data;
    if (!nokia5110)
        return -EFAULT;
    nokia5110->recv_buffer = kzalloc(size, GFP_KERNEL);
    if(copy_from_user(nokia5110->recv_buffer, buffer, size) != 0) {
        kfree(nokia5110->recv_buffer);
        return -EFAULT;
    }

    nokia5110_write_string(nokia5110->nokia5110_spi, nokia5110->recv_buffer);
    kfree(nokia5110->recv_buffer);
    return size;
}

void nokia5110_init(struct spi_device *spi)
{
    struct nokia5110 *nokia5110 = spi_get_drvdata(spi);
    // Set GPIOS
    gpio_set_value(re_pin, 0);
    udelay(2);
    gpio_set_value(re_pin, 1);
    udelay(2);
    gpio_set_value(be_pin, 1);

    // init LCD
    nokia5110_send_byte(nokia5110->nokia5110_spi, LCD_CMD, EXTEND_INSTRUCT); // LCD Extended Commands
    nokia5110_send_byte(nokia5110->nokia5110_spi, LCD_CMD, 0xb1);            // Set LCD Cop (Contrast) //0xb1
    nokia5110_send_byte(nokia5110->nokia5110_spi, LCD_CMD, TEMP_CTRL);       // Set Temp Coefficent    //0x04
    nokia5110_send_byte(nokia5110->nokia5110_spi, LCD_CMD, BIAS_SYS);        // LCD bias mode 1:48     //0x13
    nokia5110_send_byte(nokia5110->nokia5110_spi, LCD_CMD, BASIC_INSTRUCT);
    nokia5110_send_byte(nokia5110->nokia5110_spi, LCD_CMD, NORMAL_MODE);

    nokia5110_set_XY_coor(nokia5110, 0, 2);
    nokia5110_write_string(nokia5110->nokia5110_spi, "Hello World!!!!");
    printk(KERN_INFO "Init successfully!");

    gpio_set_value(be_pin, 1);
}

void nokia5110_send_byte(struct spi_device *spi, bool is_cmd, uint8_t data)
{
    if (is_cmd)
        gpio_set_value(dc_pin, 1);
    else
        gpio_set_value(dc_pin, 0);

    spi_write(spi, &data, sizeof(data)); // PASSING SPI DEVICE AS A PARAM
}

void nokia5110_clear_screen(struct nokia5110 *nokia5110)
{
    int i;
    for (i = 0; i < LCD_WIDTH * LCD_HEIGHT / 8; ++i)
    {
        nokia5110_send_byte(nokia5110->nokia5110_spi, LCD_DATA, 0x00);
    }
    nokia5110_set_XY_coor(nokia5110, 0, 0); //
}

void nokia5110_write_char(struct spi_device *spi, uint8_t data)
{
    nokia5110_send_byte(spi, LCD_DATA, 0x00);
    int i;
    for (i = 0; i < 6; ++i)
        nokia5110_send_byte(spi, LCD_DATA, ASCII[data - 0x20][i]);
}

void nokia5110_write_string(struct spi_device *spi, uint8_t *data)
{
    while (*data)
    {
        nokia5110_write_char(spi, *data++);
    }
}
void nokia5110_print_image(struct nokia5110* nokia5110, uint8_t* data) {
    while(*data)
    {
        nokia5110_send_byte(nokia5110->nokia5110_spi, *data++);
    }
}
static int nokia5110_probe(struct spi_device *spi)
{
    int ret;
    struct nokia5110 *nokia5110 = NULL;
    nokia5110 = kzalloc(sizeof(*nokia5110), GFP_KERNEL);
    if (!nokia5110)
    {
        ret = -ENOMEM;
        kfree(nokia5110);
        return ret;
    }
    nokia5110->nokia5110_spi = spi;
    spi_set_drvdata(spi, nokia5110);
    nokia5110->m_dev.minor = MISC_DYNAMIC_MINOR;
    nokia5110->m_dev.name = "nokia5110";
    nokia5110->m_dev.mode = 0666;
    nokia5110->m_dev.fops = &fops;
    if(misc_register(&nokia5110->m_dev) < 0) {
        printk(KERN_ERR "Misc Register failed\n");
        return -1;
    }
    gpio_request(re_pin, "RE");
    gpio_request(dc_pin, "DC");
    gpio_request(be_pin, "BE");
    gpio_direction_output(re_pin, 0);
    gpio_direction_output(dc_pin, 0);
    gpio_direction_output(be_pin, 0);

    nokia5110_init(nokia5110->nokia5110_spi);
    nokia5110_clear_screen(nokia5110);
    nokia5110_set_XY_coor(nokia5110, 0, 2);
    nokia5110_write_string(nokia5110->nokia5110_spi, "Hello World!!!!");
    printk(KERN_INFO "Probed successfully!");
    return 0;
}

static int nokia5110_remove(struct spi_device *spi)
{
    struct nokia5110 *nokia5110 = spi_get_drvdata(spi);
    if (!nokia5110)
    {
        return -1;
    }
    else
    {
        nokia5110_clear_screen(nokia5110);
        misc_deregister(&nokia5110->m_dev);
        kfree(nokia5110);
        printk("\tRemove nokia5110 device successfully!\n");
    };
    return 0;
}

void nokia5110_set_XY_coor(struct nokia5110 *nokia5110, uint8_t X, uint8_t Y)
{
    nokia5110_send_byte(nokia5110->nokia5110_spi, LCD_CMD, Y_BASE_COR | Y);
    nokia5110_send_byte(nokia5110->nokia5110_spi, LCD_CMD, X_BASE_COR | X*6);
    nokia5110->current_X = X;
    nokia5110->current_Y = Y;
}

void nokia5110_next_line(struct nokia5110 *nokia5110) {
    nokia5110->current_Y = (nokia5110->current_Y == MAX_Y) ? 0 : (nokia5110->current_Y + 1);
    nokia5110_set_XY_coor(nokia5110, 0, nokia5110->current_Y);
}
struct of_device_id nokia5110_of_match[] = {
    {.compatible = "nokia5110"},
    {}};

MODULE_DEVICE_TABLE(of, nokia5110_of_match);

static struct spi_driver my_spi_driver = {
    .probe = nokia5110_probe,
    .remove = nokia5110_remove,
    .driver = {
        .name = "nokia5110",
        .owner = THIS_MODULE,
        .of_match_table = nokia5110_of_match,
    },
};

static int __init func_init(void)
{
    return spi_register_driver(&my_spi_driver);
}

static void __exit func_exit(void)
{
    return spi_unregister_driver(&my_spi_driver);
}

module_init(func_init);
module_exit(func_exit);

MODULE_LICENSE(DRV_LICENSE);
MODULE_AUTHOR(DRV_AUTHOR);
MODULE_DESCRIPTION(DRV_DESC);
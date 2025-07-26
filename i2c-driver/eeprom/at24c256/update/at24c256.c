#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/ioctl.h>
#include <linux/delay.h>
#include "at24c256.h"

// #define DRV_NAME "AT24C256"
#define DRV_LICENSE "GPL"
#define DRV_AUTHOR "Ton"
#define DRV_DESC "AT24C256 EEPROM"

static int at24c256_i2c_open(struct inode *inodes, struct file *filp);
static int at24c256_i2c_release(struct inode *inodes, struct file *filp);
static long at24c256_i2c_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
static ssize_t at24c256_i2c_write(struct file *filp, const char __user *buffer, size_t size, loff_t *f_pos);
static ssize_t at24c256_i2c_read(struct file *filp, char __user *buffer, size_t size, loff_t *f_pos);
static int at24c256_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int at24c256_i2c_remove(struct i2c_client *client);

int at24c256_byte_write(struct i2c_client *client, uint8_t *user_data);
int at24c256_page_write(struct i2c_client *client, uint8_t *user_data);
int at24c256_current_read(struct i2c_client *client, uint8_t *user_data);
int at24c256_sequential_read(struct i2c_client *client, uint8_t *user_data);
int at24c256_write_image(struct i2c_client *client, uint8_t *image);
int at24c256_read_image(struct i2c_client *client, uint8_t *image);
int at24c256_random_write(struct i2c_client *client, struct data*);
int at24c256_random_read(struct i2c_client *client, struct data*);
int at24c256_random_write_byte(struct i2c_client *client, struct data*);
int at24c256_random_read_byte(struct i2c_client *client, struct data*);

static const struct file_operations fops = {
    .owner = THIS_MODULE,
    .write = at24c256_i2c_write,
    .read = at24c256_i2c_read,
    .open = at24c256_i2c_open,
    .release = at24c256_i2c_release,
    .unlocked_ioctl = at24c256_i2c_ioctl,
};
int position = 0;
int prev_position = 0;

static struct at24c256
{
    struct miscdevice m_dev;
    struct i2c_client *client;
    uint8_t read_buffer[PAGE_SIZE];
    uint8_t write_buffer[PAGE_SIZE];
    uint8_t write_data;
    uint8_t read_data;
    struct data DATA;
};

static int at24c256_i2c_open(struct inode *inodes, struct file *filp)
{
    struct at24c256 *at24c256 = container_of(filp->private_data, struct at24c256, m_dev);
    if (at24c256)
        filp->private_data = at24c256;
    else
        return -ENODEV;
    pr_info("Open device file successfully!");
    return 0;
    pr_info("Open device file successfully!");
    return 0;
}

static int at24c256_i2c_release(struct inode *inodes, struct file *filp)
{
    filp->private_data = NULL;
    pr_info("Close device file successfully!");
    return 0;
}

static long at24c256_i2c_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    struct at24c256 *at24c256 = file->private_data;
    if (!at24c256)
        return -EFAULT;
    int ret;
    uint8_t read_byte;
    
    switch (cmd)
    {
    case BYTE_WRITE:
        //at24c256->recv_buffer = kzalloc(1, GFP_KERNEL);
        if (copy_from_user(&(at24c256->write_data), (uint8_t *)arg, sizeof(at24c256->write_data)))
        {
            return -EFAULT;
        }

        ret = at24c256_byte_write(at24c256->client, &at24c256->write_data);
        //position++;
        //kfree(at24c256->recv_buffer);
        break;
    case PAGE_WRITE:
        //at24c256->recv_buffer = kzalloc(PAGE_SIZE, GFP_KERNEL);
        if (copy_from_user(&(at24c256->write_buffer), (uint8_t *)arg, PAGE_SIZE))
        {
                return -EFAULT;
        }
        ret = at24c256_page_write(at24c256->client, at24c256->write_buffer);
        //position += PAGE_SIZE;
        //kfree(at24c256->recv_buffer);
        break;
    case CURRENT_READ:
        ret = at24c256_current_read(at24c256->client, &at24c256->read_data);
        if (ret > 0)
        {
            if (copy_to_user((uint8_t *)arg, &at24c256->read_data, sizeof(at24c256->read_data)))
            {
                return -EFAULT;
            }
        }
        //position++;
        break;
    case SEQUENTIAL_READ:
        ret = at24c256_sequential_read(at24c256->client, at24c256->read_buffer);
        if (ret > 0)
        {
            if (copy_to_user((uint8_t *)arg, &at24c256->read_buffer, PAGE_SIZE))
            {
                return -EFAULT;
            }
        }
        //position += PAGE_SIZE;
        break;
    case RANDOM_WRITE:
        if (copy_from_user(&at24c256->DATA, (struct data *)arg, sizeof(at24c256->DATA)))
        {
                return -EFAULT;
        }
        ret = at24c256_random_write(at24c256->client, &at24c256->DATA);
        break;
    case RANDOM_READ:
        if (copy_from_user(&at24c256->DATA, (struct data *)arg, sizeof(at24c256->DATA)))
        {
                return -EFAULT;
        }
        ret = at24c256_random_read(at24c256->client, &at24c256->DATA);
        if (ret > 0)
        {
            if (copy_to_user((struct data *)arg, &at24c256->DATA, sizeof(at24c256->DATA)))
            {
                return -EFAULT;
            }
        }
        break;
    case RANDOM_WRITE_BYTE:
        if (copy_from_user(&at24c256->DATA, (struct data *)arg, sizeof(at24c256->DATA)))
        {
                return -EFAULT;
        }
        ret = at24c256_random_write_byte(at24c256->client, &at24c256->DATA);
        break;
    case RANDOM_READ_BYTE:
        if (copy_from_user(&at24c256->DATA, (struct data *)arg, sizeof(at24c256->DATA)))
        {
                return -EFAULT;
        }
        ret = at24c256_random_read_byte(at24c256->client, &at24c256->DATA);
        if (ret > 0)
        {
            if (copy_to_user((struct data *)arg, &at24c256->DATA, sizeof(at24c256->DATA)))
            {
                return -EFAULT;
            }
        }
        break;
    default:
        return -ENOTTY; // Command not supported
    }
    if (ret < 0)
    {
        return ret;
    }
    printk("nokia5110 ioctl function\n");
    return 0;
}

static ssize_t at24c256_i2c_write(struct file *filp, const char __user *buffer, size_t size, loff_t *f_pos)
{

    struct at24c256 *at24c256 = filp->private_data;
    if (!at24c256)
        return -EFAULT;
    int ret, tmp, i;
    struct i2c_msg msg;
    char temp_data[PAGE_SIZE];
    char buffer_data[PAGE_SIZE + 2];

    if (copy_from_user(temp_data, buffer, PAGE_SIZE) != 0)
    {
        return -EFAULT;
    };

    memset(buffer_data, 0, sizeof(buffer_data));
    buffer_data[1] = (char)0x00;
    buffer_data[0] = (char)0x00;

    for (i = 0; i < PAGE_SIZE; ++i)
    {
        buffer_data[i + 2] = temp_data[i];
    }

    msg.addr = at24c256->client->addr;
    msg.flags = at24c256->client->flags & 0;
    msg.buf = &buffer_data[0];
    msg.len = PAGE_SIZE + 2;

    ret = i2c_transfer(at24c256->client->adapter, &msg, 1);
    tmp = (ret == 1) ? msg.len : ret;

    printk("i2c code: %d return code: %d addr: 0x%02x%02x ", ret, tmp, buffer_data[0], buffer_data[1]);

    for (i = 0; i < PAGE_SIZE; ++i)
    {
        printk("Write Data: 0x%02x", buffer_data[i + 2]);
    }

    return size;
}

static ssize_t at24c256_i2c_read(struct file *filp, char __user *buffer, size_t size, loff_t *f_pos)
{
    struct at24c256 *at24c256 = filp->private_data;
    if (!at24c256)
        return -EFAULT;
    int ret, tmp, i;
    char word_addr[2];
    char buffer_data[PAGE_SIZE];
    struct i2c_msg msg[2];

    memset(word_addr, 0, sizeof(word_addr));
    memset(buffer_data, 0, sizeof(buffer_data));

    word_addr[1] = (char)0x00;
    word_addr[0] = (char)0x00;

    msg[0].addr = at24c256->client->addr;
    msg[0].flags = at24c256->client->flags & 0;
    msg[0].buf = &word_addr[0];
    msg[0].len = 2;

    msg[1].addr = at24c256->client->addr;
    msg[1].flags = I2C_M_RD;
    msg[1].buf = &buffer_data[0];
    msg[1].len = PAGE_SIZE;

    i2c_transfer(at24c256->client->adapter, msg, 2);
    // i2c_transfer(at24c256->client->adapter, &msg[1], 1);
    tmp = (ret == 1) ? msg[1].len : ret;

    printk("i2c code: %d return code: %d addr: 0x%02x%02x ", ret, tmp, word_addr[0], word_addr[1]);

    for (i = 0; i < PAGE_SIZE; ++i)
    {
        printk("Read data: 0x%02x", buffer_data[i]);
    }
    copy_to_user(buffer, buffer_data, PAGE_SIZE);
    return size;
}

int at24c256_current_read(struct i2c_client *client, uint8_t *user_data)
{
    int ret, tmp, i;
    struct i2c_msg msg;

    msg.addr = client->addr;
    msg.flags = I2C_M_RD;
    msg.buf = user_data;
    msg.len = 1;
    ret = i2c_transfer(client->adapter, &msg, 1);
    if (ret < 0)
    {
        dev_err(&client->dev, "Failed to read from EEPROM: %d\n", ret);
        return ret;
    }
    tmp = (ret == 1) ? msg.len : ret;
    // printk("i2c code: %d return code: %d addr: 0x%02x%02x ", ret, tmp, buffer_data[0], buffer_data[1]);
        printk("Read data: 0x%02x", *user_data);
        //user_data = buffer_data;
    //prev_position++;
    return ret;
}

int at24c256_sequential_read(struct i2c_client *client, uint8_t *user_data)
{
    int ret, tmp, i;
    uint8_t word_addr[2];
    struct i2c_msg msg[2];

    word_addr[0] = (u8)(prev_position >> 8) & 0xFF;
    word_addr[1] = (u8)(prev_position & 0xFF);
    
    msg[0].addr = client->addr;
    msg[0].flags = client->flags & 0;
    msg[0].buf = word_addr;
    msg[0].len = 2;

    msg[1].addr = client->addr;
    msg[1].flags = I2C_M_RD;
    msg[1].buf = user_data;
    msg[1].len = PAGE_SIZE;

    ret = i2c_transfer(client->adapter, msg, 2);
    if (ret < 0)
    {
        dev_err(&client->dev, "Failed to read from EEPROM: %d\n", ret);
        return ret;
    }
    tmp = (ret == 1) ? msg[1].len : ret;

    printk("i2c code: %d return code: %d addr: 0x%02x%02x ", ret, tmp, word_addr[0], word_addr[1]);

    for (i = 0; i < PAGE_SIZE; ++i)
    {
        printk("Read data: 0x%02x", *(user_data+i));
        //user_data[i] = buffer_data[i];
    }
    prev_position += PAGE_SIZE;
    return ret;
}

int at24c256_byte_write(struct i2c_client *client, uint8_t *user_data)
{
    int ret, tmp, i;
    struct i2c_msg msg;
    uint8_t buffer_data[3];

    memset(buffer_data, 0, sizeof(buffer_data));

    buffer_data[0] = (u8)(position >> 8) & 0xFF;
    buffer_data[1] = (u8)(position & 0xFF);
    buffer_data[2] = *user_data;

    msg.addr = client->addr;
    msg.flags = client->flags & 0;
    msg.buf = buffer_data;
    msg.len = 3;

    ret = i2c_transfer(client->adapter, &msg, 1);
    if (ret < 0)
    {
        dev_err(&client->dev, "Failed to write from EEPROM: %d\n", ret);
        return ret;
    }
    tmp = (ret == 1) ? msg.len : ret;

    printk("i2c code: %d return code: %d addr: 0x%02x%02x ", ret, tmp, buffer_data[0], buffer_data[1]);
    printk("Write Data: 0x%02x", buffer_data[2]);
    prev_position = position;
    position++;
    return ret;
}

int at24c256_page_write(struct i2c_client *client, uint8_t *user_data)
{
    int ret, tmp, i;
    struct i2c_msg msg;
    // struct i2c_msg* MSG = &msg;
    uint8_t buffer_data[PAGE_SIZE + 2];

    memset(buffer_data, 0, sizeof(buffer_data));

    buffer_data[0] = (u8)(position >> 8) & 0xFF;
    buffer_data[1] = (u8)(position & 0xFF);

    for (i = 0; i < PAGE_SIZE; ++i)
    {
        buffer_data[i + 2] = *(user_data+i);
    }

    msg.addr = client->addr;
    msg.flags = client->flags & 0;
    msg.buf = buffer_data;
    msg.len = PAGE_SIZE + 2;

    ret = i2c_transfer(client->adapter, &msg, 1);
    if (ret < 0)
    {
        dev_err(&client->dev, "Failed to write from EEPROM: %d\n", ret);
        return ret;
    }
    tmp = (ret == 1) ? msg.len : ret;

    printk("i2c code: %d return code: %d addr: 0x%02x%02x ", ret, tmp, buffer_data[0], buffer_data[1]);

    for (i = 0; i < PAGE_SIZE; ++i)
    {
        printk("Write Data: 0x%02x", buffer_data[i + 2]);
    }
    prev_position = position;
    position += PAGE_SIZE;
    return ret;
}

int at24c256_random_write(struct i2c_client *client, struct data *user_data)
{
    int ret, tmp, i;
    struct i2c_msg msg;
    // struct i2c_msg* MSG = &msg;
    uint8_t buffer_data[PAGE_SIZE + 2];

    memset(buffer_data, 0, sizeof(buffer_data));

    buffer_data[0] = (u8)(user_data->address >> 8) & 0xFF;
    buffer_data[1] = (u8)(user_data->address & 0xFF);

    for (i = 0; i < PAGE_SIZE; ++i)
    {
        buffer_data[i + 2] = user_data->chunk[i];
    }

    msg.addr = client->addr;
    msg.flags = client->flags & 0;
    msg.buf = buffer_data;
    msg.len = PAGE_SIZE + 2;

    ret = i2c_transfer(client->adapter, &msg, 1);
    if (ret < 0)
    {
        dev_err(&client->dev, "Failed to write from EEPROM: %d\n", ret);
        return ret;
    }
    tmp = (ret == 1) ? msg.len : ret;

    printk("i2c code: %d return code: %d addr: 0x%02x%02x ", ret, tmp, buffer_data[0], buffer_data[1]);

    for (i = 0; i < PAGE_SIZE; ++i)
    {
        printk("Write Data: 0x%02x", buffer_data[i + 2]);
    }
    // prev_position = position;
    // position += PAGE_SIZE;
    return ret;
}

int at24c256_random_read(struct i2c_client *client, struct data *user_data)
{
    int ret, tmp, i;
    uint8_t word_addr[2];
    struct i2c_msg msg[2];

    word_addr[0] = (u8)(user_data->address >> 8) & 0xFF;
    word_addr[1] = (u8)(user_data->address & 0xFF);
    
    msg[0].addr = client->addr;
    msg[0].flags = client->flags & 0;
    msg[0].buf = word_addr;
    msg[0].len = 2;

    msg[1].addr = client->addr;
    msg[1].flags = I2C_M_RD;
    msg[1].buf = user_data->chunk;
    msg[1].len = PAGE_SIZE;

    ret = i2c_transfer(client->adapter, msg, 2);
    if (ret < 0)
    {
        dev_err(&client->dev, "Failed to read from EEPROM: %d\n", ret);
        return ret;
    }
    tmp = (ret == 1) ? msg[1].len : ret;

    printk("i2c code: %d return code: %d addr: 0x%02x%02x ", ret, tmp, word_addr[0], word_addr[1]);

    for (i = 0; i < PAGE_SIZE; ++i)
    {
        printk("Read data: 0x%02x", user_data->chunk[i]);
        //user_data[i] = buffer_data[i];
    }
    //prev_position += PAGE_SIZE;
    return ret;
}

int at24c256_random_write_byte(struct i2c_client *client, struct data *user_data)
{
    int ret, tmp, i;
    struct i2c_msg msg;
    // struct i2c_msg* MSG = &msg;
    uint8_t buffer_data[PAGE_SIZE + 2];

    memset(buffer_data, 0, sizeof(buffer_data));

    buffer_data[0] = (u8)(user_data->address >> 8) & 0xFF;
    buffer_data[1] = (u8)(user_data->address & 0xFF);

    for (i = 0; i < PAGE_SIZE; ++i)
    {
        buffer_data[i + 2] = user_data->chunk[i];
    }

    msg.addr = client->addr;
    msg.flags = client->flags & 0;
    msg.buf = buffer_data;
    msg.len = 3;

    ret = i2c_transfer(client->adapter, &msg, 1);
    if (ret < 0)
    {
        dev_err(&client->dev, "Failed to write from EEPROM: %d\n", ret);
        return ret;
    }
    tmp = (ret == 1) ? msg.len : ret;

    printk("i2c code: %d return code: %d addr: 0x%02x%02x ", ret, tmp, buffer_data[0], buffer_data[1]);
    printk("Write Data: 0x%02x", buffer_data[2]);
    // prev_position = position;
    // position += PAGE_SIZE;
    return ret;
}

int at24c256_random_read_byte(struct i2c_client *client, struct data *user_data)
{
    int ret, tmp, i;
    uint8_t word_addr[2];
    struct i2c_msg msg[2];

    word_addr[0] = (u8)(user_data->address >> 8) & 0xFF;
    word_addr[1] = (u8)(user_data->address & 0xFF);
    
    msg[0].addr = client->addr;
    msg[0].flags = client->flags & 0;
    msg[0].buf = word_addr;
    msg[0].len = 2;

    msg[1].addr = client->addr;
    msg[1].flags = I2C_M_RD;
    msg[1].buf = user_data->chunk;
    msg[1].len = 1;

    ret = i2c_transfer(client->adapter, msg, 2);
    if (ret < 0)
    {
        dev_err(&client->dev, "Failed to read from EEPROM: %d\n", ret);
        return ret;
    }
    tmp = (ret == 1) ? msg[1].len : ret;

    printk("i2c code: %d return code: %d addr: 0x%02x%02x ", ret, tmp, word_addr[0], word_addr[1]);
    printk("Read data: 0x%02x", user_data->chunk[0]);
        //user_data[i] = buffer_data[i];
    //prev_position += PAGE_SIZE;
    return ret;
}

static int at24c256_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{

    int ret;
    struct at24c256 *at24c256 = NULL;
    at24c256 = kzalloc(sizeof(*at24c256), GFP_KERNEL);
    if (!at24c256)
    {
        ret = -ENOMEM;
        kfree(at24c256);
        return ret;
    };

    at24c256->client = client;
    i2c_set_clientdata(client, at24c256);
    
    at24c256->m_dev.minor = MISC_DYNAMIC_MINOR;
    at24c256->m_dev.name = "at24c256";
    at24c256->m_dev.mode = 0666;
    at24c256->m_dev.fops = &fops;
    printk(KERN_INFO "Probed successfully!");
    return misc_register(&at24c256->m_dev);
}

static int at24c256_i2c_remove(struct i2c_client *client)
{
    printk("\tRemove at24c256 device...\n");
    struct at24c256 *at24c256 = i2c_get_clientdata(client);
    if (!at24c256)
    {
        return -1;
    }
    else
    {
        misc_deregister(&at24c256->m_dev);
        kfree(at24c256);
        printk("\tRemove at24c256 device success\n");
    }
    return 0;
}
static const struct of_device_id at24c256_i2c_of_match[] = {
    {.compatible = "at24c256"},
    {}};

MODULE_DEVICE_TABLE(of, at24c256_i2c_of_match);

static struct i2c_driver at24c256_driver = {
    .driver = {
        .name = "at24c256",
        .owner = THIS_MODULE,
        .of_match_table = at24c256_i2c_of_match,
    },
    .probe = at24c256_i2c_probe,
    .remove = at24c256_i2c_remove,
};

module_i2c_driver(at24c256_driver);

MODULE_LICENSE(DRV_LICENSE);
MODULE_AUTHOR(DRV_AUTHOR);
MODULE_DESCRIPTION(DRV_DESC);
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/ioctl.h>
#include <linux/rslib.h>
#include <asm/uaccess.h>
#include <linux/slab.h>
#include <linux/rslib.h>
#include <linux/mutex.h>
#include "rs_driver_ioctl.h"

MODULE_LICENSE("Dual BSD/GPL");

static int rs_major = 0;
static int rs_minor = 0;
static int rs_no_dev = 2;

static dev_t dev;
static struct cdev rs_cdev_encode; 
static struct cdev rs_cdev_decode;
static struct class *rs_class;

static struct rs_control *rs_decoder;
static struct rs_params current_params;
static struct mutex rs_lock;

static long rs_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    struct rs_params user_params;
    long ret = 0;

    if (_IOC_TYPE(cmd) != RS_IOCTL_MAGIC) {
        return -ENOTTY;
    }

    mutex_lock(&rs_lock);

    switch (cmd) {
        case RS_SET_PARAMS:
            if (copy_from_user(&user_params, (void __user *)arg, sizeof(user_params))) {
                return -EFAULT;
            }

            if (rs_decoder) {
                free_rs(rs_decoder);
            }

            rs_decoder = init_rs(user_params.symsize, user_params.gfpoly,
                                 user_params.fcr, user_params.prim, user_params.nroots);

            if (IS_ERR(rs_decoder)) {
                printk(KERN_ERR "RS: Failed to initialize decoder\n");
                rs_decoder = NULL;
                ret = PTR_ERR(rs_decoder);
            } else {
                printk(KERN_INFO "RS: Decoder initialized with new parameters\n");
                current_params = user_params;
            }
            break;
        default:
            ret = -ENOTTY; 
    }
    mutex_unlock(&rs_lock);
    return ret;
}

static ssize_t rs_encode_read(struct file *file, char __user *buf, size_t len, loff_t *offset);
static ssize_t rs_encode_write(struct file *file, const char __user *buf, size_t len, loff_t *offset);
static ssize_t rs_decode_read(struct file *file, char __user *buf, size_t len, loff_t *offset);
static ssize_t rs_decode_write(struct file *file, const char __user *buf, size_t len, loff_t *offset);

static uint8_t *driver_buffer;
static size_t buffer_size = 0;
static size_t data_len = 0;

static ssize_t rs_encode_write(struct file *file, const char __user *buf, size_t len, loff_t *offset)
{
    uint16_t *parity_out;

    if (!rs_decoder) {
        return -EAGAIN; 
    }

    size_t parity_len = current_params.nroots;
    buffer_size = len + parity_len;
    
    if (driver_buffer)
        kfree(driver_buffer);
    driver_buffer = kmalloc(buffer_size, GFP_KERNEL);
    if (!driver_buffer) {
        return -ENOMEM;
    }

    if (copy_from_user(driver_buffer, buf, len)) {
        kfree(driver_buffer);
        driver_buffer = NULL;
        return -EFAULT;
    }

    parity_out = (uint16_t *)(driver_buffer + len);
    
    memset(parity_out, 0, parity_len * sizeof(uint16_t));
    encode_rs8(rs_decoder, driver_buffer, len, parity_out, 0);

    data_len = len;
    printk(KERN_INFO "RS: Encoded %zu bytes with %zu parity symbols\n", data_len, parity_len);

    return len; 
}

static ssize_t rs_decode_write(struct file *file, const char __user *buf, size_t len, loff_t *offset)
{
    int corrected_errors;
    uint16_t *parity_in;

    if (!rs_decoder || len != buffer_size) {
        return -EINVAL; 
    }

    if (copy_from_user(driver_buffer, buf, len)) {
        return -EFAULT;
    }

    parity_in = (uint16_t *)(driver_buffer + data_len);
    
    corrected_errors = decode_rs8(rs_decoder, driver_buffer, parity_in, data_len, NULL, 0, NULL, 0, NULL);

    if (corrected_errors < 0) {
        printk(KERN_ERR "RS: Decoding failed with error %d\n", corrected_errors);
        return -EIO;
    } else {
        printk(KERN_INFO "RS: Decoded data with %d corrections\n", corrected_errors);
    }

    return len; 
}

static ssize_t rs_decode_read(struct file *file, char __user *buf, size_t len, loff_t *offset)
{
    size_t to_copy;

    if (!driver_buffer || *offset >= data_len) {
        return 0;
    }

    to_copy = min(len, data_len - (size_t)*offset);

    if (copy_to_user(buf, driver_buffer + *offset, to_copy)) {
        return -EFAULT;
    }

    *offset += to_copy;
    return to_copy;
}

static ssize_t rs_encode_read(struct file *file, char __user *buf, size_t len, loff_t *offset)
{
    size_t to_copy;

    if (!driver_buffer || *offset >= buffer_size) {
        return 0; 
    }

    to_copy = min(len, buffer_size - (size_t)*offset);

    if (copy_to_user(buf, driver_buffer + *offset, to_copy)) {
        return -EFAULT;
    }

    *offset += to_copy;
    return to_copy;
}

static struct file_operations rs_encode_fops = {
    .owner = THIS_MODULE,
    .read = rs_encode_read,
    .write = rs_encode_write,
	.unlocked_ioctl = rs_ioctl,
};

static struct file_operations rs_decode_fops = {
    .owner = THIS_MODULE,
    .read = rs_decode_read,
    .write = rs_decode_write,
	.unlocked_ioctl = rs_ioctl,
};

static int __init startup_func (void){
	int result;

    mutex_init(&rs_lock);

	if (rs_major){
		dev = MKDEV (rs_major, rs_minor);
		result = register_chrdev_region(dev, rs_no_dev, "rs");
	}
	else {
		result = alloc_chrdev_region(&dev, rs_minor, rs_no_dev, "rs");
		rs_major = MAJOR(dev);
	}

	if(result < 0){
		printk(KERN_WARNING "rs: can't get major %d\n", rs_major);
		return result;
	}

	printk(KERN_INFO "rs: registered with major %d\n", rs_major);

    cdev_init(&rs_cdev_encode, &rs_encode_fops);
    rs_cdev_encode.owner = THIS_MODULE;
    cdev_add(&rs_cdev_encode, MKDEV(rs_major, 0), 1);

    cdev_init(&rs_cdev_decode, &rs_decode_fops);
    rs_cdev_decode.owner = THIS_MODULE;
    cdev_add(&rs_cdev_decode, MKDEV(rs_major, 1), 1);

	rs_class = class_create("rs");
	device_create(rs_class, NULL, MKDEV(rs_major, 0), NULL, "encoded");
	device_create(rs_class, NULL, MKDEV(rs_major, 1), NULL, "decoded");
	return 0;

}

static void __exit cleanup_func (void){
	dev_t dev = MKDEV(rs_major, rs_minor);
    printk(KERN_INFO "rs: unregister major %d\n", rs_major);
    device_destroy(rs_class, MKDEV(rs_major, 0));
    device_destroy(rs_class, MKDEV(rs_major, 1));
    class_destroy(rs_class);
    cdev_del(&rs_cdev_encode);
    cdev_del(&rs_cdev_decode);
    unregister_chrdev_region(dev, rs_no_dev);
    if (rs_decoder) free_rs(rs_decoder);
    if (driver_buffer) kfree(driver_buffer);
}

module_init(startup_func);
module_exit(cleanup_func);


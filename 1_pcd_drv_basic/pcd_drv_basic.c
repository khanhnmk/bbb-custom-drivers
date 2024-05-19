// Write a devcie driver for a virtual char device
// The device what we drive must support reading/writing/seeking/ioctl to this device

#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/slab.h>		// kmalloc()
#include <linux/sched.h>
#include <asm/current.h>
#include <linux/uaccess.h>	// copy_to_user(), copy_from_user()

#undef pr_fmt
#define pr_fmt(fmt) "%s : " fmt,__func__

#define MEM_SIZE 1024

char buffer[MEM_SIZE];

static const unsigned int MINOR_BASE = 0;
static const unsigned int MINOR_NUM  = 1;

// Declare necessary structure for driver
static dev_t pcd_number;                   // store major and minor number
static struct cdev pcd_dev;           // register deivce with kernel
static struct class *pcd_class;        // register deivce class to sysfs (under /sys/class)
static struct device *pcd_device;      // register deivce to sysfs and create device file under /dev

// Declare file operations
int pcd_open(struct inode *inode, struct file *filp);
int pcd_release(struct inode *inode, struct file *flip);
ssize_t pcd_read(struct file *filp, char __user *buff, size_t count, loff_t *f_pos);
ssize_t pcd_write(struct file *filp, const char __user *buff, size_t count, loff_t *f_pos);

// 
static struct file_operations pcd_fops = {
    .owner = THIS_MODULE,
    .read = pcd_read,
    .write = pcd_write,
    .open = pcd_open,
    .release = pcd_release
};

int pcd_open(struct inode *inode, struct file *filp) {
    pr_info(" Open \n");
    return 0;
}
int pcd_release(struct inode *inode, struct file *flip) {
    pr_info(" Release\n");
    return 0;
}
ssize_t pcd_read(struct file *filp, char __user *buff, size_t count, loff_t *f_pos) {
    pr_info(" Read\n");
    if(count > MEM_SIZE)
        count = MEM_SIZE;
    if(copy_to_user(buff,buffer,count)){
		return -EFAULT;
	}
    return count;
}
ssize_t pcd_write(struct file *filp, const char __user *buff, size_t count, loff_t *f_pos) {
    pr_info(" Write\n");

    if(count > MEM_SIZE)
        count = MEM_SIZE;

    if(copy_from_user(buffer,buff,count)){
		return -EFAULT;
	}
    return count;
}

// module init
static int __init pcd_module_init(void) {

    int ret = 0;
    pr_info("Module init !!!\n");

    // allocate the major and minor number of virtual device
    ret = alloc_chrdev_region(&pcd_number, MINOR_BASE, MINOR_NUM, "pseudo_char_device");
    if(ret < 0) {
        pr_err("Alloc chrdev failed\n");
        goto err_return;
    }

    pr_info("Device number <major>:<minor> = %d:%d \n", MAJOR(pcd_number), MINOR(pcd_number));

    // init cdev
    cdev_init(&pcd_dev, &pcd_fops);

    // add cdev
    ret = cdev_add(&pcd_dev, pcd_number, MINOR_NUM);
    if(ret < 0) {
        pr_err("Cdev add failed\n");
        goto unreg_pcd_number;
    }

    // create class
    pcd_class = class_create(THIS_MODULE, "pcd_class");
    if(IS_ERR(pcd_class)) {
        pr_err("Class creation failed\n");
        ret = PTR_ERR(pcd_class);
        goto pcdev_del;
    }

    // create device file under /dev
    pcd_device = device_create(pcd_class, NULL, pcd_number, NULL, "pcdev_1");
    if(IS_ERR(pcd_device)) {
        pr_err("Device create failed\n");
		ret = PTR_ERR(pcd_device);
        goto class_del;
    }

    pr_info("Module init was successful\n");
    return 0;

class_del:
    class_destroy(pcd_class);

pcdev_del:
    cdev_del(&pcd_dev);

unreg_pcd_number:
    unregister_chrdev_region(pcd_number, MINOR_NUM);

err_return:
    pr_info("Module insertion failed\n");
    return ret;
} 

// module exit
static void __exit pcd_module_exit(void)
{
	device_destroy(pcd_class, pcd_number);
	class_destroy(pcd_class);
	cdev_del(&pcd_dev);
	unregister_chrdev_region(pcd_number, MINOR_NUM);
    pr_info("Module exit !!!\n");
}

module_init(pcd_module_init);
module_exit(pcd_module_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("minkha");
MODULE_DESCRIPTION("A basic pseudo character driver");
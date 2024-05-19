#include<linux/module.h>
#include<linux/fs.h>
#include<linux/cdev.h>
#include<linux/device.h>
#include<linux/kdev_t.h>
#include<linux/uaccess.h>


#undef pr_fmt
#define pr_fmt(fmt) "%s : " fmt,__func__

#define NO_OF_DEVICES 4

#define MEM_SIZE_PCDEV1 512
#define MEM_SIZE_PCDEV2 1024
#define MEM_SIZE_PCDEV3 512
#define MEM_SIZE_PCDEV4 1024

char device_data_pcdev1[MEM_SIZE_PCDEV1];
char device_data_pcdev2[MEM_SIZE_PCDEV2];
char device_data_pcdev3[MEM_SIZE_PCDEV3];
char device_data_pcdev4[MEM_SIZE_PCDEV4];

struct pcdev_private_data {
    char *buffer;
    unsigned int size;
    const char *serial_number;
    struct cdev cdev;
};

struct pcdrv_private_data {
    unsigned int total_device;
    dev_t device_number;
    struct class *class_pcd;
    struct device *device_pcd;
    struct pcdev_private_data  pcdev_data[NO_OF_DEVICES];
};

struct pcdrv_private_data pcdrv_data = {
    .total_device = NO_OF_DEVICES,
    .pcdev_data = {
         
        [0] = {
            .buffer = device_data_pcdev1,
            .size = MEM_SIZE_PCDEV1,
            .serial_number = "PCDEV1_XYZ123"
        },

        [1] = {
            .buffer = device_data_pcdev2,
            .size = MEM_SIZE_PCDEV2,
            .serial_number = "PCDEV2_XYZ123"
        },

        [2] = {
            .buffer = device_data_pcdev3,
            .size = MEM_SIZE_PCDEV3,
            .serial_number = "PCDEV3_XYZ123"
        },

        [3] = {
            .buffer = device_data_pcdev4,
            .size = MEM_SIZE_PCDEV4,
            .serial_number = "PCDEV4_XYZ123"
        },
    }
};



int pcd_open(struct inode *inode, struct file *filp);
int pcd_release(struct inode *inode, struct file *flip);
ssize_t pcd_read(struct file *filp, char __user *buff, size_t count, loff_t *f_pos);
ssize_t pcd_write(struct file *filp, const char __user *buff, size_t count, loff_t *f_pos);

/* file operations of the driver */
struct file_operations pcd_fops =
{
    .open = pcd_open,
    .release = pcd_release,
    .read = pcd_read,
    .write = pcd_write,
    .owner = THIS_MODULE
};

int pcd_open(struct inode *inode, struct file *filp) {
    
    pr_info("%s called\n", __func__);

    int ret, minor_n;

    struct pcdev_private_data *pcdevX_data;

    // find out which deivce file was attemped by user space
    minor_n = MINOR(inode->i_rdev);
    pr_info("minor access = %d\n",minor_n);

    // get device private data structure
    pcdevX_data = container_of(inode->i_cdev, struct pcdev_private_data, cdev);

    filp->private_data = pcdevX_data;

    return ret;
}

int pcd_release(struct inode *inode, struct file *flip) {
    pr_info("%s called\n", __func__);
    pr_info("release was successful\n");
	return 0;
}

ssize_t pcd_read(struct file *filp, char __user *buff, size_t count, loff_t *f_pos) {
    pr_info("%s called\n", __func__);
    struct pcdev_private_data *pcdevX_data = (struct pcdev_private_data*) filp->private_data;

    int max_size = pcdevX_data->size;

    pr_info("Read requested for %zu bytes \n",count);
	pr_info("Current file position = %lld\n",*f_pos);

	
	/* Adjust the 'count' */
	if((*f_pos + count) > max_size)
		count = max_size - *f_pos;

	/*copy to user */
	if(copy_to_user(buff,pcdevX_data->buffer+(*f_pos),count)){
		return -EFAULT;
	}

	/*update the current file postion */
	*f_pos += count;

	pr_info("Number of bytes successfully read = %zu\n",count);
	pr_info("Updated file position = %lld\n",*f_pos);

	/*Return number of bytes which have been successfully read */
	return count;
}

ssize_t pcd_write(struct file *filp, const char __user *buff, size_t count, loff_t *f_pos) {
    pr_info("%s called\n", __func__);
    struct pcdev_private_data *pcdevX_data = (struct pcdev_private_data*)filp->private_data;

	int max_size = pcdevX_data->size;
	
	pr_info("Write requested for %zu bytes\n",count);
	pr_info("Current file position = %lld\n",*f_pos);

	
	/* Adjust the 'count' */
	if((*f_pos + count) > max_size)
		count = max_size - *f_pos;

	if(!count){
		pr_err("No space left on the device \n");
		return -ENOMEM;
	}

	/*copy from user */
	if(copy_from_user(pcdevX_data->buffer+(*f_pos),buff,count)){
		return -EFAULT;
	}

	/*update the current file postion */
	*f_pos += count;

	pr_info("Number of bytes successfully written = %zu\n",count);
	pr_info("Updated file position = %lld\n",*f_pos);

	/*Return number of bytes which have been successfully written */
	return count;
}

static int __init pcd_module_init(void) {

    pr_info("Module init !!!\n");

    int ret = 0, i = 0;

    ret = alloc_chrdev_region(&pcdrv_data.device_number, 0, NO_OF_DEVICES, "pcdevs");
    if(ret < 0) {
        pr_err("Alloc chrdev failed\n");
        goto err_return;
    }

    pcdrv_data.class_pcd = class_create(THIS_MODULE, "pcd_class");
    if(IS_ERR(pcdrv_data.class_pcd )) {
        pr_err("Class creation failded\n");
        ret = PTR_ERR(pcdrv_data.class_pcd);
        goto unreg_cdev;
    }

    for(i = 0; i < NO_OF_DEVICES; i++) {
        pr_info("Device number <major>:<minor> = %d:%d\n",MAJOR(pcdrv_data.device_number+i),MINOR(pcdrv_data.device_number+i));

        cdev_init(&pcdrv_data.pcdev_data[i].cdev, &pcd_fops);

        ret = cdev_add(&pcdrv_data.pcdev_data[i].cdev, pcdrv_data.device_number+i, 1);
        if(ret < 0) {
            pr_err("Cdev[%d] add failed\n", i);
            goto cdev_del;
        }

        pcdrv_data.device_pcd = device_create(pcdrv_data.class_pcd, NULL, pcdrv_data.device_number+i, NULL, "pcdev_%d", i);
        if(IS_ERR(pcdrv_data.device_pcd)){
            pr_err("Device create failed\n");
            ret = PTR_ERR(pcdrv_data.device_pcd);
            goto class_del;
        }
    }

    pr_info("Module init was successful\n");
    return 0;

cdev_del:
class_del:
    for(; i >= 0; i--) {
        device_destroy(pcdrv_data.class_pcd,pcdrv_data.device_number+i);
        cdev_del(&pcdrv_data.pcdev_data[i].cdev);
    }
    class_destroy(pcdrv_data.class_pcd);

unreg_cdev:
    unregister_chrdev_region(pcdrv_data.device_number,NO_OF_DEVICES);

err_return:
    pr_info("Module insertion failed\n");
    return ret; 
}

static void __exit pcd_module_exit(void)
{
    int i;

    for(i=0;i<NO_OF_DEVICES;i++){
        device_destroy(pcdrv_data.class_pcd,pcdrv_data.device_number+i); /* remove device information from sysfs */
        cdev_del(&pcdrv_data.pcdev_data[i].cdev); /*  Unregister a device (cdev structure) with VFS */
    }
    class_destroy(pcdrv_data.class_pcd); /*delete device class under /sys/class/ */
 
    unregister_chrdev_region(pcdrv_data.device_number,NO_OF_DEVICES); /*Dynamically deallocate  device numbers */
    
    pr_info("Module exit !!!\n");
}

module_init(pcd_module_init);
module_exit(pcd_module_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("minkha");
MODULE_DESCRIPTION("A pseudo character driver for multiple device");
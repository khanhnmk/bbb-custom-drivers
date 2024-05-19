#include<linux/module.h>
#include<linux/fs.h>
#include<linux/cdev.h>
#include<linux/device.h>
#include<linux/kdev_t.h>
#include<linux/uaccess.h>
#include <linux/platform_device.h>
#include<linux/slab.h>
#include<linux/mod_devicetable.h>
#include<linux/property.h>


#undef pr_fmt
#define pr_fmt(fmt) "%s : " fmt,__func__

#define MAX_DEVICES 10

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
    unsigned int total_devices;
    dev_t device_number;
    struct class *class_pcd;
    struct device *device_pcd;
};

struct pcdev_private_data pcdev_data[MAX_DEVICES] = {
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
};

struct pcdrv_private_data pcdrv_data;

int pcd_open(struct inode *inode, struct file *filp);
int pcd_release(struct inode *inode, struct file *flip);
ssize_t pcd_read(struct file *filp, char __user *buff, size_t count, loff_t *f_pos);
ssize_t pcd_write(struct file *filp, const char __user *buff, size_t count, loff_t *f_pos);

struct file_operations pcd_fops =
{
    .open = pcd_open,
    .release = pcd_release,
    .read = pcd_read,
    .write = pcd_write,
    .owner = THIS_MODULE
};

int pcd_open(struct inode *inode, struct file *filp) {
    pr_info(" called !!!\n");
   
    int ret, minor_n;

    struct pcdev_private_data *pcdevX_data;
    minor_n = MINOR(inode->i_rdev);
    pr_info("minor access = %d\n",minor_n);

     // get device private data structure
    pcdevX_data = container_of(inode->i_cdev, struct pcdev_private_data, cdev);

    filp->private_data = pcdevX_data;
    return 0;
}

int pcd_release(struct inode *inode, struct file *filp) {
    pr_info(" called !!!\n");
    return 0;
}

ssize_t pcd_read(struct file *filp, char __user *buff, size_t count, loff_t *f_pos) {
    pr_info(" called !!!\n");
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
    pr_info(" called !!!\n");
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

struct platform_device_id pcdevs_ids[] = 
{
	[0] = {.name = "pcdev-A1x", .driver_data = (kernel_ulong_t)&pcdev_data[0]},
	[1] = {.name = "pcdev-B1x", .driver_data = (kernel_ulong_t)&pcdev_data[1]},
	[2] = {.name = "pcdev-C1x", .driver_data = (kernel_ulong_t)&pcdev_data[2]},
	[3] = {.name = "pcdev-D1x", .driver_data = (kernel_ulong_t)&pcdev_data[3]},
	{ } /*Null termination */
};

int pcd_platform_driver_probe(struct platform_device *pdev) {
    pr_info(" probe !!!\n");

    int ret;
    struct pcdev_private_data *pcdevX_data;
    pcdevX_data = pdev->id_entry->driver_data;
    // pr_info("pcdevX_data - serial: %s\n", pcdevX_data->serial_number);

    int id = pdev->id;

    cdev_init(&pcdevX_data->cdev,&pcd_fops);
    ret = cdev_add(&pcdevX_data->cdev,pcdrv_data.device_number+id,1);
    if(ret < 0){
		pr_err("Cdev add failed\n");
		return ret;
	}

    pcdrv_data.device_pcd = device_create(pcdrv_data.class_pcd,NULL,pcdrv_data.device_number+id,NULL,"pcdev-%d",pdev->id);
    if(IS_ERR(pcdrv_data.device_pcd)){
		pr_err("Device create failed\n");
		ret = PTR_ERR(pcdrv_data.device_pcd);
		cdev_del(&pcdevX_data->cdev);
		return ret;
	}

    pcdrv_data.total_devices++;
    return 0;
}

int pcd_platform_driver_remove(struct platform_device *pdev)
{
    pr_info(" remove !!!\n");

    int ret;

    struct pcdev_private_data *pcdevX_data;
    pcdevX_data = pdev->id_entry->driver_data;

    int id = pdev->id;

    device_destroy(pcdrv_data.class_pcd,pcdrv_data.device_number+id);
    cdev_del(&pcdevX_data->cdev);

    pcdrv_data.total_devices--;

	pr_info("A device is removed\n");

	return 0;
}

struct platform_driver pcd_platform_driver = 
{
	.probe = pcd_platform_driver_probe,
	.remove = pcd_platform_driver_remove,
	.id_table = pcdevs_ids,
	.driver = {
		.name = "pseudo-char-device"
	}

};

static int __init pcd_platform_driver_init(void)
{
	int ret;

	ret = alloc_chrdev_region(&pcdrv_data.device_number,0,MAX_DEVICES,"pcdevs");
	if(ret < 0){
		pr_err("Alloc chrdev failed\n");
		return ret;
	}

	pcdrv_data.class_pcd = class_create(THIS_MODULE,"pcd_class");
	if(IS_ERR(pcdrv_data.class_pcd)){
		pr_err("Class creation failed\n");
		ret = PTR_ERR(pcdrv_data.class_pcd);
		unregister_chrdev_region(pcdrv_data.device_number,MAX_DEVICES);
		return ret;
	}

	/*Register a platform driver */
	platform_driver_register(&pcd_platform_driver);
	
	pr_info("pcd platform driver loaded\n");
	
	return 0;
}

static void __exit pcd_platform_driver_exit(void)
{
	/*Unregister the platform driver */
	platform_driver_unregister(&pcd_platform_driver);

	/*Class destroy */
	class_destroy(pcdrv_data.class_pcd);

	/*Unregister device numbers for MAX_DEVICES */
	unregister_chrdev_region(pcdrv_data.device_number,MAX_DEVICES);
	
	pr_info("pcd platform driver unloaded\n");

}

module_init(pcd_platform_driver_init);
module_exit(pcd_platform_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("minkha");
MODULE_DESCRIPTION("A pseudo character platform driver which handles n platform pcdevs");
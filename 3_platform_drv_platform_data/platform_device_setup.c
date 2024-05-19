#include<linux/module.h>
#include<linux/platform_device.h>

#undef pr_fmt
#define pr_fmt(fmt) "%s : " fmt,__func__

void pcdev_release(struct device *dev)
{
	pr_info("Device released \n");
}


struct platform_device platform_pcdev_1 = {
	.name = "pcdev-A1x",
	.id = 0
};


struct platform_device platform_pcdev_2 = {
	.name = "pcdev-B1x",
	.id = 1
};


struct platform_device platform_pcdev_3 = {
	.name = "pcdev-C1x",
	.id = 2
};


struct platform_device platform_pcdev_4 = {
	.name = "pcdev-D1x",
	.id = 3
};


struct platform_device *platform_pcdevs[] = 
{
	&platform_pcdev_1,
	&platform_pcdev_2,
	&platform_pcdev_3,
	&platform_pcdev_4
};

static int __init pcdev_platform_init(void)
{
	/* register n platform devices */

	//platform_device_register(&platform_pcdev_1);
	//platform_device_register(&platform_pcdev_2);
	
	platform_add_devices(platform_pcdevs,ARRAY_SIZE(platform_pcdevs) );

	pr_info("Device setup module loaded \n");

	return 0;
}


static void __exit pcdev_platform_exit(void)
{

	platform_device_unregister(&platform_pcdev_1);
	platform_device_unregister(&platform_pcdev_2);
	platform_device_unregister(&platform_pcdev_3);
	platform_device_unregister(&platform_pcdev_4);
	pr_info("Device setup module unloaded \n");


}

module_init(pcdev_platform_init);
module_exit(pcdev_platform_exit);


MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Module which registers n platform devices");
MODULE_AUTHOR("minkha");
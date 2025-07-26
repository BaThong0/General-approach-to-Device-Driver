#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/elfnote-lto.h>
#include <linux/export-internal.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;
BUILD_LTO_INFO;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif


static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0xb1ad28e0, "__gnu_mcount_nc" },
	{ 0x92997ed8, "_printk" },
	{ 0xefd6cf06, "__aeabi_unwind_cpp_pr0" },
	{ 0x37a0cba, "kfree" },
	{ 0x12e4b0fa, "serdev_device_close" },
	{ 0x9d669763, "memcpy" },
	{ 0x637493f3, "__wake_up" },
	{ 0xe3ec2f2b, "alloc_chrdev_region" },
	{ 0x729bab70, "cdev_init" },
	{ 0x95c1026a, "cdev_add" },
	{ 0x61907396, "__class_create" },
	{ 0xd43e80f0, "device_create" },
	{ 0x3a8831fe, "class_destroy" },
	{ 0x5318d808, "__serdev_device_driver_register" },
	{ 0x6091b333, "unregister_chrdev_region" },
	{ 0x800473f, "__cond_resched" },
	{ 0xfe487975, "init_wait_entry" },
	{ 0x1000e51, "schedule" },
	{ 0x647af474, "prepare_to_wait_event" },
	{ 0x49970de8, "finish_wait" },
	{ 0xf9a482f9, "msleep" },
	{ 0x3ea1b6e4, "__stack_chk_fail" },
	{ 0x8f678b07, "__stack_chk_guard" },
	{ 0xeacedcb4, "driver_unregister" },
	{ 0x8c7fb26f, "device_destroy" },
	{ 0x2bf9031b, "cdev_del" },
	{ 0xd9d9ad29, "serdev_device_write_buf" },
	{ 0xc4bda6b9, "devm_kmalloc" },
	{ 0x47baf178, "serdev_device_open" },
	{ 0xe702d4c8, "kmalloc_caches" },
	{ 0x876500e2, "serdev_device_set_baudrate" },
	{ 0xe30fc911, "serdev_device_set_flow_control" },
	{ 0x78f7ccf0, "serdev_device_set_parity" },
	{ 0x5995f766, "kmalloc_trace" },
	{ 0x5f754e5a, "memset" },
	{ 0x51a910c0, "arm_copy_to_user" },
	{ 0x421d4dcf, "krealloc" },
	{ 0xae353d77, "arm_copy_from_user" },
	{ 0xc84d16dc, "module_layout" },
};

MODULE_INFO(depends, "");

MODULE_ALIAS("of:N*T*Cfpc,fpc1020a");
MODULE_ALIAS("of:N*T*Cfpc,fpc1020aC*");

MODULE_INFO(srcversion, "F3B43EC6C6D230E5808D77F");

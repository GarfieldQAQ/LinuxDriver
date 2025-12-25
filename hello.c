#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>

#define DEVICE_NAME "hello"
#define CLASS_NAME "hello_class"
#define BUF_SIZE 128

static dev_t devno;              // 设备号（主+次）
static struct cdev hello_cdev;   // 字符设备对象
static struct class *hello_class;

struct hello_file_ctx{
	char buf[BUF_SIZE];
	size_t len;
};





static int hello_open(struct inode *inode, struct file *file)
{
	struct hello_file_ctx *ctx;

    ctx = kmalloc(sizeof(*ctx), GFP_KERNEL);
    if (!ctx)
        return -ENOMEM;
	ctx->len = 0;
    file->private_data = ctx;

    pr_info("hello: open called, ctx=%px\n", ctx);
    return 0;
}

static int hello_close(struct inode *inode, struct file *file)
{
    struct hello_file_ctx *ctx = file->private_data;

    pr_info("hello: release called, ctx=%px\n", ctx);

    kfree(ctx);
    return 0;

}

static ssize_t hello_write(struct file *file,
                           const char __user *ubuf,
                           size_t count,
                           loff_t *ppos)
{
    struct hello_file_ctx *ctx = file->private_data;
    size_t to_copy;

    to_copy = min(count, (size_t)(BUF_SIZE - 1));

    if (copy_from_user(ctx->buf, ubuf, to_copy))
        return -EFAULT;

    ctx->buf[to_copy] = '\0';
    ctx->len = to_copy;

    pr_info("hello: write %zu bytes: %s\n", to_copy, ctx->buf);
    return to_copy;
}

static ssize_t hello_read(struct file *file,
                          char __user *ubuf,
                          size_t count,
                          loff_t *ppos)
{
    struct hello_file_ctx *ctx = file->private_data;

    if (ctx->len - *ppos <= 0)
        return 0;   // EOF

    if (count > ctx->len - *ppos)
        count = ctx->len - *ppos;

    if (copy_to_user(ubuf, ctx->buf + *ppos, count))
        return -EFAULT;

    *ppos += count;
    return count;
}



/* file_operations */
static struct file_operations hello_fops = {
    .owner   = THIS_MODULE,
    .open    = hello_open,
    .release = hello_close,
    .read  	 = hello_read,
    .write	 = hello_write,
};


static int __init hello_init(void)
{
	int ret;
	
	// 申请设备号
	ret = alloc_chrdev_region(&devno, 0, 1, DEVICE_NAME);
	if (ret < 0)
	{
		pr_err("alloc_chrdev_region failed\n");
        return ret;
	}
	
	// 初始化 dev 设备
	cdev_init(&hello_cdev, &hello_fops);
    hello_cdev.owner = THIS_MODULE;

	// 注册 dev 设备
	ret = cdev_add(&hello_cdev, devno, 1);
    if (ret < 0) {
        pr_err("cdev_add failed\n");
        goto unregister_dev;
    }

	// 创建 hello_class 类 
	hello_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(hello_class)) {
        pr_err("class_create failed\n");
        ret = PTR_ERR(hello_class);
        goto del_cdev;
    }

	// 创建设备节点
	device_create(hello_class, NULL, devno, NULL, DEVICE_NAME);

    pr_info("hello: module loaded, major=%d\n", MAJOR(devno));
    return 0;

del_cdev:
	cdev_del(&hello_cdev);
unregister_dev:
	unregister_chrdev_region(devno, 1);
	return ret;

}

static void __exit hello_exit(void)
{
	device_destroy(hello_class, devno);
    class_destroy(hello_class);
    cdev_del(&hello_cdev);
    unregister_chrdev_region(devno, 1);
    pr_info("hello: module unloaded\n");
	
}

module_init(hello_init);
module_exit(hello_exit);
MODULE_LICENSE("Dual BSD/GPL");


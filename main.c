#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/debugfs.h>
#include <linux/init.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/jiffies.h>
#include <linux/semaphore.h>

MODULE_LICENSE("GPL");

#define USERNAME "akmaral"
#define LEN 10

struct mutex lock;
struct dentry *root=0;
int ret;
char my_foo_buf[PAGE_SIZE];


static ssize_t my_read(struct file *f, char *buffer, 
			   size_t length, loff_t *offset)
{
	int res;
	char *read_from = USERNAME + *offset;
	size_t read_num = length < (LEN - *offset) ? length : (LEN - *offset);

	if (read_num == 0) {
		res = 0;
		return res;
	}

	res = copy_to_user(buffer, read_from, read_num);
	if (res == read_num) {
		res = -EIO;
	} else {
		*offset = LEN - res;
		res = read_num - res;
	}
	return res;
}

static ssize_t my_write(struct file *f, const char *buf, 
			   size_t len, loff_t *offset)
{
	char newbuf[LEN];
	ssize_t res;

	if (len != LEN) {
		res = -EINVAL;
		return res;
	}
	copy_from_user(newbuf, buf, LEN);
	if (strncmp(newbuf, USERNAME, LEN) == 0)
		res = LEN;
	else
		res = -EINVAL;

	return res;
}

static ssize_t my_user1_read(struct file *f, __user char *buffer,
			   size_t length, loff_t *offset)
{
	char *read_from = my_foo_buf + *offset;
	size_t read_num = length < (PAGE_SIZE - *offset) ? 
			  length : (PAGE_SIZE - *offset);

	ret = mutex_lock_interruptible(&lock);
	if (ret)
		return -1;

	if (read_num == 0) {
		ret = 0;
		mutex_unlock(&lock);
		return ret;
	}

	ret = copy_to_user(buffer, read_from, read_num);
	if (ret == read_num) {
		ret = -EIO;
	} else {
		*offset = PAGE_SIZE - ret;
		ret = read_num - ret;
	}
	mutex_unlock(&lock);
	return ret;
} 

static ssize_t my_user1_write(struct file *f, const char *buf, 
			   size_t len, loff_t *offset)
{
	int byte_write = 0;
	int append = 0;

	ret = mutex_lock_interruptible(&lock);
	if (ret)
		return -1;

	if (f->f_flags & O_APPEND) 
		append = strlen(my_foo_buf);
	if (*offset + append >= PAGE_SIZE)
		ret = -EINVAL;
	while ((byte_write < len) && (*offset + append < PAGE_SIZE))
	{
		get_user(my_foo_buf[append + *offset], &buf[byte_write]);
		*offset = *offset + 1;
		byte_write++;
	}
	mutex_unlock(&lock);
	return byte_write ? byte_write : ret;
}

static struct file_operations my_user2_fops = {
  .read = my_read,
  .write = my_write,
};

static struct file_operations my_user1_fops = {
  .read = my_user1_read,
  .write = my_user1_write,
};

int __init hello_init(void)
{
	root = debugfs_create_dir("fortytwo", 0);
    if (!root) {
        // Abort module load.
        printk(KERN_ALERT "debugfs_example1: failed to create /sys/kernel/debug/fortytwo\n");
        return -1;
    }
	if (!(debugfs_create_file("user2", 0666, root, NULL, &my_user2_fops) &&
                debugfs_create_ulong("jiffies", 0444, root, (long unsigned int *)&jiffies) &&
				debugfs_create_file("user1", 0666, root, NULL, &my_user1_fops)))
		return -1;
	mutex_init(&lock);
	return 0;

}

void __exit hello_exit(void)
{
	printk(KERN_INFO "Cleaning up module.\n");
	debugfs_remove_recursive(root);
}

module_init(hello_init);
module_exit(hello_exit);

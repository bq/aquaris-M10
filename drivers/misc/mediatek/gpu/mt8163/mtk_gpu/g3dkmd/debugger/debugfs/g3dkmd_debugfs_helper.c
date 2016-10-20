/*
 * Copyright (c) 2014 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>


#include "g3dbase_common_define.h"
#include "yl_define.h"
#include "g3dkmd_define.h"


/* debugfs helper functions */

static int my_seq_open(struct inode *inode, struct file *file)
{
	return single_open(file, inode->i_private, NULL);
}


static const struct file_operations debugfs_seq_fops = {
	.owner = THIS_MODULE,
	.open = my_seq_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};

void _g3dKmdCreateDebugfsFileNode(const char *name, struct dentry *parentNode,
				  int (*func)(struct seq_file *, void *))
{
	debugfs_create_file(name, (S_IRUGO | S_IWUSR), parentNode,
				func, /* pass to inode.i_private when open(). */
				&debugfs_seq_fops);

}

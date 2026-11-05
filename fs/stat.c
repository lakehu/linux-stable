/*
 *  linux/fs/stat.c
 *
 *  (C) 1991  Linus Torvalds
 */

#include <errno.h>
#include <sys/stat.h>

#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <asm/segment.h>

static void cp_stat(struct m_inode * inode, struct stat * statbuf)
{
	struct stat tmp;
	int i;

	verify_area(statbuf,sizeof (* statbuf));
	tmp.st_dev = inode->i_dev;
	tmp.st_ino = inode->i_num;
	tmp.st_mode = inode->i_mode;
	tmp.st_nlink = inode->i_nlinks;
	tmp.st_uid = inode->i_uid;
	tmp.st_gid = inode->i_gid;
	tmp.st_rdev = inode->i_zone[0];
	tmp.st_size = inode->i_size;
	tmp.st_atime = inode->i_atime;
	tmp.st_mtime = inode->i_mtime;
	tmp.st_ctime = inode->i_ctime;
	for (i=0 ; i<sizeof (tmp) ; i++)
		put_fs_byte(((char *) &tmp)[i],&((char *) statbuf)[i]);
}

int sys_stat(char * filename, struct stat * statbuf)
{
	struct m_inode * inode;

	if (!(inode=namei(filename)))
		return -ENOENT;
	cp_stat(inode,statbuf);
	iput(inode);
	return 0;
}

int sys_fstat(unsigned int fd, struct stat * statbuf)
{
	struct file * f;
	struct m_inode * inode;

	if (fd >= NR_OPEN || !(f=current->filp[fd]) || !(inode=f->f_inode))
		return -EBADF;
	cp_stat(inode,statbuf);
	return 0;
}

/*
 * stat64/lstat64/fstat64 for Linux programs (uClibc-ng uses them for
 * stat(), lstat() and fstat()), after the Linux 0.01 remake by Abdel
 * Benamrouche (patch_rm_3.3, ref/linux-0.01-rm/). There are no symbolic
 * links, so lstat64 is stat64.
 */
static void cp_stat64(struct m_inode * inode, struct stat64 * statbuf)
{
	struct stat64 tmp;
	int i;

	verify_area(statbuf,sizeof (* statbuf));
	for (i=0 ; i<sizeof (tmp) ; i++)
		((char *) &tmp)[i] = 0;
	tmp.st_dev = inode->i_dev;
	tmp.st_ino = tmp.__st_ino = inode->i_num;
	tmp.st_mode = inode->i_mode;
	tmp.st_nlink = inode->i_nlinks;
	tmp.st_uid = inode->i_uid;
	tmp.st_gid = inode->i_gid;
	tmp.st_rdev = inode->i_zone[0];
	tmp.st_size = inode->i_size;
	tmp.st_blksize = BLOCK_SIZE;
	tmp.st_blocks = (inode->i_size + 511) / 512;
	tmp.st_atime = inode->i_atime;
	tmp.st_mtime = inode->i_mtime;
	tmp.st_ctime = inode->i_ctime;
	for (i=0 ; i<sizeof (tmp) ; i++)
		put_fs_byte(((char *) &tmp)[i],&((char *) statbuf)[i]);
}

int sys_stat64(char * filename, struct stat64 * statbuf)
{
	struct m_inode * inode;

	if (!(inode=namei(filename)))
		return -ENOENT;
	cp_stat64(inode,statbuf);
	iput(inode);
	return 0;
}

int sys_lstat64(char * filename, struct stat64 * statbuf)
{
	return sys_stat64(filename,statbuf);
}

int sys_fstat64(unsigned int fd, struct stat64 * statbuf)
{
	struct file * f;
	struct m_inode * inode;

	if (fd >= NR_OPEN || !(f=current->filp[fd]) || !(inode=f->f_inode))
		return -EBADF;
	cp_stat64(inode,statbuf);
	return 0;
}

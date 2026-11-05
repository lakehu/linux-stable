/*
 *  linux/fs/readdir.c
 *
 * getdents64 (220) for Linux programs: uClibc-ng's readdir() uses it.
 * Same Linux ABI as later kernels: as many struct linux_dirent64 records
 * as fit into the buffer, each d_reclen bytes long (8-byte aligned),
 * d_off = file position after the entry, and the number of bytes filled
 * as return value (0 at the end of the directory).
 *
 * The Linux 0.01 remake by Abdel Benamrouche (patch_rm_3.1/3.3, see
 * ref/linux-0.01-rm/) returns one entry per call, with the name length
 * as return value and as d_reclen. That only works with its own dietlibc
 * (readdir64() fetches again after each entry); uClibc-ng copies
 * d_reclen - 19 bytes of name and crashes.
 */

#include <errno.h>
#include <sys/stat.h>

#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <asm/segment.h>

/* offsets in struct linux_dirent64 (u64 d_ino, s64 d_off, ...) */
#define D_INO		0
#define D_OFF		8
#define D_RECLEN	16
#define D_TYPE		18		/* DT_UNKNOWN (0): ask stat() */
#define D_NAME		19

int sys_getdents64(unsigned int fd, char * dirp, unsigned int count)
{
	struct file * file;
	struct m_inode * inode;
	struct buffer_head * bh;
	struct dir_entry * de;
	char rec[(D_NAME + NAME_LEN + 1 + 7) & ~7];
	int block, len, reclen, i, done = 0;

	if (fd >= NR_OPEN || !(file = current->filp[fd]) ||
	    !(inode = file->f_inode))
		return -EBADF;
	if (!S_ISDIR(inode->i_mode))
		return -ENOTDIR;
	verify_area(dirp, count);
	while (file->f_pos < inode->i_size) {
		if (!(block = bmap(inode, file->f_pos / BLOCK_SIZE)) ||
		    !(bh = bread(inode->i_dev, block))) {
			file->f_pos += BLOCK_SIZE - file->f_pos % BLOCK_SIZE;
			continue;
		}
		de = (struct dir_entry *) (bh->b_data + file->f_pos % BLOCK_SIZE);
		if (de->inode) {
			for (len = 0 ; len < NAME_LEN && de->name[len] ; len++)
				/* nothing */ ;
			reclen = (D_NAME + len + 1 + 7) & ~7;
			if (done + reclen > count) {
				brelse(bh);
				return done ? done : -EINVAL;
			}
			for (i = 0 ; i < reclen ; i++)
				rec[i] = 0;
			*(unsigned long *) (rec + D_INO) = de->inode;
			*(unsigned long *) (rec + D_OFF) =
				file->f_pos + sizeof (struct dir_entry);
			*(unsigned short *) (rec + D_RECLEN) = reclen;
			for (i = 0 ; i < len ; i++)
				rec[D_NAME + i] = de->name[i];
			for (i = 0 ; i < reclen ; i++)
				put_fs_byte(rec[i], dirp + done + i);
			done += reclen;
		}
		file->f_pos += sizeof (struct dir_entry);
		brelse(bh);
	}
	return done;
}

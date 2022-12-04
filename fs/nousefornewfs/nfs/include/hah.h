#ifndef _HAH_H_
#define _HAH_H_

#define FUSE_USE_VERSION 26
#include "stdio.h"
#include "stdlib.h"
#include <unistd.h>
#include "fcntl.h"
#include "string.h"
#include "fuse.h"
#include <stddef.h>
#include "ddriver.h"
#include "errno.h"
#include "types.h"

#define HAH_MAGIC                  /* TODO: Define by yourself */
#define HAH_DEFAULT_PERM    0777   /* 全权限打开 */

/******************************************************************************
* SECTION: hah.c
*******************************************************************************/
void* 			   hah_init(struct fuse_conn_info *);
void  			   hah_destroy(void *);
int   			   hah_mkdir(const char *, mode_t);
int   			   hah_getattr(const char *, struct stat *);
int   			   hah_readdir(const char *, void *, fuse_fill_dir_t, off_t,
						                struct fuse_file_info *);
int   			   hah_mknod(const char *, mode_t, dev_t);
int   			   hah_write(const char *, const char *, size_t, off_t,
					                  struct fuse_file_info *);
int   			   hah_read(const char *, char *, size_t, off_t,
					                 struct fuse_file_info *);
int   			   hah_access(const char *, int);
int   			   hah_unlink(const char *);
int   			   hah_rmdir(const char *);
int   			   hah_rename(const char *, const char *);
int   			   hah_utimens(const char *, const struct timespec tv[2]);
int   			   hah_truncate(const char *, off_t);
			
int   			   hah_open(const char *, struct fuse_file_info *);
int   			   hah_opendir(const char *, struct fuse_file_info *);

#endif  /* _hah_H_ */
 #include "newfs.h"
/******************************************************************************
* SECTION: 宏定义
*******************************************************************************/
#define OPTION(t, p)        { t, offsetof(struct custom_options, p), 1 }

/******************************************************************************
* SECTION: 全局变量
*******************************************************************************/
static const struct fuse_opt option_spec[] = {		/* 用于FUSE文件系统解析参数 */
	OPTION("--device=%s", device),
	FUSE_OPT_END
};

struct custom_options newfs_options;			 /* 全局选项 */
struct newfs_super newfs_super; 
/******************************************************************************
* SECTION: FUSE操作定义
*******************************************************************************/
static struct fuse_operations operations = {
	.init = newfs_init,						 /* mount文件系统 */		
	.destroy = newfs_destroy,				 /* umount文件系统 */
	.mkdir = newfs_mkdir,					 /* 建目录，mkdir */
	.getattr = newfs_getattr,				 /* 获取文件属性，类似stat，必须完成 */
	.readdir = newfs_readdir,				 /* 填充dentrys */
	.mknod = newfs_mknod,					 /* 创建文件，touch相关 */
	.write = newfs_write,								  	 /* 写入文件 */
	.read = newfs_read,								  	 /* 读文件 */
	.utimens = newfs_utimens,				 /* 修改时间，忽略，避免touch报错 */
	.truncate = NULL,						  		 /* 改变文件大小 */
	.unlink = NULL,							  		 /* 删除文件 */
	.rmdir	= NULL,							  		 /* 删除目录， rm -r */
	.rename = NULL,							  		 /* 重命名，mv */

	.open = NULL,							
	.opendir = NULL,
	.access = NULL
};
/******************************************************************************
* SECTION: 必做函数实现
*******************************************************************************/
/**
 * @brief 挂载（mount）文件系统
 * 
 * @param conn_info 可忽略，一些建立连接相关的信息 
 * @return void*
 */
void* newfs_init(struct fuse_conn_info * conn_info) {
	/* TODO: 在这里进行挂载 */
	if (newfs_mount(newfs_options) != NEWFS_ERROR_NONE) {
        NEWFS_DBG("[%s] mount error\n", __func__);
		fuse_exit(fuse_get_context()->fuse);
		return NULL ;
	} 
	return NULL;
	/* 下面是一个控制设备的示例 */
	//  newfs_super.fd = ddriver_open(newfs_options.device);

	// return NULL;
}

/**
 * @brief 卸载（umount）文件系统
 * 
 * @param p 可忽略
 * @return void
 */
void newfs_destroy(void* p) {
	/* TODO: 在这里进行卸载 */
	if (newfs_umount() != NEWFS_ERROR_NONE) {
		NEWFS_DBG("[%s] unmount error\n", __func__);
		fuse_exit(fuse_get_context()->fuse);
		return;
	}
	return;

	// ddriver_close(newfs_super.fd);

	// return;
}

/**
 * @brief 创建目录
 * 
 * @param path 相对于挂载点的路径
 * @param mode 创建模式（只读？只写？），可忽略
 * @return int 0成功，否则失败
 */
int newfs_mkdir(const char* path, mode_t mode) {
	/* TODO: 解析路径，创建目录 */
	(void)mode;
	boolean is_find, is_root;
	char* fname;
	struct newfs_dentry* last_dentry = newfs_lookup(path, &is_find, &is_root);
	struct newfs_dentry* dentry;
	struct newfs_inode*  inode;
	
	if (is_find) {
		return -NEWFS_ERROR_EXISTS;
	}

	if (NEWFS_IS_REG(last_dentry->inode)) {
		return -NEWFS_ERROR_UNSUPPORTED;
	}

	fname  = newfs_get_fname(path);
	dentry = new_dentry(fname, NEWFS_DIR); 
	dentry->parent = last_dentry;
	inode  = newfs_alloc_inode(dentry);
	newfs_alloc_dentry(last_dentry->inode, dentry);
	return NEWFS_ERROR_NONE;
}

/**
 * @brief 获取文件或目录的属性，该函数非常重要
 * 
 * @param path 相对于挂载点的路径
 * @param newfs_stat 返回状态
 * @return int 0成功，否则失败
 */
int newfs_getattr(const char* path, struct stat * newfs_stat) {
	/* TODO: 解析路径，获取Inode，填充newfs_stat，可参考/fs/simplefs/newfs.c的newfs_getattr()函数实现 */
	boolean	is_find, is_root;
	struct newfs_dentry* dentry = newfs_lookup(path, &is_find, &is_root);
	if (is_find == FALSE) {
		return -NEWFS_ERROR_NOTFOUND;
	}
	if (NEWFS_IS_DIR(dentry->inode)) {
		newfs_stat->st_mode = S_IFDIR | NEWFS_DEFAULT_PERM;
		newfs_stat->st_size = dentry->inode->dir_cnt * sizeof(struct newfs_dentry_d);
	}
	else if (NEWFS_IS_REG(dentry->inode)) {
		newfs_stat->st_mode = S_IFREG | NEWFS_DEFAULT_PERM;
		newfs_stat->st_size = dentry->inode->size;
	}

	newfs_stat->st_nlink = 1;
	newfs_stat->st_uid 	 = getuid();
	newfs_stat->st_gid 	 = getgid();
	newfs_stat->st_atime   = time(NULL);
	newfs_stat->st_mtime   = time(NULL);
	newfs_stat->st_blksize = NEWFS_BLKS_SZ(1);
	

	if (is_root) {
		newfs_stat->st_size	= newfs_super.sz_usage; 
		newfs_stat->st_blocks = NEWFS_DISK_SZ() / NEWFS_BLKS_SZ(1);
		newfs_stat->st_nlink  = 2;		/* !特殊，根目录link数为2 */
	}

	return NEWFS_ERROR_NONE;
}

/**
 * @brief 遍历目录项，填充至buf，并交给FUSE输出
 * 
 * @param path 相对于挂载点的路径
 * @param buf 输出buffer
 * @param filler 参数讲解:
 * 
 * typedef int (*fuse_fill_dir_t) (void *buf, const char *name,
 *				const struct stat *stbuf, off_t off)
 * buf: name会被复制到buf中
 * name: dentry名字
 * stbuf: 文件状态，可忽略
 * off: 下一次offset从哪里开始，这里可以理解为第几个dentry
 * 
 * @param offset 第几个目录项？
 * @param fi 可忽略
 * @return int 0成功，否则失败
 */
int newfs_readdir(const char * path, void * buf, fuse_fill_dir_t filler, off_t offset,
			    		 struct fuse_file_info * fi) {
    /* TODO: 解析路径，获取目录的Inode，并读取目录项，利用filler填充到buf，可参考/fs/simplefs/newfs.c的newfs_readdir()函数实现 */
   
     boolean	is_find, is_root;
	int		cur_dir = offset;

	struct newfs_dentry* dentry = newfs_lookup(path, &is_find, &is_root);
	struct newfs_dentry* sub_dentry;
	struct newfs_inode* inode;
	if (is_find) {
		inode = dentry->inode;
		sub_dentry = newfs_get_dentry(inode, cur_dir);
		if (sub_dentry) {
			filler(buf, sub_dentry->fname, NULL, ++offset);
		}
		return NEWFS_ERROR_NONE;
	}
	return -NEWFS_ERROR_NOTFOUND;
}

/**
 * @brief 创建文件
 * 
 * @param path 相对于挂载点的路径
 * @param mode 创建文件的模式，可忽略
 * @param dev 设备类型，可忽略
 * @return int 0成功，否则失败
 */
int newfs_mknod(const char* path, mode_t mode, dev_t dev) {
	/* TODO: 解析路径，并创建相应的文件 */
		boolean	is_find, is_root;
	
	struct newfs_dentry* last_dentry = newfs_lookup(path, &is_find, &is_root);
	struct newfs_dentry* dentry;
	struct newfs_inode* inode;
	char* fname;
	
	if (is_find == TRUE) {
		return -NEWFS_ERROR_EXISTS;
	}

	fname = newfs_get_fname(path);
	
	if (S_ISREG(mode)) {
		dentry = new_dentry(fname, NEWFS_REG_FILE);
	}
	else if (S_ISDIR(mode)) {
		dentry = new_dentry(fname, NEWFS_DIR);
	}
	else {
		dentry = new_dentry(fname, NEWFS_REG_FILE);
	}
	dentry->parent = last_dentry;
	inode = newfs_alloc_inode(dentry);
	newfs_alloc_dentry(last_dentry->inode, dentry);
	return NEWFS_ERROR_NONE;
}

/**
 * @brief 修改时间，为了不让touch报错 
 * 
 * @param path 相对于挂载点的路径
 * @param tv 实践
 * @return int 0成功，否则失败
 */
int newfs_utimens(const char* path, const struct timespec tv[2]) {
	(void)path;
	return NEWFS_ERROR_NONE;
}
/******************************************************************************
* SECTION: 选做函数实现 	
*******************************************************************************/
/**
 * @brief 
 * 
 * @param path 
 * @param buf 
 * @param size 
 * @param offset 
 * @param fi 
 * @return int 
 */
int newfs_read(const char* path, char* buf, size_t size, off_t offset,
		       struct fuse_file_info* fi) {
	boolean	is_find, is_root;
	struct newfs_dentry* dentry = newfs_lookup(path, &is_find, &is_root);
	struct newfs_inode*  inode;

	if (is_find == FALSE) {
		return -NEWFS_ERROR_NOTFOUND;
	}

	inode = dentry->inode;
	
	if (NEWFS_IS_DIR(inode)) {
		return -NEWFS_ERROR_ISDIR;	
	}

	if (inode->size < offset) {
		return -NEWFS_ERROR_SEEK;
	}
	size_t size_remian = size;
	off_t offset_remain = offset;
	for(int i = 0; i < NEWFS_DATA_PER_FILE; i++){
		if(offset_remain > NEWFS_BLKS_SZ(1)){
			offset_remain -= NEWFS_BLKS_SZ(1);
		}
		else{
			off_t bufcnt = 0;
			while(i < NEWFS_DATA_PER_FILE && size_remian > NEWFS_BLKS_SZ(1)- offset_remain){
				memcpy(buf + bufcnt , inode->block_pointer[i] + offset_remain, NEWFS_BLKS_SZ(1) - offset_remain);
				size_remian = size_remian - NEWFS_BLKS_SZ(1) + offset_remain ;
				bufcnt = bufcnt + NEWFS_BLKS_SZ(1) - offset_remain;
				offset_remain = 0;
				i++;
			}		
				memcpy(buf + bufcnt, inode->block_pointer[i] + offset_remain, size_remian);

			
		}
		
	}
	
	return size;			   
}

/**
 * @brief 
 * 
 * @param path 
 * @param buf 
 * @param size 
 * @param offset 
 * @param fi 
 * @return int 
 */
int newfs_write(const char* path, const char* buf, size_t size, off_t offset,
		        struct fuse_file_info* fi) {
    boolean	is_find, is_root;
	struct newfs_dentry* dentry = newfs_lookup(path, &is_find, &is_root);
	struct newfs_inode*  inode;

	if (is_find == FALSE) {
		return -NEWFS_ERROR_NOTFOUND;
	}

	inode = dentry->inode;
	
	if (NEWFS_IS_DIR(inode)) {
		return -NEWFS_ERROR_ISDIR;	
	}
	if (inode->size < offset) {
		return -NEWFS_ERROR_SEEK;
	}
	size_t size_remian = size;
	off_t offset_remain = offset;
	for(int i = 0; i < NEWFS_DATA_PER_FILE; i++){
		if(offset_remain > NEWFS_BLKS_SZ(1)){
			offset_remain -= NEWFS_BLKS_SZ(1);
		}
		else{
			off_t bufcnt = 0;
			while(i < NEWFS_DATA_PER_FILE && size_remian > NEWFS_BLKS_SZ(1) - offset_remain){
				memcpy(inode->block_pointer[i] + offset_remain, buf + bufcnt, NEWFS_BLKS_SZ(1) - offset_remain);
				size_remian = size_remian - (NEWFS_BLKS_SZ(1) - offset_remain) ;
				bufcnt = bufcnt + NEWFS_BLKS_SZ(1) - offset_remain;
				offset_remain = 0;
				i++;
			}
			
			memcpy(inode->block_pointer[i] + offset_remain, buf + bufcnt * NEWFS_BLKS_SZ(1), size_remian);

			
			
		}
		
	}

	// memcpy(inode->data + offset, buf, size);
	inode->size = offset + size > inode->size ? offset + size : inode->size;
	
	return size;
}

/**
 * @brief 删除文件
 * 
 * @param path 相对于挂载点的路径
 * @return int 0成功，否则失败
 */
int newfs_unlink(const char* path) {
	/* 选做 */
	return 0;
}

/**
 * @brief 删除目录
 * 
 * 一个可能的删除目录操作如下：
 * rm ./tests/mnt/j/ -r
 *  1) Step 1. rm ./tests/mnt/j/j
 *  2) Step 2. rm ./tests/mnt/j
 * 即，先删除最深层的文件，再删除目录文件本身
 * 
 * @param path 相对于挂载点的路径
 * @return int 0成功，否则失败
 */
int newfs_rmdir(const char* path) {
	/* 选做 */
	return 0;
}

/**
 * @brief 重命名文件 
 * 
 * @param from 源文件路径
 * @param to 目标文件路径
 * @return int 0成功，否则失败
 */
int newfs_rename(const char* from, const char* to) {
	/* 选做 */
	return 0;
}

/**
 * @brief 打开文件，可以在这里维护fi的信息，例如，fi->fh可以理解为一个64位指针，可以把自己想保存的数据结构
 * 保存在fh中
 * 
 * @param path 相对于挂载点的路径
 * @param fi 文件信息
 * @return int 0成功，否则失败
 */
int newfs_open(const char* path, struct fuse_file_info* fi) {
	/* 选做 */
	return 0;
}

/**
 * @brief 打开目录文件
 * 
 * @param path 相对于挂载点的路径
 * @param fi 文件信息
 * @return int 0成功，否则失败
 */
int newfs_opendir(const char* path, struct fuse_file_info* fi) {
	/* 选做 */
	return 0;
}

/**
 * @brief 改变文件大小
 * 
 * @param path 相对于挂载点的路径
 * @param offset 改变后文件大小
 * @return int 0成功，否则失败
 */
int newfs_truncate(const char* path, off_t offset) {
	/* 选做 */
	return 0;
}


/**
 * @brief 访问文件，因为读写文件时需要查看权限
 * 
 * @param path 相对于挂载点的路径
 * @param type 访问类别
 * R_OK: Test for read permission. 
 * W_OK: Test for write permission.
 * X_OK: Test for execute permission.
 * F_OK: Test for existence. 
 * 
 * @return int 0成功，否则失败
 */
int newfs_access(const char* path, int type) {
	/* 选做: 解析路径，判断是否存在 */
	return 0;
}	
/******************************************************************************
* SECTION: FUSE入口
*******************************************************************************/
int main(int argc, char **argv)
{
    int ret;
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
	
	newfs_options.device = strdup("TODO: 这里填写你的ddriver设备路径");

	if (fuse_opt_parse(&args, &newfs_options, option_spec, NULL) == -1)
		return -1;

	ret = fuse_main(args.argc, args.argv, &operations, NULL);
		
	fuse_opt_free_args(&args);
	return ret;
}
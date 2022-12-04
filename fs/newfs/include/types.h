#ifndef _TYPES_H_
#define _TYPES_H_

/******************************************************************************
* SECTION: Type def
*******************************************************************************/
typedef int          boolean;
typedef uint16_t     flag16;

typedef enum NEWFS_file_type {
    NEWFS_REG_FILE,
    NEWFS_DIR,
    NEWFS_SYM_LINK
} NEWFS_FILE_TYPE;

/******************************************************************************
* SECTION: Macro
*******************************************************************************/
#define TRUE                    1
#define FALSE                   0
#define UINT32_BITS             32
#define UINT8_BITS              8

#define NEWFS_MAGIC_NUM           0x52415453  
#define NEWFS_SUPER_OFS           0
#define NEWFS_ROOT_INO            0


#define MAX_NAME_LEN    128     

#define MAX_FILES_DATA_NUMBER  4    



#define NEWFS_ERROR_NONE          0
#define NEWFS_ERROR_ACCESS        EACCES
#define NEWFS_ERROR_SEEK          ESPIPE     
#define NEWFS_ERROR_ISDIR         EISDIR
#define NEWFS_ERROR_NOSPACE       ENOSPC
#define NEWFS_ERROR_EXISTS        EEXIST
#define NEWFS_ERROR_NOTFOUND      ENOENT
#define NEWFS_ERROR_UNSUPPORTED   ENXIO
#define NEWFS_ERROR_IO            EIO     /* Error Input/Output */
#define NEWFS_ERROR_INVAL         EINVAL  /* Invalid Args */

#define NEWFS_MAX_FILE_NAME       128
// #define NEWFS_INODE_PER_FILE      4
#define NEWFS_DATA_PER_FILE       4
#define NEWFS_DEFAULT_PERM        0777

#define NEWFS_IOC_MAGIC           'S'
#define NEWFS_IOC_SEEK            _IO(NEWFS_IOC_MAGIC, 0)

#define NEWFS_FLAG_BUF_DIRTY      0x1
#define NEWFS_FLAG_BUF_OCCUPY     0x2


// 规定每个部分的大小
#define NEWFS_SUPER_BLK 1
#define NEWFS_INODE_NUM 512
#define NEWFS_MAP_INODE_BLKS 1
#define NEWFS_DATA_NUM   2048
#define NEWFS_MAP_DATA_BLKS  1
#define NEWFS_IO_SIZE 512
#define NEWFS_BLOCK_SIZE 1024

/******************************************************************************
* SECTION: Macro Function
*******************************************************************************/
#define NEWFS_IO_SZ()                     (newfs_super.sz_io)
#define NEWFS_DISK_SZ()                   (newfs_super.sz_disk)
#define NEWFS_DRIVER()                    (newfs_super.driver_fd)

#define NEWFS_ROUND_DOWN(value, round)    (value % round == 0 ? value : (value / round) * round)
#define NEWFS_ROUND_UP(value, round)      (value % round == 0 ? value : (value / round + 1) * round)

#define NEWFS_BLKS_SZ(blks)               (blks * 2 * NEWFS_IO_SZ()) //一块的大小为2个io
#define NEWFS_ASSIGN_FNAME(pNEWFS_dentry, _fname) memcpy(pNEWFS_dentry->fname, _fname, strlen(_fname))
#define NEWFS_INO_OFS(ino)                (newfs_super.inode_offset + ino * NEWFS_BLKS_SZ(1))
#define NEWFS_DATA_OFS(bno)               (newfs_super.data_offset + bno * NEWFS_BLKS_SZ(1))

#define NEWFS_IS_DIR(pinode)              (pinode->dentry->ftype == NEWFS_DIR)
#define NEWFS_IS_REG(pinode)              (pinode->dentry->ftype == NEWFS_REG_FILE)
/******************************************************************************
* SECTION: FS Specific Structure - In memory structure
****************************************************************************/



struct newfs_inode;
struct newfs_dentry;
struct newfs_super;

struct custom_options {
	const char*        device;
};



struct newfs_super {
    uint32_t magic;
    int      fd;
    /* TODO: Define yourself */

    int                driver_fd;

    int                sz_io;/*512B*/
    int                sz_disk;/*4MB*/
    int                sz_blk;/*1024B, 一块的大小*/ 
    int                sz_usage;

    int                map_inode_blks;/*inode位图所占的数据块*/
    int                map_inode_offset;/*inode位图的起始地址*/
    int                inode_offset; /*索引结点的偏移*/
    uint8_t*           map_inode;/*inode位图*/
    int                max_ino;/*inode的数目*/

    int                map_data_offset;/*data位图的起始地址*/
    int                map_data_blks; /*data位图占用的块数*/
    int                data_offset;  /*数据块的起始地址*/
    uint8_t*           map_data;/*data位图*/
    int                max_data;/*num of data*/

    boolean            is_mounted;

    struct newfs_dentry* root_dentry;/*根目录*/

};

struct newfs_inode {
    uint32_t ino;                              /* 在inode位图中的下标 */
    /* TODO: Define yourself */
    int                size;                          /* 文件已占用空间 */
    int                dir_cnt;
    struct newfs_dentry* dentry;                        /* 指向该inode的dentry, 父节点 */
    struct newfs_dentry* dentrys;                       /* 所有目录项 */
    uint8_t*           block_pointer[NEWFS_DATA_PER_FILE];     /*节点*/
    int                block_num[NEWFS_DATA_PER_FILE];       /*数据块*/

};

struct newfs_dentry {
    char     fname[MAX_NAME_LEN];
    uint32_t ino;
    /* TODO: Define yourself */
    struct newfs_dentry* parent;                        /* 父亲Inode的dentry */
    struct newfs_dentry* brother;                       /* 兄弟 */
    struct newfs_inode*  inode;                         /* 指向inode */
    NEWFS_FILE_TYPE      ftype;
};



static inline struct newfs_dentry* new_dentry(char * fname, NEWFS_FILE_TYPE ftype) {
    struct newfs_dentry * dentry = (struct newfs_dentry *)malloc(sizeof(struct newfs_dentry));
    memset(dentry, 0, sizeof(struct newfs_dentry));
    NEWFS_ASSIGN_FNAME(dentry, fname);
    dentry->ftype   = ftype;
    dentry->ino     = -1;
    dentry->inode   = NULL;
    dentry->parent  = NULL;
    dentry->brother = NULL; 

    return dentry;                                           
}
/******************************************************************************
* SECTION: FS Specific Structure - Disk structure
*******************************************************************************/
struct newfs_super_d
{
    uint32_t           magic_num;
    int                sz_usage;
    
    int                max_ino;
    int                map_inode_blks;
    int                map_inode_offset;
    int                inode_offset; /*inode起始位置*/

    int                max_data;
    int                map_data_blks;
    int                map_data_offset;
    int                data_offset;
};

struct newfs_inode_d
{
    int                ino;                           /* 在inode位图中的下标 */
    int                size;                          /* 文件已占用空间 */
    int                dir_cnt;
    NEWFS_FILE_TYPE      ftype;   
    int                block_num[NEWFS_DATA_PER_FILE];

};  

struct newfs_dentry_d
{
    char               fname[NEWFS_MAX_FILE_NAME];
    NEWFS_FILE_TYPE      ftype;
    int                ino;                           /* 指向的ino号 */
};  


#endif /* _TYPES_H_ */
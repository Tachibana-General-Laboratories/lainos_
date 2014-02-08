#ifndef FILE_H
#define FILE_H

struct fs_node;

typedef uint32_t (read_type_t) (struct fs_node *, uint32_t offset, uint32_t size, uint8_t *buffer);
typedef uint32_t (write_type_t) (struct fs_node *, uint32_t offset, uint32_t size, uint8_t *buffer);
typedef void (open_type_t) (struct fs_node *, uint32_t flags);
typedef void (close_type_t) (struct fs_node *);
typedef struct dirent *(readdir_type_t) (struct fs_node *, uint32_t index);
typedef struct fs_node *(finddir_type_t) (struct fs_node *, char *name);

typedef void (mknod_type_t) (struct fs_node *, char *name, uint16_t type, uint16_t permission, uint32_t dev);
typedef void (unlink_type_t) (struct fs_node *, char *name);

typedef int (ioctl_type_t) (struct fs_node *, int request, void * argp);
typedef int (get_size_type_t) (struct fs_node *);
typedef int (chmod_type_t) (struct fs_node *, int mode);


#define FS_UNDEFINED   0x00
#define FS_FILE        0x01
#define FS_DIRECTORY   0x02
#define FS_CHARDEVICE  0x04
#define FS_BLOCKDEVICE 0x08
#define FS_PIPE        0x10
#define FS_SYMLINK     0x20
#define FS_MOUNTPOINT  0x40
#define FS_SOCKET      0x80


typedef struct fs_node {
  enum { FD_NONE, FD_PIPE, FD_INODE } type;
  //uint32_t type;

  int ref; // reference count
  char readable;
  char writable;
  struct pipe *pipe;
  //struct inode *ip;
  void *ip;

  read_type_t *read;
  write_type_t *write;

  open_type_t *open;
  close_type_t *close;

  readdir_type_t *readdir;
  finddir_type_t *finddir;

  uint32_t offset;
} fs_node_t;

#endif

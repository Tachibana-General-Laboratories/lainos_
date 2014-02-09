//
// File descriptors
//

#include "types.h"
#include "defs.h"
#include "param.h"
#include "fs.h"
#include "file.h"
#include "spinlock.h"
#include "stat.h"
#include "fcntl.h"

struct devsw devsw[NDEV];
struct {
  struct spinlock lock;
  fs_node_t file[NFILE];
} ftable;

void
fileinit(void)
{
  initlock(&ftable.lock, "ftable");
}

// Allocate a file structure.
fs_node_t*
filealloc(void)
{
  fs_node_t *f;

  acquire(&ftable.lock);
  for(f = ftable.file; f < ftable.file + NFILE; f++){
    if(f->ref == 0){
      memset(f, 0, sizeof(fs_node_t));
      f->ref = 1;
      release(&ftable.lock);
      return f;
    }
  }
  release(&ftable.lock);
  return 0;
}

static fs_node_t* _root;

extern void sfs_root(fs_node_t *node);

fs_node_t*
vfs_kopen(char *path)
{
  if(!_root) {
    cprintf("alloc _root\n");
    _root = filealloc();
    if(!_root)
      panic("fileopen FAIL alloc _root");
    cprintf("init _root\n");
    sfs_root(_root);
    struct dirent d;

    cprintf("open _root\n");
    _root->open(_root, 0);
    cprintf("readdir _root\n");
    _root->readdir(_root, &d, 0);
    cprintf("IN ROOT: %s\n", d.d_name);
    cprintf("close _root\n");
    //_root->close(_root);
    fileclose(_root);
  }

  //struct fs_node_t* node;
  //_root

  return 0;
}

fs_node_t*
fileopen(char *path, int omode)
{
  fs_node_t *f;

  if((f = filealloc()) == 0)
    return 0;

  if(!_root){
    cprintf("alloc _root\n");
    _root = filealloc();
    if(!_root)
      panic("fileopen FAIL alloc _root");
    cprintf("init _root\n");
    sfs_root(_root);
    struct dirent d;

    cprintf("open _root\n");
    _root->open(_root, 0);
    cprintf("readdir _root\n");
    _root->readdir(_root, &d, 0);
    cprintf("IN ROOT: %s\n", d.d_name);
    cprintf("close _root\n");
    //_root->close(_root);
    fileclose(_root);
  }

  f->type = FD_INODE;
  f->offset = 0;
  f->readable = !(omode & O_WRONLY);
  f->writable = (omode & O_WRONLY) || (omode & O_RDWR);

  if(omode & O_CREATE){
    f->ip = sfs_createi_file(path);
    if(f->ip == 0) {
      fileclose(f);
      return 0;
    }
  } else {
    f->ip = sfs_openi(path, omode);
    if(f->ip == 0) {
      fileclose(f);
      return 0;
    }
  }

  return f;
}

// Increment ref count for file f.
fs_node_t*
filedup(fs_node_t *f)
{
  acquire(&ftable.lock);
  if(f->ref < 1)
    panic("filedup");
  f->ref++;
  release(&ftable.lock);
  return f;
}

// Close file f.  (Decrement ref count, close when reaches 0.)
void
fileclose(fs_node_t *f)
{
  fs_node_t ff;

  acquire(&ftable.lock);
  if(f->ref < 1)
    panic("fileclose");
  if(--f->ref > 0){
    release(&ftable.lock);
    return;
  }
  ff = *f;
  f->ref = 0;
  f->type = FD_NONE;
  release(&ftable.lock);

  if(ff.close) {
    ff.close(&ff);
    return;
  }

  if(ff.type == FD_PIPE)
    pipeclose(ff.pipe, ff.writable);
  else if(ff.type == FD_INODE){
    sfs_closei(ff.ip);
  }
}

// Get metadata about file f.
int
filestat(fs_node_t *f, struct stat *st)
{
  if(f->type == FD_INODE){
    sfs_stati(f->ip, st);
    return 0;
  }
  return -1;
}


uint32_t
sfs_read(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer);

uint32_t
sfs_write(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer);

// Read from file f.
int32_t
vfs_read(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer)
{
  if(node->readable == 0)
    return 0;

  if(node->read) {
    return node->read(node, offset, size, buffer);
  }

  if(node->type == FD_PIPE)
    return piperead(node->pipe, (void*)buffer, size);
  if(node->type == FD_INODE){
    return sfs_read(node, offset, size, buffer);
    //if((r = sfs_readi(f->ip, addr, f->offset, n)) > 0)
      //f->offset += r;
  }
  panic("fileread");
}

// Write to file f.
int32_t
vfs_write(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer)
{
  if(node->writable == 0)
    return -1;

  if(node->write) {
    return node->write(node, offset, size, buffer);
  }

  if(node->type == FD_PIPE)
    return pipewrite(node->pipe, (void*)buffer, size);
  if(node->type == FD_INODE){
    return sfs_write(node, offset, size, buffer);
    //if ((r = sfs_writei(f->ip, addr, f->offset, n)) > 0)
      //f->offset += r;
  }
  panic("filewrite");
}


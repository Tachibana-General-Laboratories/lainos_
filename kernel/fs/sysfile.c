//
// File-system system calls.
// Mostly argument checking, since we don't trust
// user code, and calls into file.c and fs.c.
//

#include "types.h"
#include "defs.h"
#include "param.h"
#include "stat.h"
#include "mmu.h"
#include "proc.h"
#include "fs.h"
#include "file.h"
#include "fcntl.h"

// Fetch the nth word-sized system call argument as a file descriptor
// and return both the descriptor and the corresponding struct file.
static int
argfd(int n, int *pfd, fs_node_t **pf)
{
  int fd;
  fs_node_t *f;

  if(argint(n, &fd) < 0)
    return -1;
  if(fd < 0 || fd >= NOFILE || (f=proc->ofile[fd]) == 0)
    return -1;
  if(pfd)
    *pfd = fd;
  if(pf)
    *pf = f;
  return 0;
}

// Allocate a file descriptor for the given file.
// Takes over file reference from caller on success.
static int
fdalloc(fs_node_t *f)
{
  int fd;

  for(fd = 0; fd < NOFILE; fd++){
    if(proc->ofile[fd] == 0){
      proc->ofile[fd] = f;
      return fd;
    }
  }
  return -1;
}

int
sys_dup(void)
{
  fs_node_t *f;
  int fd;
  
  if(argfd(0, 0, &f) < 0)
    return -1;
  if((fd=fdalloc(f)) < 0)
    return -1;
  filedup(f);
  return fd;
}

int
sys_read(void)
{
  fs_node_t *f;
  int n;
  char *p;

  if(argfd(0, 0, &f) < 0 || argint(2, &n) < 0 || argptr(1, &p, n) < 0)
    return -1;
  return fileread(f, p, n);
}

int
sys_write(void)
{
  fs_node_t *f;
  int n;
  char *p;

  if(argfd(0, 0, &f) < 0 || argint(2, &n) < 0 || argptr(1, &p, n) < 0)
    return -1;
  return filewrite(f, p, n);
}

int
sys_close(void)
{
  int fd;
  fs_node_t *f;
  
  if(argfd(0, &fd, &f) < 0)
    return -1;
  proc->ofile[fd] = 0;
  fileclose(f);
  return 0;
}

int
sys_fstat(void)
{
  fs_node_t *f;
  struct stat *st;
  
  if(argfd(0, 0, &f) < 0 || argptr(1, (void*)&st, sizeof(*st)) < 0)
    return -1;
  return filestat(f, st);
}

// Create the path new as a link to the same inode as old.
int
sys_link(void)
{
  char *new, *old;

  if(argstr(0, &old) < 0 || argstr(1, &new) < 0)
    return -1;

  return sfs_linki(old, new);
}

int
sys_unlink(void)
{
  char *path;

  if(argstr(0, &path) < 0)
    return -1;

  return sfs_unlinki(path);
}

int
sys_open(void)
{
  char *path;
  int fd, omode;
  fs_node_t *f;

  if(argstr(0, &path) < 0 || argint(1, &omode) < 0)
    return -1;

  if((f = fileopen(path, omode)) == 0)
    return -1;

  if((fd = fdalloc(f)) < 0){
    if(f)
      fileclose(f);
    //iunlockput(ip);
    return -1;
  }
  return fd;
}

int
sys_mkdir(void)
{
  char *path;

  if(argstr(0, &path) < 0){
    return -1;
  }

  return sfs_mkdiri(path);
}

int
sys_mknod(void)
{
  char *path;
  int major, minor;
  
  if(argstr(0, &path) < 0 ||
     argint(1, &major) < 0 ||
     argint(2, &minor) < 0) {
    return -1;
  }

  return sfs_mknodi(path, major, minor);
}

int
sys_chdir(void)
{
  //FIXME: . & ..
  char *path;
  char x_path[DIRSIZ];
  int len;

  if(argstr(0, &path) < 0)
    return -1;

  if(*path == '/') {
    safestrcpy(x_path, path, sizeof(x_path));
  } else {
    len = strlen(proc->cwd_path);
    safestrcpy(x_path, proc->cwd_path, sizeof(x_path));
    safestrcpy(x_path+len, path, sizeof(x_path)-len);
  }

  len = strlen(x_path);
  if(x_path[len - 1] != '/'){
    x_path[len] = '/';
    x_path[len+1] = 0;
  }

  //TODO: проверка реальности пути
  safestrcpy(proc->cwd_path, x_path, sizeof(proc->cwd_path));

  return 0;
}

int
sys_exec(void)
{
  char *path, *argv[MAXARG];
  int i;
  uint uargv, uarg;

  if(argstr(0, &path) < 0 || argint(1, (int*)&uargv) < 0){
    return -1;
  }
  memset(argv, 0, sizeof(argv));
  for(i=0;; i++){
    if(i >= NELEM(argv))
      return -1;
    if(fetchint(uargv+4*i, (int*)&uarg) < 0)
      return -1;
    if(uarg == 0){
      argv[i] = 0;
      break;
    }
    if(fetchstr(uarg, &argv[i]) < 0)
      return -1;
  }
  return exec(path, argv);
}

int
sys_pipe(void)
{
  int *fd;
  fs_node_t *rf, *wf;
  int fd0, fd1;

  if(argptr(0, (void*)&fd, 2*sizeof(fd[0])) < 0)
    return -1;
  if(pipealloc(&rf, &wf) < 0)
    return -1;
  fd0 = -1;
  if((fd0 = fdalloc(rf)) < 0 || (fd1 = fdalloc(wf)) < 0){
    if(fd0 >= 0)
      proc->ofile[fd0] = 0;
    fileclose(rf);
    fileclose(wf);
    return -1;
  }
  fd[0] = fd0;
  fd[1] = fd1;
  return 0;
}

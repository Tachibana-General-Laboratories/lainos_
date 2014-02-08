// On-disk file system format. 
// Both the kernel and user programs use this header file.

// Block 0 is unused.
// Block 1 is super block.
// Blocks 2 through sb.ninodes/IPB hold inodes.
// Then free bitmap blocks holding sb.size bits.
// Then sb.nblocks data blocks.
// Then sb.nlog log blocks.

#define ROOTINO 1  // root i-number
#define BSIZE 512  // block size

// File system super block
struct superblock {
  uint size;         // Size of file system image (blocks)
  uint nblocks;      // Number of data blocks
  uint ninodes;      // Number of inodes.
  uint nlog;         // Number of log blocks
};

#define DEV_offset 1

#define NDIRECT 12
#define NINDIRECT (BSIZE / sizeof(uint))
#define MAXFILE (NDIRECT + NINDIRECT)

// On-disk inode structure
struct dinode {
  short type;           // File type
  short major;          // Major device number (T_DEV only)
  short minor;          // Minor device number (T_DEV only)
  short nlink;          // Number of links to inode in file system
  uint size;            // Size of file (bytes)
  uint addrs[NDIRECT+1];   // Data block addresses
};

// in-memory copy of an inode
struct inode {
  uint dev;           // Device number
  uint inum;          // Inode number
  int ref;            // Reference count
  int flags;          // I_BUSY, I_VALID

  short type;         // copy of disk inode
  short major;
  short minor;
  short nlink;
  uint size;
  uint addrs[NDIRECT+1];
};
#define I_BUSY 0x1
#define I_VALID 0x2


// Inodes per block.
#define IPB           (BSIZE / sizeof(struct dinode))

// Block containing inode i
#define IBLOCK(i)     ((i) / IPB + 1 + DEV_offset)

// Bitmap bits per block
#define BPB           (BSIZE*8*32)

// Block containing bit for block b
#define BBLOCK(b, ninodes) (b/BPB + (ninodes)/IPB + 2 + DEV_offset)

// Directory is a file containing a sequence of dirent structures.
#define DIRSIZ 14

struct sfs_dirent {
  ushort inum;
  char name[DIRSIZ];
};

/*struct dirent {
  ushort inum;
  char name[DIRSIZ];
};*/

struct dirent
{
    uint32_t d_ino;           /* номер inode */
    uint32_t d_off;           /* смещение на dirent */
    uint16_t d_reclen;        /* длина d_name */
    //char d_name [NAME_MAX+1];   /* имя файла (оканчивающееся нулем) */
    char d_name [DIRSIZ+1];   /* имя файла (оканчивающееся нулем) */
};

// table mapping major device number to
// device functions
struct devsw {
  int (*read)(struct inode*, char*, uint, uint);
  int (*write)(struct inode*, char*, uint, uint);
};

extern struct devsw devsw[];

#define CONSOLE 1


// See COPYRIGHT for copyright information.

#ifndef INC_FS_H
#define INC_FS_H

#ifndef FS_FORMAT_TOOL
#include <types.h>
#include <mmu.h>
#endif

// File nodes (both in-memory and on-disk)

// Bytes per file system block - same as page size
#define BLKSIZE		PGSIZE
#define BLKBITSIZE	(BLKSIZE * 8)

// Maximum size of a filename (a single path component), including null
// Must be a multiple of 4
#define MAXNAMELEN	128

// Maximum size of a complete pathname, including null
#define MAXPATHLEN	1024

// Number of block pointers in a File descriptor
#define NDIRECT		10
// Number of direct block pointers in an indirect block
#define NINDIRECT	(BLKSIZE / 4)

#define MAXFILESIZE	((NDIRECT + NINDIRECT) * BLKSIZE)

struct File {
	char f_name[MAXNAMELEN];	// filename
	off_t f_size;			// file size in bytes
	uint32_t f_type;		// file type

	// Block pointers.
	// A block is allocated iff its value is != 0.
	uint32_t f_direct[NDIRECT];	// direct blocks
	uint32_t f_indirect;		// indirect block

	// Pad out to 256 bytes; must do arithmetic in case we're compiling
	// fsformat on a 64-bit machine.
	uint8_t f_pad[256 - MAXNAMELEN - 8 - 4*NDIRECT - 4];
} __attribute__((packed));	// required only on some 64-bit machines

// An inode block contains exactly BLKFILES 'struct File's
#define BLKFILES	(BLKSIZE / sizeof(struct File))

// File types
#define FTYPE_REG	0	// Regular file
#define FTYPE_DIR	1	// Directory


// File system super-block (both in-memory and on-disk)

#define FS_MAGIC	0x58594f54	// related vaguely to 'TOYX'

struct Super {
	uint32_t s_magic;		// Magic number: FS_MAGIC
	uint32_t s_nblocks;		// Total number of blocks on disk
	struct File s_root;		// Root directory node
};

// Definitions for requests from clients to file system
enum {
	FSREQ_OPEN = 1,
	FSREQ_SET_SIZE,
	// Read returns a Fsret_read on the request page
	FSREQ_READ,
	FSREQ_WRITE,
	// Stat returns a Fsret_stat on the request page
	FSREQ_STAT,
	FSREQ_FLUSH,
	FSREQ_REMOVE,
	FSREQ_SYNC,

	FSREQ_INFO,
	FSREQ_RENAME,
};

union Fsipc {
	struct Fsreq_open {
		char req_path[MAXPATHLEN];
		int req_omode;
	} open;
	struct Fsreq_set_size {
		int req_fileid;
		off_t req_size;
	} set_size;
	struct Fsreq_read {
		int req_fileid;
		size_t req_n;
	} read;
	struct Fsret_read {
		char ret_buf[PGSIZE];
	} readRet;
	struct Fsreq_write {
		int req_fileid;
		size_t req_n;
		char req_buf[PGSIZE - (sizeof(int) + sizeof(size_t))];
	} write;
	struct Fsreq_stat {
		int req_fileid;
	} stat;
	struct Fsret_stat {
		char ret_name[MAXNAMELEN];
		off_t ret_size;
		int ret_isdir;
	} statRet;
	struct Fsreq_flush {
		int req_fileid;
	} flush;
	struct Fsreq_remove {
		char req_path[MAXPATHLEN];
	} remove;
	struct Fsreq_info {
		uint32_t blk_num;
		uint32_t blk_ocp;
	} info;
	struct Fsreq_rename {
		char src_path[MAXPATHLEN];
		char dst_path[MAXPATHLEN];
	} rename;

	// Ensure Fsipc is one page
	char _pad[PGSIZE];
};

#endif /* !INC_FS_H */

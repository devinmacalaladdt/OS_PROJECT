/*
 *  Copyright (C) 2020 CS416 Rutgers CS
 *	Tiny File System
 *	File:	tfs.c
 *
 */

#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <math.h>
#include <errno.h>
#include <sys/time.h>
#include <libgen.h>
#include <limits.h>

#include "block.h"
#include "tfs.h"

char diskfile_path[PATH_MAX];

// Declare your in-memory data structures here
superblock * super_block = NULL;

/*
 * Get available inode number from bitmap
 */
int get_avail_ino() {
	return get_avail(1);
	// Step 1: Read inode bitmap from disk

	// Step 2: Traverse inode bitmap to find an available slot

	// Step 3: Update inode bitmap and write to disk 

}

/* 
 * Get available data block number from bitmap
 */
int get_avail_blkno() {
	return get_avail(0);
	// Step 1: Read data block bitmap from disk
	
	// Step 2: Traverse data block bitmap to find an available slot

	// Step 3: Update data block bitmap and write to disk 

}

int get_avail(int bitmap_type) {
	if (super_block == NULL)
		return -1;
	if(bitmap_type)
		uint32_t block = super_block->i_bitmap_blk;
	else
		uint32_t block = super_block->d_bitmap_blk;
	unsigned char bitmap[BLOCK_SIZE];
	int chk = bio_read(block, bitmap);
	if (chk < 0)
		return -1;
	int x, y;
	for (x = 0; x < BLOCK_SIZE; x++) {
		if (bitmap[x] == 0b11111111)
			continue;
		for (y = 0; y < 8; y++) {
			if (!((bitmap[x] >> y) & 1)) {
				bitmap[x] &= !((bitmap[x] >> y) & 1);
				chk = bio_write(block, bitmap);
				if (chk < 0)
					return -1;
				return (x * 8 + y);
			}
		}
	}
	return -1;
}

/* 
 * inode operations
 */
int readi(uint16_t ino, inode *_inode) {


  	unsigned char buf[BLOCK_SIZE];

  // Step 1: Get the inode's on-disk block number

  	uint16_t blk_num = ino/(BLOCK_SIZE/sizeof(inode));

  // Step 2: Get offset of the inode in the inode on-disk block

  	uint16_t inner_ino = ino%(BLOCK_SIZE/sizeof(inode));

  // Step 3: Read the block from disk and then copy into inode structure

	bio_read(super_block->i_start_blk+blk_num,buf);
	memcpy(_inode, buf+(inner_ino*sizeof(inode)),sizeof(inode));

	return 0;
}

int writei(uint16_t ino, inode *_inode) {

	unsigned char buf[BLOCK_SIZE];

	// Step 1: Get the block number where this inode resides on disk

	uint16_t blk_num = ino/(BLOCK_SIZE/sizeof(inode));
	
	// Step 2: Get the offset in the block where this inode resides on disk

	uint16_t inner_ino = ino%(BLOCK_SIZE/sizeof(inode));

	// Step 3: Write inode to disk 

	bio_read(super_block->i_start_blk+blk_num,buf);
	memcpy(buf+(inner_ino*sizeof(inode)), _inode,sizeof(inode));
	bio_write(super_block->i_start_blk+blk_num,buf);

	return 0;
}


/* 
 * directory operations
 */
int dir_find(uint16_t ino, const char *fname, size_t name_len, dirent *_dirent) {

  // Step 1: Call readi() to get the inode using ino (inode number of current directory)

	inode i;
	readi(ino,&i);
	int num_dirents = i.size/(sizeof(dirent));

  // Step 2: Get data block of current directory from inode

	int b = 0;//current block index in direct_ptr
	int d = 0;//current total dirent within enitre directory

	for(b=0;b<16;b++){

		if(i.direct_ptr[b]==0){continue;}
		unsigned char buf[BLOCK_SIZE];
		bio_read(i.direct_ptr[b],buf);
		int curr_dirent = 0;//current dirent within block
		while(curr_dirent<(BLOCK_SIZE/sizeof(dirent)) && d<num_dirents){

			dirent dir;
			memcpy(&dir,buf+curr_dirent*(sizeof(dirent)),sizeof(dirent));
			if(dir.valid==1){

				if(strcmp(dir.name,fname)==0){
					
					memcpy(_dirent,&dir,sizeof(dirent));
					return 0;

				}

				d++;

			}
			curr_dirent++;

		}

	}

	return -1;

  // Step 3: Read directory's data block and check each directory entry.
  //If the name matches, then copy directory entry to dirent structure
}

int dir_add(inode dir_inode, uint16_t f_ino, const char *fname, size_t name_len) {
	if (name_len > 207)
		return -1;
	// Step 1: Read dir_inode's data block and check each directory entry of dir_inode
	unsigned char block[BLOCK_SIZE];
	int x, y;
	for (x = 0; x < 16; x++) {
		if (dir_inode.direct_ptr[x] < 1) continue;
		int chk = bio_read(dir_inode.direct_ptr[x], block);
		if(!(chk < 0))
			for (y = 0; y < BLOCK_SIZE / sizeof(dirent); y++) { //16
				dirent *dir = (dirent*)malloc(sizeof(dirent));
				memcpy(dir, (block + x * sizeof(dirent)), sizeof(dirent));
				if (strcmp(dir->name, fname) == 0 && dir->valid == 1) {
					free(dir);
					return -1; //already exists
				}
				free(dir);
			}
	}

	/*precondition: validated that it doesn't already exist above*/
	for (x = 0; x < 16; x++) {
		if (dir_inode.direct_ptr[x] < 1) {
			dir_inode.direct_ptr[x] = get_avail_blkno(); //new block
			dir_inode.vstat.st_blksize++;
		}
		int chk = bio_read(dir_inode.direct_ptr[x], block);
		if (!(chk < 0))
			for (y = 0; y < BLOCK_SIZE / sizeof(dirent); y++) { //16
				dirent *dir = (dirent*)malloc(sizeof(dirent));
				memcpy(dir, (block + x * sizeof(dirent)), sizeof(dirent));
				if (dir->valid == 0) { //invalid Or unused
					memcpy(dir->name, fname, name_len);
					dir->valid = 1;
					dir->ino = f_ino;
					dir->len = name_len;
					/*write new/updated dirent block to disk*/
					memcpy((block + x * sizeof(dirent)), dir, sizeof(dirent));
					chk = bio_write(dir_inode.direct_ptr[x], block);
					free(dir);
					if (chk < 0) return -1;
					/*write updated dir_inode*/
					dir_inode.vstat.st_size += sizeof(dirent);
					dir_inode.size += sizeof(dirent);
					chk = writei(dir_inode.ino, &dir_inode);
					if (chk < 0) return -1;
					return 1;
				}
			}
	}
	// Step 2: Check if fname (directory name) is already used in other entries

	// Step 3: Add directory entry in dir_inode's data block and write to disk

	// Allocate a new data block for this directory if it does not exist

	// Update directory inode

	// Write directory entry

	return -1;
}

int dir_remove(inode dir_inode, const char *fname, size_t name_len) {
	if (name_len > 207)
		return -1;
	unsigned char block[BLOCK_SIZE];
	int x, y;
	for (x = 0; x < 16; x++) {
		if (dir_inode.direct_ptr[x] < 1) continue;
		int chk = bio_read(dir_inode.direct_ptr[x], block);
		if (!(chk < 0))
			for (y = 0; y < BLOCK_SIZE / sizeof(dirent); y++) { //16
				dirent *dir = (dirent*)malloc(sizeof(dirent));
				memcpy(dir, (block + x * sizeof(dirent)), sizeof(dirent));
				if (strcmp(dir->name, fname) == 0 && dir->valid == 1) {
					inode *i = (inode*)malloc(sizeof(inode));
					readi(dir->ino, i);
					int z;
					for (z = 0; z < 16; z++)
						if (i->direct_ptr[z] != 0)
							return -2; //cannot delete dir with files in it
					dir->valid = 0;
					memcpy((block + x * sizeof(dirent)), dir, sizeof(dirent));
					chk = bio_write(dir_inode.direct_ptr[x], block);
					free(i);
					if (chk < 0) return -1;
					dir_inode.vstat.st_size -= sizeof(dirent);
					dir_inode.size -= sizeof(dirent);
					writei(dir_inode.ino, &dir_inode);
					return 1;
				}
				free(dir);
			}
	}
	// Step 1: Read dir_inode's data block and checks each directory entry of dir_inode
	
	// Step 2: Check if fname exist

	// Step 3: If exist, then remove it from dir_inode's data block and write to disk

	return -1;
}

/* 
 * namei operation
 */
int get_node_by_path(const char *path, uint16_t ino, inode *_inode) {
	
	// Step 1: Resolve the path name, walk through path, and finally, find its inode.
	if(strcmp(path,"")=="/"){

		readi(0,_inode);
		return 0;

	}
	char p[208];
	strcpy(p,path);
	char * tok = strtok(p,"/");
	dirent d;

	while(tok!=NULL){

		int res = dir_find(ino,tok,strlen(tok),&d);
		if(res!=0){printf("Invalid Path\n");return -1;}
		ino = d.ino;

	}

	readi(ino,_inode);


	// Note: You could either implement it in a iterative way or recursive way

	return 0;
}

/* 
 * Make file system
 */
int tfs_mkfs() {

	// Call dev_init() to initialize (Create) Diskfile
	dev_init("a.txt");

	// write superblock information
	super_block = (superblock*)(malloc(sizeof(superblock)));
	super_block->magic_num = MAGIC_NUM;
	super_block->max_inum = MAX_INUM;
	super_block->max_dnum = MAX_DNUM;
	super_block->i_bitmap_blk = 1;
	super_block->d_bitmap_blk = 2;
	super_block->i_start_blk = 3;
	super_block->d_start_blk = 3+MAX_INUM/(BLOCK_SIZE/(sizeof(inode)));

	unsigned char buf[BLOCK_SIZE];
	memcpy(buf,super_block,sizeof(superblock));
	bio_write(0,super_block);

	// initialize inode bitmap
	unsigned char buf1[BLOCK_SIZE];
	memset(buf1,0,BLOCK_SIZE);
	


	// initialize data block bitmap
	unsigned char buf2[BLOCK_SIZE];
	memset(buf2,0,BLOCK_SIZE);

	// update bitmap information for root directory
	set_bitmap(buf1,0);
	set_bitmap(buf2,0);


	bio_write(super_block->i_bitmap_blk,buf1);
	bio_write(super_block->d_bitmap_blk,buf2);

	// update inode for root directory
	inode root;
	root.ino=0;
	root.vstat.st_ino=0;
	root.valid=1;
	root.size = 0;
	root.vstat.st_size = 0;
	root.type = S_IFDIR;
	root.vstat.st_mode = S_IFDIR;
	root.link=0;
	root.vstat.st_nlink = 0;
	root.vstat.st_blksize = BLOCK_SIZE;
	root.vstat.blocks = 0;
	root.vstat.st_gid = getgid();
	root.vstat.st_uid = getuid();
	time(&root.vstat.st_atime);
	time(&root.vstat.st_ctime);
	time(&root.vstat.st_mtime);


	int i = 0;
	for(i=0;i<16;i++){

		root.direct_ptr[i]=0;

	}

	root.direct_ptr[0]=super_block->d_start_blk;
	unsigned char buf3[BLOCK_SIZE];
	int c = 0;
	for(c=0;c<(BLOCK_SIZE/sizeof(dirent));c++){

		dirent d;
		d.valid = 0;
		memcpy(buf3+(c*sizeof(dirent)),&d,sizeof(dirent));

	}

	bio_write(super_block->d_start_blk,buf3);

	memset(buf1,0,BLOCK_SIZE);
	memcpy(buf1,&root,sizeof(inode));

	bio_write(super_block->i_start_blk,buf1);

	return 0;
}


/* 
 * FUSE file operations
 */
static void *tfs_init(struct fuse_conn_info *conn) {

	// Step 1a: If disk file is not found, call mkfs
	if(dev_open("a.txt")==-1){tfs_mkfs(); return NULL;}

  // Step 1b: If disk file is found, just initialize in-memory data structures
  // and read superblock from disk
  	super_block = (super_block*)(malloc(sizeof(superblock)));
	unsigned char buf[BLOCK_SIZE];
	bio_read(0,buf);
	memcpy(super_block,buf,sizeof(superblock));

	return NULL;
}

static void tfs_destroy(void *userdata) {

	// Step 1: De-allocate in-memory data structures
	free(super_block);

	// Step 2: Close diskfile
	dev_close();

}

static int tfs_getattr(const char *path, struct stat *stbuf) {

	// Step 1: call get_node_by_path() to get inode from path

	inode i;
	int res = get_node_by_path(path,0,&i);
	if(res==-1){return -1;}

	// Step 2: fill attribute of file into stbuf from inode

	stbuf->st_ino = i.vstat.st_ino;
	stbuf->st_size = i.vstat.st_size;
	stbuf->st_mode = i.vstat.st_mode;
	stbuf->st_nlink = i.vstat.st_nlink;
	stbuf->st_blksize = i.vstat.st_blksize;
	stbuf->st_blocks = i.vstat.blocks;
	stbuf->st_gid = getgid();
	stbuf->st_uid = getuid();
	time(&stbuf->st_atime);
	time(&stbuf->st_mtime);
	time(&stbuf->st_ctime);

	return 0;
}

static int tfs_opendir(const char *path, struct fuse_file_info *fi) {

	// Step 1: Call get_node_by_path() to get inode from path
	inode i;
	int res = get_node_by_path(path,0,&i);
	if(res==-1){return -1;}
	if(i.type!=S_IFDIR){return -1;}
	// Step 2: If not find, return -1

    return 0;
}

static int tfs_readdir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {

	// Step 1: Call get_node_by_path() to get inode from path
	inode i;
	int res = get_node_by_path(path,0,&i);
	if(res==-1){return -1;}
	if(i.type!=S_IFDIR){return -1;}

	// Step 2: Read directory entries from its data blocks, and copy them to filler
	int b = 0;//current block index in direct_ptr
	int d = 0;//current total dirent within enitre directory
	int num_dirents = i.size/(sizeof(dirent));

	for(b=0;b<16;b++){

		if(i.direct_ptr[b]==0){continue;}
		unsigned char buf[BLOCK_SIZE];
		bio_read(i.direct_ptr[b],buf);
		int curr_dirent = 0;//current dirent within block
		while(curr_dirent<(BLOCK_SIZE/sizeof(dirent)) && d<num_dirents){

			dirent dir;
			memcpy(&dir,buf+curr_dirent*(sizeof(dirent)),sizeof(dirent));

			if(dir.valid==1){

				filler(buffer,dir.name,NULL,0);
				d++;

			}
			curr_dirent++;

		}

	}

	return 0;
}


static int tfs_mkdir(const char *path, mode_t mode) {

	// Step 1: Use dirname() and basename() to separate parent directory path and target directory name
	char * dirPath = dirname(path); 
	char * fileName = basename(path);

	// Step 2: Call get_node_by_path() to get inode of parent directory
	inode parent;
	int res = get_node_by_path(path,0,&parent);
	if(res==-1){return -1;}
	if(parent.type!=S_IFDIR){return -1;}

	// Step 3: Call get_avail_ino() to get an available inode number
	int next_avail = get_avail(1);

	// Step 4: Call dir_add() to add directory entry of target directory to parent directory
	res = dir_add(parent,next_avail,fileName,strlen(fileName));
	if(res==-1){return -1;}

	// Step 5: Update inode for target directory
	inode i;
	i.ino=next_avail;
	i.vstat.st_ino=next_avail;
	i.valid=1;
	i.size = 0;
	i.vstat.st_size = 0;
	i.type = S_IFDIR;
	i.vstat.st_mode = S_IFDIR;
	i.link=1;
	i.vstat.st_nlink = 1;
	i.vstat.st_blksize = BLOCK_SIZE;
	i.vstat.blocks = 0;
	i.vstat.st_gid = getgid();
	i.vstat.st_uid = getuid();
	time(&i.vstat.st_atime);
	time(&i.vstat.st_ctime);
	time(&i.vstat.st_mtime);


	int i = 0;
	for(i=0;i<16;i++){

		i.direct_ptr[i]=0;

	}

// allocate new block to hold dirents, set all to invalid initially
	int dir_next_avail = get_avail(0);
	i.direct_ptr[0]=super_block->d_start_blk+dir_next_avail;
	unsigned char buf[BLOCK_SIZE];
	int c = 0;
	for(c=0;c<(BLOCK_SIZE/sizeof(dirent));c++){

		dirent d;
		d.valid = 0;
		memcpy(buf+(c*sizeof(dirent)),&d,sizeof(dirent));

	}

	bio_write(super_block->d_start_blk+dir_next_avail,buf);
	

	// Step 6: Call writei() to write inode to disk
	writei(next_avail,&i);
	

	return 0;
}

static int tfs_rmdir(const char *path) {

	// Step 1: Use dirname() and basename() to separate parent directory path and target directory name

	// Step 2: Call get_node_by_path() to get inode of target directory

	// Step 3: Clear data block bitmap of target directory

	// Step 4: Clear inode bitmap and its data block

	// Step 5: Call get_node_by_path() to get inode of parent directory

	// Step 6: Call dir_remove() to remove directory entry of target directory in its parent directory

	return 0;
}

static int tfs_releasedir(const char *path, struct fuse_file_info *fi) {
	// For this project, you don't need to fill this function
	// But DO NOT DELETE IT!
    return 0;
}

static int tfs_create(const char *path, mode_t mode, struct fuse_file_info *fi) {

	// Step 1: Use dirname() and basename() to separate parent directory path and target file name
	char * dirPath = dirname(path); 
	char * fileName = basename(path);

	// Step 2: Call get_node_by_path() to get inode of parent directory
	inode *pd = (inode*)malloc(sizeof(inode));
	int ino = get_node_by_path(dirPath, 0, pd);

	// Step 3: Call get_avail_ino() to get an available inode number
	int availino = get_avail_ino();
	
	// Step 4: Call dir_add() to add directory entry of target file to parent directory
	if (dir_add(*pd, availino, fileName, strlen(fileName)) < 0)
		return -1;

	// Step 5: Update inode for target file
	

	// Step 6: Call writei() to write inode to disk

	return 0;
}

static int tfs_open(const char *path, struct fuse_file_info *fi) {

	// Step 1: Call get_node_by_path() to get inode from path

	// Step 2: If not find, return -1

	return 0;
}

static int tfs_read(const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi) {

	// Step 1: You could call get_node_by_path() to get inode from path

	// Step 2: Based on size and offset, read its data blocks from disk

	// Step 3: copy the correct amount of data from offset to buffer

	// Note: this function should return the amount of bytes you copied to buffer
	return 0;
}

static int tfs_write(const char *path, const char *buffer, size_t size, off_t offset, struct fuse_file_info *fi) {
	// Step 1: You could call get_node_by_path() to get inode from path

	// Step 2: Based on size and offset, read its data blocks from disk

	// Step 3: Write the correct amount of data from offset to disk

	// Step 4: Update the inode info and write it to disk

	// Note: this function should return the amount of bytes you write to disk
	return size;
}

static int tfs_unlink(const char *path) {

	// Step 1: Use dirname() and basename() to separate parent directory path and target file name

	// Step 2: Call get_node_by_path() to get inode of target file

	// Step 3: Clear data block bitmap of target file

	// Step 4: Clear inode bitmap and its data block

	// Step 5: Call get_node_by_path() to get inode of parent directory

	// Step 6: Call dir_remove() to remove directory entry of target file in its parent directory

	return 0;
}

static int tfs_truncate(const char *path, off_t size) {
	// For this project, you don't need to fill this function
	// But DO NOT DELETE IT!
    return 0;
}

static int tfs_release(const char *path, struct fuse_file_info *fi) {
	// For this project, you don't need to fill this function
	// But DO NOT DELETE IT!
	return 0;
}

static int tfs_flush(const char * path, struct fuse_file_info * fi) {
	// For this project, you don't need to fill this function
	// But DO NOT DELETE IT!
    return 0;
}

static int tfs_utimens(const char *path, const struct timespec tv[2]) {
	// For this project, you don't need to fill this function
	// But DO NOT DELETE IT!
    return 0;
}

char* dirname(char* p) {
	char* path = (char*)malloc(strlen(p) + 1);
	strcpy(path, p);
	int x;
	for (x = strlen(path); x > 0; x--) {
		if (*(path + x - 1) == '/') {
			*(path + x - 1) = '\0';
			return path;
		}
	}
	return -1;
}

char* basename(char* p) {
	char* path = (char*)malloc(strlen(p) + 1);
	strcpy(path, p);
	int x;
	for (x = strlen(path); x > 0; x--) {
		if (*(path + x - 1) == '/') {
			return (path + x);
		}
	}
	return -1;
}

static struct fuse_operations tfs_ope = {
	.init		= tfs_init,
	.destroy	= tfs_destroy,

	.getattr	= tfs_getattr,
	.readdir	= tfs_readdir,
	.opendir	= tfs_opendir,
	.releasedir	= tfs_releasedir,
	.mkdir		= tfs_mkdir,
	.rmdir		= tfs_rmdir,

	.create		= tfs_create,
	.open		= tfs_open,
	.read 		= tfs_read,
	.write		= tfs_write,
	.unlink		= tfs_unlink,

	.truncate   = tfs_truncate,
	.flush      = tfs_flush,
	.utimens    = tfs_utimens,
	.release	= tfs_release
};


int main(int argc, char *argv[]) {
	int fuse_stat;

	getcwd(diskfile_path, PATH_MAX);
	strcat(diskfile_path, "/DISKFILE");

	fuse_stat = fuse_main(argc, argv, &tfs_ope, NULL);

	return fuse_stat;
}


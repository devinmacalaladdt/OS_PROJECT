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
int get_avail(int);
char* dirName(char*);
char* baseName(char*);

/*
 * Get available inode number from bitmap
 -1 on error
 */
int get_avail_ino() {
	return get_avail(1);
	// Step 1: Read inode bitmap from disk

	// Step 2: Traverse inode bitmap to find an available slot

	// Step 3: Update inode bitmap and write to disk 

}

/* 
 * Get available data block number from bitmap
 -1 on error
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
	uint32_t block;
	if(bitmap_type)
		block = super_block->i_bitmap_blk;
	else
		block = super_block->d_bitmap_blk;
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
				//printf("BITMAP: %x %x %x %x\n", bitmap[x], (bitmap[x] >> y) & 1, !((bitmap[x] >> y) & 1), bitmap[x] | !((bitmap[x] >> y) & 1));
				bitmap[x] |= (1 << y);
				chk = bio_write(block, bitmap);
				if (chk < 0)
					return -1;
				if(bitmap_type)
					return (x * 8 + y) + super_block->i_start_blk;
				return (x * 8 + y) + super_block->d_start_blk;
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
	//printf("DIR_FIND CALLED WITH: %s %d %d\n", fname, (int)(name_len),(int)(ino));
  // Step 1: Call readi() to get the inode using ino (inode number of current directory)

	inode i;
	readi(ino,&i);
	//int num_dirents = i.size/(sizeof(dirent));

  // Step 2: Get data block of current directory from inode

	int b = 0;//current block index in direct_ptr
	//int d = 0;//current total dirent within enitre directory

	for(b=0;b<16;b++){
		//printf("ptr: %d\n", (int)(i.direct_ptr[b]));
		if(i.direct_ptr[b]==0) continue;
		unsigned char buf[BLOCK_SIZE];
		bio_read(i.direct_ptr[b],buf);
		int y;//current dirent within block
		for (y = 0; y < BLOCK_SIZE / sizeof(dirent); y++) {

			dirent *dir = (dirent*)malloc(sizeof(dirent));
			memcpy(dir, (buf + y * sizeof(dirent)), sizeof(dirent));
			//printf("INFO: %s : %s\n", dir->name, fname);
			if (dir->valid) {
				//printf("STRCMP FIND: %s : %s But INODE: %d\n", dir->name, fname, (int)(dir->ino));
				//printf("STRCMP: %s : %s\n", dir->name, fname);
				if (strcmp(dir->name, fname) == 0) {

					memcpy(_dirent, dir, sizeof(dirent));
					free(dir);
					return 0;

				}
			}
			else {
				if (strcmp(dir->name, fname) == 0) {

					printf("TARGET FOUND: valid bit off\n");

				}
			}
			free(dir);
		}

	}

	return -1;

  // Step 3: Read directory's data block and check each directory entry.
  //If the name matches, then copy directory entry to dirent structure
}

int dir_add(inode dir_inode, uint16_t f_ino, const char *fname, size_t name_len) {
	if (name_len > 207)
		return -EPERM;
	// Step 1: Read dir_inode's data block and check each directory entry of dir_inode
	unsigned char block[BLOCK_SIZE];
	int x, y;
	for (x = 0; x < 16; x++) {
		if (dir_inode.direct_ptr[x] < 1) continue;
		int chk = bio_read(dir_inode.direct_ptr[x], block);
		if(!(chk < 0))
			for (y = 0; y < BLOCK_SIZE / sizeof(dirent); y++) { //16
				dirent *dir = (dirent*)malloc(sizeof(dirent));
				memcpy(dir, (block + y * sizeof(dirent)), sizeof(dirent));
				if (strcmp(dir->name, fname) == 0 && dir->valid == 1) {
					free(dir);
					//printf("DIR_ADD - %s : %s\n", dir->name, fname);
					return -1; //already exists
				}
				free(dir);
			}
	}
	int chk;
	/*precondition: validated that it doesn't already exist above*/
	for (x = 0; x < 16; x++) {
		if (dir_inode.direct_ptr[x] < 1) {
			dir_inode.direct_ptr[x] = get_avail_blkno(); //new block
			dir_inode.vstat.st_blocks++;
			/*ensure it's 0*/
			memset(block, 0, BLOCK_SIZE);
			chk = bio_write(dir_inode.direct_ptr[x], block);
			if (chk < 0)
				return -1;
		}
		chk = bio_read(dir_inode.direct_ptr[x], block);
		if (!(chk < 0))
			for (y = 0; y < BLOCK_SIZE / sizeof(dirent); y++) { //16
				dirent *dir = (dirent*)malloc(sizeof(dirent));
				memcpy(dir, (block + y * sizeof(dirent)), sizeof(dirent));
				if (dir->valid == 0) { //invalid Or unused
					strcpy(dir->name, fname);
					dir->valid = 1;
					dir->ino = f_ino;
					dir->len = name_len;
					//printf("DIR_ADD ADDED: %s %d\n", dir->name, (int)(dir->ino));
					/*write new/updated dirent block to disk*/
					memcpy((block + y * sizeof(dirent)), dir, sizeof(dirent));
					chk = bio_write(dir_inode.direct_ptr[x], block);
					free(dir);
					if (chk < 0) return -1;

					/*write updated dir_inode*/
					dir_inode.vstat.st_size += sizeof(dirent);
					dir_inode.size += sizeof(dirent);
					dir_inode.vstat.st_nlink++;
					dir_inode.link++;
					chk = writei(dir_inode.ino, &dir_inode);
					if (chk < 0) return -1;
					/*printf("READ: %d\n", (int)(dir_inode.ino));
					readi(dir_inode.ino, &dir_inode);
					printf("::%d\n", dir_inode.direct_ptr[0]);*/
					//test
					/*dir = (dirent*)malloc(sizeof(dirent));
					readi(dir_inode.ino, &dir_inode);
					bio_read(dir_inode.direct_ptr[0], block);
					printf("READ INO: %d, READ PTR: %d\n", dir_inode.ino, dir_inode.direct_ptr[0]);
					memcpy(dir, (block + 0 * sizeof(dirent)), sizeof(dirent));
					printf("Y: %d\n", y);
					printf("DIRENT: ->name: %s  ->ino: %d  ->val: %d  ->len: %d  \n", dir->name, (int)(dir->ino), (int)(dir->valid), (int)(dir->len));
					free(dir);
					dirent d;
					int a = dir_find(dir_inode.ino, fname, name_len, &d);
					printf("%d\n", a);*/
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
		return -EPERM;
	unsigned char block[BLOCK_SIZE];
	int x, y;
	//printf("a\n");
	for (x = 0; x < 16; x++) {
		if (dir_inode.direct_ptr[x] < 1) continue;
		int chk = bio_read(dir_inode.direct_ptr[x], block);
		if (!(chk < 0))
			for (y = 0; y < BLOCK_SIZE / sizeof(dirent); y++) { //16
				dirent *dir = (dirent*)malloc(sizeof(dirent));
				memcpy(dir, (block + y * sizeof(dirent)), sizeof(dirent));
				//printf("b\n");
				if (strcmp(dir->name, fname) == 0 && dir->valid == 1) {
					//printf("c\n");
					inode *i = (inode*)malloc(sizeof(inode));
					readi(dir->ino, i);
					/*int z;
					for (z = 0; z < 16; z++)
						if (i->direct_ptr[z] != 0) {
							return -11; //cannot delete dir with files in it
						}*/
					dir->valid = 0;
					//printf("Invalidated...\n");
					memcpy((block + y * sizeof(dirent)), dir, sizeof(dirent));
					chk = bio_write(dir_inode.direct_ptr[x], block);
					free(i);
					//printf("Written erase::\n");
					if (chk < 0) return -1;
					dir_inode.vstat.st_size -= sizeof(dirent);
					dir_inode.size -= sizeof(dirent);
					dir_inode.vstat.st_nlink--;
					dir_inode.link--;
					//dir_inode.direct_ptr[x] = 0;
					writei(dir_inode.ino, &dir_inode);
					return 1;
				}
				free(dir);
			}
	}
	// Step 1: Read dir_inode's data block and checks each directory entry of dir_inode
	
	// Step 2: Check if fname exist

	// Step 3: If exist, then remove it from dir_inode's data block and write to disk
	//printf("d\n");
	return -1;
}

/* 
 * namei operation
 */
int get_node_by_path(const char *path, uint16_t ino, inode *_inode) {
	// Step 1: Resolve the path name, walk through path, and finally, find its inode.
	if(strcmp(path,"")==0){

		readi(0,_inode);
		return 0;

	}
	char p[208];
	strcpy(p,path);
	char * tok = strtok(p,"/");
	dirent d;

	//printf("TOK 1: %s\n", tok);

	while(tok!=NULL){

		if(dir_find(ino, tok, strlen(tok)+1, &d) != 0){
			//printf("CRITICAL namei: %s\n", tok);
//			printf("Invalid Path\n");
			return -1;
		}
		ino = d.ino;
		//printf("WARNING namei: %s\n", tok);

		tok = strtok(NULL, "/");

	}

	readi(ino,_inode);


	// Note: You could either implement it in a iterative way or recursive way

	return 0;
}

/* 
 * Make file system
 */
int tfs_mkfs() {
	printf("MAKE FS\n");
	// Call dev_init() to initialize (Create) Diskfile
	dev_init(diskfile_path);

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
	root.vstat.st_mode = S_IFDIR | 0755;
	root.link=2;
	root.vstat.st_nlink = 2;
	root.vstat.st_blksize = BLOCK_SIZE;
	root.vstat.st_blocks = 0;
	root.vstat.st_gid = getgid();
	root.vstat.st_uid = getuid();
	time(&(root.vstat.st_atime));
	time(&(root.vstat.st_ctime));
	time(&(root.vstat.st_mtime));


	int i = 0;
	for(i=0;i<16;i++){

		root.direct_ptr[i]=0;

	}

	//initial block for root
	root.direct_ptr[0]=super_block->d_start_blk;
	unsigned char buf3[BLOCK_SIZE];
	memset(buf3,0,BLOCK_SIZE);
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
	printf("INIT\n");
	// Step 1a: If disk file is not found, call mkfs
	if(dev_open(diskfile_path)==-1){tfs_mkfs(); return NULL;}

  // Step 1b: If disk file is found, just initialize in-memory data structures
  // and read superblock from disk
  	super_block = (superblock*)(malloc(sizeof(superblock)));
	unsigned char buf[BLOCK_SIZE];
	bio_read(0,buf);
	memcpy(super_block,buf,sizeof(superblock));
	return NULL;
}

static void tfs_destroy(void *userdata) {
	printf("DESTROY FS\n");
	// Step 1: De-allocate in-memory data structures
	free(super_block);

	// Step 2: Close diskfile
	dev_close();

}

static int tfs_getattr(const char *path, struct stat *stbuf) {
	printf("GETATTR\n");
	// Step 1: call get_node_by_path() to get inode from path

	inode i;
	
	//printf("GNBP CALLED WITH: path: %s\n", path);
	if(get_node_by_path(path, 0, &i)==-1){
		//memset(stbuf, 0, sizeof(struct stat));
		return -ENOENT;
	}

	// Step 2: fill attribute of file into stbuf from inode

	memcpy(stbuf, &(i.vstat), sizeof(struct stat));

	/*stbuf->st_ino = i.vstat.st_ino;
	stbuf->st_size = i.vstat.st_size;
	stbuf->st_mode = i.vstat.st_mode;
	stbuf->st_nlink = i.vstat.st_nlink;
	stbuf->st_blksize = i.vstat.st_blksize;
	stbuf->st_blocks = i.vstat.st_blocks;
	stbuf->st_uid = i.vstat.st_uid;
	stbuf->st_gid = i.vstat.st_gid;
	stbuf->st_atime = i.vstat.st_atime;
	stbuf->st_ctime = i.vstat.st_ctime;
	stbuf->st_mtime = i.vstat.st_mtime;*/
	/*stbuf->st_gid = getgid();
	stbuf->st_uid = getuid();
	time(&(stbuf->st_atime));
	time(&(stbuf->st_mtime));
	time(&(stbuf->st_ctime));*/

	return 0;
}

static int tfs_opendir(const char *path, struct fuse_file_info *fi) {
	printf("OPEN DIR\n");
	// Step 1: Call get_node_by_path() to get inode from path
	inode i;
	int res = get_node_by_path(path,0,&i);
	if(res==-1){return -1;}
	if(i.type!=S_IFDIR){return -1;}
	// Step 2: If not find, return -1

    return 0;
}

static int tfs_readdir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
	printf("READ DIR\n");
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
	printf("MAKE DIR\n");
	// Step 1: Use dirname() and basename() to separate parent directory path and target directory name
	char* _path = (char*)malloc(strlen(path) + 1);
	strcpy(_path, path);
	char * dirPath = dirName(_path); 
	char * fileName = baseName(_path);
	//printf("PATH: %s, _PATH: %s, DIR: %s, BASE: %s\n", path, _path, dirPath, fileName);

	// Step 2: Call get_node_by_path() to get inode of parent directory
	inode parent;
	if(get_node_by_path(dirPath, 0, &parent) ==-1){
		return -ENOENT;
	}
	if(parent.type!=S_IFDIR){
		return -EPERM;
	}

	// Step 3: Call get_avail_ino() to get an available inode number
	int next_avail = get_avail_ino();
	//printf("NEXT_AVAIL: %d\n", next_avail);

	// Step 4: Call dir_add() to add directory entry of target directory to parent directory
	//printf("CALL DIR_ADD - %s\n", path);
	if(dir_add(parent, next_avail, fileName, strlen(path))==-1){
		return -1;
	}

	// Step 5: Update inode for target directory
	inode i;
	i.ino=next_avail;
	i.vstat.st_ino=next_avail;
	i.valid=1;
	i.size = sizeof(dirent);
	i.vstat.st_size = sizeof(dirent);
	i.type = S_IFDIR;
	i.vstat.st_mode = S_IFDIR | mode;
	i.link=2;
	i.vstat.st_nlink = 2;
	i.vstat.st_blksize = BLOCK_SIZE;
	i.vstat.st_blocks = 0;
	i.vstat.st_gid = getgid();
	i.vstat.st_uid = getuid();
	time(&(i.vstat.st_atime));
	time(&(i.vstat.st_ctime));
	time(&(i.vstat.st_mtime));


	int c;
	for(c=1;c<16;c++)
		i.direct_ptr[c]=0;

	//initial block for new directory
	//int dir_next_avail = get_avail_blkno();
	//i.direct_ptr[0]=dir_next_avail;
	//unsigned char buf[BLOCK_SIZE];

	//bio_write(dir_next_avail,buf);
	

	// Step 6: Call writei() to write inode to disk
	writei(next_avail,&i);
	
	/*readi(0, &parent);
	char block[BLOCK_SIZE];
	dirent *dir = (dirent*)malloc(sizeof(dirent));
	bio_read(parent.direct_ptr[0], block);
	memcpy(dir, (block + 0 * sizeof(dirent)), sizeof(dirent));
	printf("DIRENT: ->name: %s  ->ino: %d  ->val: %d  ->len: %d  \n", dir->name, (int)(dir->ino), (int)(dir->valid), (int)(dir->len));
	free(dir);
	dirent d;
	int a = dir_find(parent.ino, fileName, strlen(fileName) + 1, &d);
	printf("END: %d\n", a);*/

	return 0;
}

static int tfs_rmdir(const char *path) {
	printf("REMOVE DIR\n");
	// Step 1: Use dirname() and basename() to separate parent directory path and target directory name
	char _path[strlen(path)];
	strcpy(_path,path);
	char * dirPath = dirName(_path); 
	char * fileName = baseName(_path);

	// Step 2: Call get_node_by_path() to get inode of parent directory
	inode parent;
	if(get_node_by_path(dirPath, 0, &parent)==-1)
		return -ENOENT;
	if(parent.type!=S_IFDIR)
		return -EPERM;
	// Step 3: Clear data block bitmap of target directory
	inode child;

	if (get_node_by_path(path, 0, &child) == -1)
		return -ENOENT;
	if(child.type!=S_IFDIR)
		return -EPERM;
	// unset data bitmap at locations taken by target directory
	unsigned char buf[BLOCK_SIZE];
	bio_read(super_block->d_bitmap_blk,buf);
	int c = 0;
	for(c=0;c<16;c++){

		if(child.direct_ptr[c] > 0){

			unset_bitmap(buf,child.direct_ptr[c]);

		}

	}
	bio_write(super_block->d_bitmap_blk,buf);
	// Step 4: Clear inode bitmap and its data block
	bio_read(super_block->i_bitmap_blk,buf);
	unset_bitmap(buf,child.ino);
	bio_write(super_block->i_bitmap_blk,buf);
	// Step 5: Call get_node_by_path() to get inode of parent directory
	// this was step 2?

	// Step 6: Call dir_remove() to remove directory entry of target directory in its parent directory

	if(dir_remove(parent, fileName, strlen(fileName))==-1)
		return -1;
	return 0;
}

static int tfs_releasedir(const char *path, struct fuse_file_info *fi) {
	// For this project, you don't need to fill this function
	// But DO NOT DELETE IT!
    return 0;
}

static int tfs_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
	printf("CREATE\n");
	// Step 1: Use dirname() and basename() to separate parent directory path and target file name
	char* _path = (char*)malloc(strlen(path) + 1);
	strcpy(_path, path);
	char * dirPath = dirName(_path);
	char * fileName = baseName(_path);
	//printf("PATH: %s, _PATH: %s, DIR: %s, BASE: %s\n", path, _path, dirPath, fileName);

	// Step 2: Call get_node_by_path() to get inode of parent directory
	inode parent;
	if (get_node_by_path(dirPath, 0, &parent) == -1) {
		return -ENOENT;
	}
	if (parent.type != S_IFDIR) {
		return -EPERM;
	}

	// Step 3: Call get_avail_ino() to get an available inode number
	int next_avail = get_avail_ino();
	//printf("NEXT_AVAIL: %d\n", next_avail);
	
	// Step 4: Call dir_add() to add directory entry of target file to parent directory
	//printf("CALL DIR_ADD - %s\n", path);
	if (dir_add(parent, next_avail, fileName, strlen(path)) == -1) {
		return -1;
	}
	//printf("FILE ADD SUCCESS!\n");
	// Step 5: Update inode for target file
	inode i;
	i.ino = next_avail;
	i.vstat.st_ino = next_avail;
	i.valid = 1;
	i.size = 0;
	i.vstat.st_size = 0;
	i.type = S_IFREG;
	i.vstat.st_mode = S_IFREG | mode;
	i.link = 1;
	i.vstat.st_nlink = 1;
	i.vstat.st_blksize = BLOCK_SIZE;
	i.vstat.st_blocks = 0;
	i.vstat.st_gid = getgid();
	i.vstat.st_uid = getuid();
	time(&(i.vstat.st_atime));
	time(&(i.vstat.st_ctime));
	time(&(i.vstat.st_mtime));
	int x;
	for (x = 0; x < 16; x++) {
		if (x % 2 == 0)
			i.indirect_ptr[x / 2] = 0;
		i.direct_ptr[x] = 0;
	}

	// Step 6: Call writei() to write inode to disk
	int chk = writei(next_avail, &i);
	if (chk < 0)
		return -1;
	return 0;
}

static int tfs_open(const char *path, struct fuse_file_info *fi) {
	printf("OPEN\n");
	// Step 1: Call get_node_by_path() to get inode from path
	inode *pd = (inode*)malloc(sizeof(inode));

	// Step 2: If not found, return -1
	if (get_node_by_path(path, 0, pd) < 0)
		return -ENOENT;
	return 0;
}

static int tfs_read(const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi) {
	printf("READ\n");
	// Step 1: You could call get_node_by_path() to get inode from path
	inode *pd = (inode*)malloc(sizeof(inode));
	/*int ino = */get_node_by_path(path, 0, pd);

	// Step 2: Based on size and offset, read its data blocks from disk
	/*starting info*/
	int block = offset / BLOCK_SIZE; 
	int offsetInBlock = offset % BLOCK_SIZE;

	/*ending info*/
	int endBlock = (offset + size) / BLOCK_SIZE;
	int offsetInEndBlock = (offset + size) % BLOCK_SIZE;

	int blocksToRead = endBlock - block;
	char blockBuf[BLOCK_SIZE];
	size_t bufSize = 0;
	int readSize = BLOCK_SIZE - offsetInBlock;
	//printf("START: %d, %d END: %d %d\n", block, offsetInBlock, endBlock, offsetInEndBlock);

	do {
		int blk_no = block + pd->direct_ptr[block];
		if(bio_read(blk_no, blockBuf) < 0)
			return -1; //some problem occured
		
		memcpy(buffer + bufSize, blockBuf + offsetInBlock, readSize);
		bufSize += readSize;
		if (block + 1 == endBlock) {
			offsetInBlock = offsetInEndBlock;
			readSize = offsetInEndBlock;
		}
		else {
			offsetInBlock = 0;
			readSize = BLOCK_SIZE;
		}
		block++;
	} while (--blocksToRead > 0);

	free(pd);
	return bufSize;

	// Step 3: copy the correct amount of data from offset to buffer

	// Note: this function should return the amount of bytes you copied to buffer
}

static int tfs_write(const char *path, const char *buffer, size_t size, off_t offset, struct fuse_file_info *fi) {
	printf("WRITE\n");
	// Step 1: You could call get_node_by_path() to get inode from path
	inode *pd = (inode*)malloc(sizeof(inode));
	if (get_node_by_path(path, 0, pd) < 0) {
		//printf("Step 1: in write is unable to find the file\n");
		return -ENOENT;
	}
	//printf("inode dump: %d %d %d %d\n", (int)(pd->direct_ptr[0]), (int)(pd->vstat.st_size), (int)(pd->vstat.st_nlink), (int)(pd->vstat.st_blocks));
	// Step 2: Based on size and offset, read its data blocks from disk
	/*starting info*/
	int block = offset / BLOCK_SIZE;
	int offsetInBlock = offset % BLOCK_SIZE;

	/*ending info*/
	int endBlock = (offset + size) / BLOCK_SIZE;
	int offsetInEndBlock = (offset + size) % BLOCK_SIZE;

	int blocksToWrite = endBlock - block;
	char blockBuf[BLOCK_SIZE];
	size_t bufSize = 0;
	int writeSize = BLOCK_SIZE - offsetInBlock;
	//printf("START: %d, %d END: %d %d\n", block, offsetInBlock, endBlock, offsetInEndBlock);

	do {
		int blk_no = pd->direct_ptr[block];
		if (blk_no == 0) { //uninitialized
			pd->direct_ptr[block] = get_avail_blkno();
			blk_no = pd->direct_ptr[block];
			pd->vstat.st_blocks++;
			pd->vstat.st_nlink++;
			pd->link++;
			//printf("Spawned New Block: %d\n", pd->direct_ptr[block]);
		}
		if(bio_read(blk_no, blockBuf) < 0)
			return -1; //some problem occured

		//printf("MEMCPY: blockbuf + %d, buffer + %d, write %d\n", (int)(offsetInBlock), (int)(bufSize), (int)(writeSize));
		memcpy(blockBuf + offsetInBlock, buffer + bufSize, writeSize);
		bufSize += writeSize;
		if (bio_write(blk_no, blockBuf) < 0) return -1;
		if (block + 1 == endBlock) {
			offsetInBlock = offsetInEndBlock;
			writeSize = offsetInEndBlock;
		}
		else {
			offsetInBlock = 0;
			writeSize = BLOCK_SIZE;
		}
		block++;
	} while (--blocksToWrite > 0);
	pd->size += bufSize;
	pd->vstat.st_size = pd->size;
	if (writei(pd->ino, pd) < 0) return -1;
	free(pd);
	// Step 3: Write the correct amount of data from offset to disk

	// Step 4: Update the inode info and write it to disk

	// Note: this function should return the amount of bytes you write to disk
	return size;
}

static int tfs_unlink(const char *path) {
	printf("REMOVE\n");
	if (super_block == NULL) return -1;

	// Step 1: Use dirname() and basename() to separate parent directory path and target file name
	char _path[strlen(path)];
	strcpy(_path, path);
	char * dirPath = dirName(_path);
	char * fileName = baseName(_path);

	// Step 2: Call get_node_by_path() to get inode of target file
	inode parent;
	if (get_node_by_path(dirPath, 0, &parent) == -1)
		return -ENOENT;
	if (parent.type != S_IFDIR)
		return -EPERM;

	// Step 3: Clear data block bitmap of target file
	inode child;

	if (get_node_by_path(path, 0, &child) == -1)
		return -ENOENT;
	if (child.type != S_IFREG)
		return -1;

	// unset data bitmap at locations taken by target directory
	unsigned char bitmap[BLOCK_SIZE];
	bio_read(super_block->d_bitmap_blk, bitmap);
	int c = 0;
	for (c = 0; c < 16; c++) {

		if (child.direct_ptr[c] > 0) {

			unset_bitmap(bitmap, child.direct_ptr[c]);

		}

	}
	if (bio_write(super_block->d_bitmap_blk, bitmap) < 0) return -1;

	// Step 4: Clear inode bitmap and its data block

	bio_read(super_block->i_bitmap_blk, bitmap);
	unset_bitmap(bitmap, child.ino);
	bio_write(super_block->i_bitmap_blk, bitmap);

	// Step 5: Call get_node_by_path() to get inode of parent directory
	

	// Step 6: Call dir_remove() to remove directory entry of target file in its parent directory
	if (dir_remove(parent, fileName, strlen(fileName)) == -1)
		return -1;
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

char* dirName(char* p) {
	char* path = (char*)malloc(strlen(p) + 1);
	strcpy(path, p);
	int x;
	for (x = strlen(path); x > 0; x--) {
		if (*(path + x - 1) == '/') {
			*(path + x - 1) = '\0';
			return path;
		}
	}
	return p;
}

char* baseName(char* p) {
	char* path = (char*)malloc(strlen(p) + 1);
	strcpy(path, p);
	int x;
	for (x = strlen(path); x > 0; x--) {
		if (*(path + x - 1) == '/') {
			return (path + x);
		}
	}
	return p;
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
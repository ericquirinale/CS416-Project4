/*
 *  Copyright (C) 2021 CS416 Rutgers CS
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
#include <errno.h>
#include <sys/time.h>
#include <libgen.h>
#include <limits.h>
#include <math.h>

#include "block.h"
#include "tfs.h"

char diskfile_path[PATH_MAX];

// Declare your in-memory data structures here
#define FILE 0
#define DIRECTORY 1
#define SUPERBLOCK 2 


bitmap_t inode_bm;
bitmap_t data_bm;
struct superblock* sb;
int inodes_per_block=(int)(BLOCK_SIZE/sizeof(struct inode));
int inode_start_block=3;
int isInit=0;

/* 
 * Get available inode number from bitmap
 * Returns -1 if no empty spot found

 */
int get_avail_ino() {
	for(int i=0;i<(sb->max_inum);i++){
		if(get_bitmap(inode_bm,i)==0){
			//Idk if we should update the bitmap here
			return i;
		}
	}
	// Step 1: Read inode bitmap from disk
	
	// Step 2: Traverse inode bitmap to find an available slot

	// Step 3: Update inode bitmap and write to disk 

	return 0;
}

/* 
 * Get available data block number from bitmap
 * Returns -1 if no empty spot found
 */
int get_avail_blkno() {
	for(int i=0;i<(sb->max_dnum);i++){
		if(get_bitmap(data_bm,i)==0){
			//Idk if we should update the bitmap here
			return i;
		}
	}
	// Step 1: Read data block bitmap from disk
	
	// Step 2: Traverse data block bitmap to find an available slot

	// Step 3: Update data block bitmap and write to disk 

	return -1;
}

/* 
 * inode operations
 */
int readi(uint16_t ino, struct inode *inode) {

  // Step 1: Get the inode's on-disk block number
	int onDiskBM=(ino/inodes_per_block)+inode_start_block;
  // Step 2: Get offset of the inode in the inode on-disk block
	int offset=ino%inodes_per_block;

  // Step 3: Read the block from disk and then copy into inode structure

	struct inode* data=malloc(BLOCK_SIZE);
	bio_read(onDiskBM,data);
	// inode = malloc(sizeof(struct inode));
	struct inode* temp = malloc(sizeof(struct inode));
	memcpy(temp,data+offset,sizeof(struct inode));

	*inode = *temp;
	free(data);
	return 0;
}

int writei(uint16_t ino, struct inode *inode) {

	// Step 1: Get the block number where this inode resides on disk
	// printf("Before 110\n");
	int onDiskBM=(ino/inodes_per_block)+inode_start_block;
	// printf("After 110\n");
	// Step 2: Get the offset in the block where this inode resides on disk
	int offset=ino%inodes_per_block;
	// Step 3: Write inode to disk 
	
	struct inode* data=malloc(BLOCK_SIZE);

	int readRet=bio_read(onDiskBM,data);
	if(readRet<0){
		printf("Error reading from disk\n");
		free(data);
		free(inode);
		return readRet;
	}
	memcpy(data+offset,inode,sizeof(struct inode));

	bio_write(onDiskBM,data);
	free(data);
	// free(inode);
	return 0;
}


/* 
 * directory operations
 */
int dir_find(uint16_t ino, const char *fname, size_t name_len, struct dirent *dirent) {

  // Step 1: Call readi() to get the inode using ino (inode number of current directory)

  // Step 2: Get data block of current directory from inode

  // Step 3: Read directory's data block and check each directory entry.
	printf("Inside dir_find\n");
	struct inode* root=malloc(sizeof(struct inode));
	struct dirent* temp_dirent;
	int readRet=readi(ino,root);
	//If there was an error finding the inode, return an error
	if(readRet<0){
		printf("Error reading inode");
		free(root);
		return -2;
	}
	//If the parameter ino is a file, return an error.
	if(root->type==FILE){
		printf("Ino is for a file\n");
		free(root);
		return -2;
	}

	struct dirent* currentBlock=malloc(BLOCK_SIZE);
	int dirents_per_block=(int) ((double)BLOCK_SIZE)/((double)sizeof(struct dirent));
	//Goes through all the datablocks of the current inode.
	for(int i=0;i<16;i++){
		if(root->direct_ptr[i]==-1){
			//This index does not have an attached datablock
			continue;
		}
		else{
			//A datablock was found, putting its data in currentblock
			bio_read(sb->d_start_blk+root->direct_ptr[i],currentBlock);
			//Going through all possible dirents in the current datablock
			for(int j=0;j<dirents_per_block;j++){
				temp_dirent=currentBlock+j;
				if(temp_dirent==NULL||temp_dirent->valid==0){
					continue;
				}
				else{
					if(strcmp(temp_dirent->name,fname)==0){
  						//If the name matches, then copy directory entry to dirent structure
						// int val=root->direct_ptr[i];
						*dirent=*temp_dirent;
						free(root);
						free(currentBlock);
						return 0;
					}
				}
			}
		}
	}
	//A directory/file with the given name was not found
	free(root);
	free(currentBlock);
	printf("Directory not found\n");
	return -1;
}

//Method to allocate memory to sb, and bitmaps and read from disk
void start(){
	if(isInit==0){
		sb= (struct superblock*) malloc(BLOCK_SIZE);
		inode_bm=malloc(BLOCK_SIZE);
		data_bm=malloc(BLOCK_SIZE);
		int sb_success=bio_read(0,sb);
		int inode_success=bio_read(1,inode_bm);
		int data_success=bio_read(2,data_bm);
		if(sb_success<0||inode_success<0||data_success<0){
			printf("Error in start\n");
		}
		isInit=1;
	}

}

//Method to write sb and bitmaps to disk and free allocated memory
void end(){
	if(isInit==1){
		isInit = 0;
		bio_write(0,sb);
		bio_write(1,inode_bm);
		bio_write(2,data_bm);
		free(sb);
		free(inode_bm);
		free(data_bm);
	}
}

int dir_add(struct inode dir_inode, uint16_t f_ino, const char *fname, size_t name_len) {
	int added=0;
	printf("Inside dir_add\n");
	struct dirent* currentBlock=malloc(BLOCK_SIZE);
	if(dir_inode.type==FILE){
		printf("dir_inode file type is not a directory \n");
		return -2;
	}
	if(dir_inode.valid==0){
		printf("dir_inode is not valid\n");
		return -2;
	}
	if(get_bitmap(inode_bm,f_ino)==1){
		printf("The new directories inode is already used somewhere\n");
		return -2;
	}
	struct dirent* tempDirent=malloc(sizeof(struct dirent));
	if(dir_find(dir_inode.ino,fname,name_len,tempDirent)!=-1){
		printf("File or folder with this name already exists in the directory");
		return -2;
	}
	printf("After checks in dir_add\n");
	free(tempDirent);
	int dirents_per_block=(int) ((double)BLOCK_SIZE)/((double)sizeof(struct dirent));
	int set=0;
	struct dirent* newDirent=malloc(sizeof(struct dirent));
	newDirent->valid=1;
	newDirent->len=name_len;
	newDirent->ino=f_ino;
	strcpy(newDirent->name,fname);
	for(int i=0;i<16;i++){
		if(set==1){
			break;
		}
		if(dir_inode.direct_ptr[i]==-1){
			continue;
		}
		else{
			bio_read(sb->d_start_blk+dir_inode.direct_ptr[i],currentBlock);
			for(int j=0;j<dirents_per_block;j++){
				struct dirent* curr=currentBlock+j;
				if(curr==NULL||curr->valid==0){
					set=1;
					currentBlock[j]=*newDirent;
					bio_write(sb->d_start_blk+dir_inode.direct_ptr[i],currentBlock);
					free(newDirent);
					added=1;
					break;
					// free(currentBlock);
				}
			}
		}
	}
	//Goes here if there is no datablocks for this directory that are empty.
	//In this case, have to update this inode, so gotta biowrite for inode;
	if(set==0){
		printf("Have to add datablock in dir_add\n");

		for(int i=0;i<16;i++){
			if(dir_inode.direct_ptr[i]==-1){
				int blockNum=get_avail_blkno();
				dir_inode.direct_ptr[i]=blockNum;
				bio_read(sb->d_start_blk+dir_inode.direct_ptr[i],currentBlock);
				currentBlock[0]=*newDirent;
				bio_write(sb->d_start_blk+dir_inode.direct_ptr[i],currentBlock);
				writei(dir_inode.ino,&dir_inode);
				set_bitmap(data_bm,blockNum);
				// free(newDirent);
				added=1;
				break;
				// dir_inode.
			}
		}
	}

	//Goes here if all of the datablocks for this inode is full. IDK what to do here
	if(added==0){
		printf("All datablocks for this inode are full\n");
		return -1;
	}
	struct inode* parent_inode=malloc(sizeof(struct inode));
	readi(dir_inode.ino,parent_inode);
	parent_inode->size+=sizeof(struct dirent);
	time(& (parent_inode->vstat.st_mtime));
	writei(parent_inode->ino,parent_inode);
	free(parent_inode);
	return 0;
	// Step 1: Read dir_inode's data block and check each directory entry of dir_inode

	// Step 2: Check if fname (directory name) is already used in other entries

	// Step 3: Add directory entry in dir_inode's data block and write to disk

	// Allocate a new data block for this directory if it does not exist

	// Update directory inode

	// Write directory entry
}

//Deleting leads to empty block, remove from parents inode and make it empty in the bitmap
int remove_block(struct inode dir_inode, struct dirent* currentBlock, int i){
	int dirents_per_block=(int) ((double)BLOCK_SIZE)/((double)sizeof(struct dirent));

	for(int j=0; j<dirents_per_block; j++){
		struct dirent* temp = currentBlock+j;
		
		//Return if the block is not empty
		if(temp!=NULL && temp->valid!=0){
			return -1;
		}
	}

	//Remove from parents inode and unset from bitmap
	dir_inode.direct_ptr[i] = -1;
	writei(dir_inode.ino, &dir_inode);

	unset_bitmap(data_bm, i);

	return 0;
}

//TODO: Finish this method
int dir_remove(struct inode dir_inode, const char *fname, size_t name_len) {

	//Find the dirent corresponding to fname
	//Delete it (set valid to 0 in dirent) and delete corresponding inode(set valid to 0 and ) 
	//If deleting it results in an empty block, remove it from parents inode and make it empty in the bitmap

	if(dir_inode.type==FILE){
		printf("Given inode is for a file, not a directory\n");
		return -2;
	}
	struct dirent* currentBlock=malloc(BLOCK_SIZE);
	int dirents_per_block=(int) ((double)BLOCK_SIZE)/((double)sizeof(struct dirent));
	
	for(int i=0;i<16;i++){
		if(dir_inode.direct_ptr[i]!=-1){
			bio_read(sb->d_start_blk+dir_inode.direct_ptr[i], currentBlock);
			for(int j=0;j<dirents_per_block;j++){
				struct dirent* temp=currentBlock+j;
				if(temp==NULL||temp->valid==0){
					continue;
				}
				else if(strcmp(temp->name,fname)==0){
					temp->valid=0;
					//set dirent to invalid, now have to go to the inode for this dirent and set it as invalid
					struct inode* toDelete=malloc(sizeof(struct inode));
					
					//Set it to invalid
					readi(temp->ino,toDelete);
					toDelete->valid=0;
					writei(temp->ino,toDelete);
					
					//Set the bitmap for this inode to be 0 (empty)
					unset_bitmap(inode_bm, temp->ino);

					//If datablocks are all empty, unmap from bitmap and inode
					remove_block(dir_inode, currentBlock, i);
					return 0;
				}
			}
		}
	}
	printf("Directory not found\n");
	return -1;

	// Step 1: Read dir_inode's data block and checks each directory entry of dir_inode
	
	// Step 2: Check if fname exist

	// Step 3: If exist, then remove it from dir_inode's data block and write to disk

}


/* 
 * namei operation
 */
int get_node_by_path(const char *path, uint16_t ino, struct inode *inode) {
	// if(path[0]!= '/'){
	// 	printf("Invalid path\n");
	// 	return -1;
	// }
	printf("Inside get_node_by_path\n");
	printf("Path:%s\n",path);
	if(strcmp(path,"/")==0){
		printf("Inside if\n");
		readi(0,inode);
		return 0;
	}
	char* temp=malloc(strlen(path)+1);
	strncpy(temp,path,strlen(path)+1);
	char* name=malloc(256);
	name=strtok(temp,"/");
	//Splits the path up into names
	struct dirent* crtDirent=malloc(sizeof(struct dirent));
	//Initialize crtInode to the root of the directory
	struct inode* crtInode= malloc(sizeof(struct inode));
	readi(ino,crtInode);
	while(name!=NULL){
		int findRet=dir_find(crtInode->ino,name,strlen(name),crtDirent);
		if(findRet==-1){
			printf("No directory with this path\n");
			// free(crtInode);
			// free(crtDirent);
			// free(name);
			// free(temp);
			return -1;
		}
		free(crtInode);
		readi(crtDirent->ino,crtInode);
		name=strtok(NULL,"/");
	}
	*inode=*crtInode;
	free(crtInode);
	// Step 1: Resolve the path name, walk through path, and finally, find its inode.
	// Note: You could either implement it in a iterative way or recursive way
	return 0;
}

/* 
 * Make file system
 */
int tfs_mkfs() {

	// Call dev_init() to initialize (Create) Diskfile
	dev_init(diskfile_path);
	dev_open(diskfile_path);
	// write superblock information
	sb = malloc(sizeof(struct superblock));
	sb->magic_num = MAGIC_NUM;
	sb->max_inum = MAX_INUM;
	sb->max_dnum = MAX_DNUM;
	sb->i_bitmap_blk=1;
	sb->d_bitmap_blk=2;
	sb->i_start_blk=3;
	int x=(int)ceil(((double)(MAX_INUM*sizeof(struct inode)))/(double)BLOCK_SIZE);//calculations for start of datablocks i think
	sb->d_start_blk=sb->i_start_blk+x;

	// initialize inode bitmap
	inode_bm = malloc(sizeof(char)*MAX_INUM/8.0);

	// initialize data block bitmap
	data_bm = malloc(sizeof(char)*MAX_DNUM/8.0);

	// update bitmap information for root directory
	memset(inode_bm, 0, MAX_INUM/8.0);
	memset(data_bm, 0, MAX_DNUM/8.0);

	// update inode for root directory
	
	struct stat* vstat=malloc(sizeof(struct stat));
	vstat->st_mode= S_IFDIR |0755;
	time(& vstat->st_mtime);

	struct inode* root_inode=malloc(sizeof(struct inode));
	root_inode->ino=0;
	root_inode->valid=1;
	root_inode->type=DIRECTORY;
	root_inode->link=2;
	root_inode->vstat=*vstat;
	root_inode->size=0;
	for(int i=0;i<16;i++){
		root_inode->direct_ptr[i]=-1;
	}
	set_bitmap(inode_bm,0);
	bio_write(0,(void*)(sb));
	bio_write(1,(void*)(inode_bm));
	bio_write(2,(void*)(data_bm));
	// free(sb);
	// sb=malloc(sizeof(struct superblock));
	// bio_write(4,(void*)(root_inode));
	writei(0,root_inode);
	printf("End\n");
	return 0;
}


/* 
 * FUSE file operations
 */
static void *tfs_init(struct fuse_conn_info *conn) {
	printf("Init\n");
	// Step 1a: If disk file is not found, call mkfs
	int open=dev_open(diskfile_path);
	printf("OPEN:%d\n",open);
	inodes_per_block=BLOCK_SIZE/sizeof(struct inode);
	if(open==-1){
		tfs_mkfs();
	}
	else{
		sb= (struct superblock*) malloc(BLOCK_SIZE);
		inode_bm=malloc(sizeof(sizeof(char)*MAX_INUM/8.0));
		data_bm=malloc(sizeof(sizeof(char)*MAX_DNUM/8.0));
		int sb_success=bio_read(0,sb);
		int inode_success=bio_read(1,inode_bm);
		int data_success=bio_read(2,data_bm);
		
		// int inode_success=bio_read(1,inode_bm_node);
		// int data_success=bio_read(2,data_bm_node);
		if(sb_success<0||inode_success<0||data_success<0){
			printf("Couldn't find superblock or bitmap nodes\n");
			return NULL;
		}
		else{
			printf("Everything found!\n");
		}
		// dev_close();
	}



	// Step 1b: If disk file is found, just initialize in-memory data structures
	// and read superblock from disk

	return NULL;
}

static void tfs_destroy(void *userdata) {
	// printf("In destroy?\n")
	// Step 1: De-allocate in-memory data structures
	dev_close();
	// Step 2: Close diskfile

}

static int tfs_getattr(const char *path, struct stat *stbuf) {
	// return -1;
	// Step 1: call get_node_by_path() to get inode from path
	start();
	struct inode* inode=malloc(sizeof(struct inode));
	int ret=get_node_by_path(path,0,inode);
	if(ret==-1){
		printf("Get node by path\n");
		return -ENOENT;
	}
	stbuf->st_mode=inode->vstat.st_mode;
	stbuf->st_nlink=inode->link;
	stbuf->st_size=inode->size;
	stbuf->st_ino=inode->ino;
	stbuf->st_uid=getuid();
	stbuf->st_gid=getgid();
	stbuf->st_mtime=inode->vstat.st_mtime;
	printf("End of tfs_getAttr\n");
	// Step 2: fill attribute of file into stbuf from inode
	// stbuf->st_mode   = S_IFDIR | 0755;
	// stbuf->st_nlink  = 2;
	// time(&stbuf->st_mtime);
	end();
	return 0;
}

static int tfs_opendir(const char *path, struct fuse_file_info *fi) {

	// Step 1: Call get_node_by_path() to get inode from path
	struct inode* node=malloc(sizeof(struct inode));
	if(get_node_by_path(path,0,node)<0){
		printf("Path not found\n");
		free(node);
		return -1;
	}
	// Step 2: If not find, return -1
	free(node);
	return 0;
}

static int tfs_readdir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
	printf("INSIDE READDIR\n");
	// return 0;
	start();

	// Step 1: Call get_node_by_path() to get inode from path
	struct inode* node = malloc(sizeof(struct inode));
	int ret = get_node_by_path(path, 0, node);
	if(ret < 0){
		printf("No directory with path\n");
		end();
		return -1;
	}
	if(node->type!=DIRECTORY){
		printf("Path is not a directory\n");
		end();
		return -1;
	}

	// Step 2: Read directory entries from its data blocks, and copy them to filler
	struct dirent* currentBlock=malloc(BLOCK_SIZE);
	int dirents_per_block=(int) ((double)BLOCK_SIZE)/((double)sizeof(struct dirent));

	filler(buffer, ".", NULL, 0);
	filler(buffer, "..", NULL, 0);
	for(int i=0; i<16; i++){
		if(node->direct_ptr[i] != -1){
			bio_read(sb->d_start_blk+node->direct_ptr[i], currentBlock);
			for(int j=0;j<dirents_per_block;j++){
				struct dirent* temp=currentBlock+j;
				if(temp==NULL||temp->valid==0){
					continue;
				}else{ 
					filler(buffer, currentBlock->name, NULL, 0);
				}
			}
		}
	}
	end();
	free(currentBlock);
	printf("Finished readdir\n");
	return 0;
}


static int tfs_mkdir(const char *path, mode_t mode) {
	start();
	printf("INSIDE MKDIR\n");
	printf("Path in mkdir:%s\n",path);
	// Step 1: Use dirname() and basename() to separate parent directory path and target directory name
	// char* dirc;
	// char* basec;
	// dirc = strdup(path);
	// basec = strdup(path);
	char* dirc = malloc(strlen(path)+1);
	strncpy(dirc,path, strlen(path)+1);
	char* basec = malloc(strlen(path)+1);
	strncpy(basec,path, strlen(path)+1);

	char* dir_name = dirname(dirc);
	char* base_name = basename(basec);
	printf("dir_name%s\n",dir_name);
	printf("base_name%s\n",base_name);
	// Step 2: Call get_node_by_path() to get inode of parent directory
	struct inode* parent_inode = malloc(sizeof(struct inode));
	int parent_ino = get_node_by_path(dir_name, 0, parent_inode);
	if(parent_ino == -1){
		printf("Directory does not exist\n");
		end();
		return -1;
	}

	// Step 3: Call get_avail_ino() to get an available inode number
	int avail_ino = get_avail_ino();
	struct inode* new_inode = malloc(sizeof(struct inode));
	struct stat* vstat=malloc(sizeof(struct stat));
	vstat->st_mode= S_IFDIR | 0755;
	time(& vstat->st_mtime);

	new_inode->vstat = *vstat;
	new_inode->ino = avail_ino;
	new_inode->size=0;
	new_inode->link = 2;
	new_inode->valid = 1;
	new_inode->type = DIRECTORY;
	for(int i=0; i<16; i++){
		new_inode->direct_ptr[i] = -1;
	}

	// Step 4: Call dir_add() to add directory entry of target directory to parent directory
	int dir_ret = dir_add(*parent_inode, avail_ino, base_name, strlen(base_name));
	if(dir_ret < 0){
		printf("Error adding new directory");
		end();
		return -1;
	}

	// Step 5: Update inode for target directory
	
	set_bitmap(inode_bm,new_inode->ino);
	// Step 6: Call writei() to write inode to disk
	writei(avail_ino, new_inode);

	end();
	return 0;
}

static int tfs_rmdir(const char *path) {
	start();
	char* dirc = malloc(strlen(path)+1);
	strncpy(dirc,path, strlen(path)+1);
	char* basec = malloc(strlen(path)+1);
	strncpy(basec,path, strlen(path)+1);
	char* parent= dirname(dirc);
	char* target = basename(basec);
	struct inode* parentInode=malloc(sizeof(struct inode));
	int ret=get_node_by_path(parent,0,parentInode);
	if(ret==-1){
		printf("No directory with this path");
		free(parentInode);
		return -1;
	}
	struct dirent* targetDirent=malloc(sizeof(struct dirent));
	dir_find(parentInode->ino,target,strlen(target),targetDirent);
	struct inode* targetInode=malloc(sizeof(struct inode));
	readi(targetDirent->ino,targetInode);
	for(int i=0;i<16;i++){
		if(targetInode->direct_ptr[i]!=-1){
			printf("rmdir: failed to remove \'%s\': Directory not empty",target);
			free(targetInode);
			free(targetDirent);
			free(parentInode);
			return -1;
		}
	}
	
	dir_remove(*parentInode,target,strlen(target));
	unset_bitmap(inode_bm,targetInode->ino);
	end();
	printf("Removed successfully");
	return 0;
	// Step 1: Use dirname() and basename() to separate parent directory path and target directory name

	// Step 2: Call get_node_by_path() to get inode of target directory

	// Step 3: Clear data block bitmap of target directory

	// Step 4: Clear inode bitmap and its data block

	// Step 5: Call get_node_by_path() to get inode of parent directory

	// Step 6: Call dir_remove() to remove directory entry of target directory in its parent directory

}

static int tfs_releasedir(const char *path, struct fuse_file_info *fi) {
	// For this project, you don't need to fill this function
	// But DO NOT DELETE IT!
    return 0;
}

static int tfs_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
	printf("Inside tfs_create\n");

	// Step 1: Use dirname() and basename() to separate parent directory path and target file name
	char* dirc = malloc(strlen(path)+1);
	strncpy(dirc,path, strlen(path)+1);
	char* basec = malloc(strlen(path)+1);
	strncpy(basec,path, strlen(path)+1);

	char* dir_name = dirname(dirc);
	char* base_name = basename(basec);
	printf("dir_name%s\n",dir_name);
	printf("base_name%s\n",base_name);

	// Step 2: Call get_node_by_path() to get inode of parent directory
	struct inode* parent_inode = malloc(sizeof(struct inode));
	int parent_ino = get_node_by_path(dir_name, 0, parent_inode);
	if(parent_ino == -1){
		printf("Directory does not exist\n");
		end();
		return -1;
	}

	// Step 3: Call get_avail_ino() to get an available inode number
	int avail_ino = get_avail_ino();
	struct inode* new_inode = malloc(sizeof(struct inode));
	struct stat* vstat=malloc(sizeof(struct stat));
	vstat->st_mode= S_IFREG; //FILE TYPE MODE
	time(& vstat->st_mtime);

	new_inode->vstat = *vstat;
	new_inode->ino = avail_ino;
	new_inode->size=0;
	new_inode->link = 1;
	new_inode->valid = 1;
	new_inode->type = FILE;
	for(int i=0; i<16; i++){
		new_inode->direct_ptr[i] = -1;
	}

	// Step 4: Call dir_add() to add directory entry of target file to parent directory
	int dir_ret = dir_add(*parent_inode, avail_ino, base_name, strlen(base_name));
	if(dir_ret < 0){
		printf("Error adding new directory");
		end();
		return -1;
	}

	// Step 5: Update inode for target file

	set_bitmap(inode_bm,new_inode->ino);
	// Step 6: Call writei() to write inode to disk
	writei(avail_ino, new_inode);

	return 0;
}

static int tfs_open(const char *path, struct fuse_file_info *fi) {
	printf("Inside tfs_open\n");
	// Step 1: Call get_node_by_path() to get inode from path
	struct inode* inode=malloc(sizeof(struct inode));
	int ret=get_node_by_path(path,0,inode);
	free(inode);
	if(ret==-1){
		return -1;
	}
	// Step 2: If not find, return -1

	return 0;
}

static int tfs_read(const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi) {
	printf("Inside tfs_read\n");
	// Step 1: You could call get_node_by_path() to get inode from path

	// Step 2: Based on size and offset, read its data blocks from disk

	// Step 3: copy the correct amount of data from offset to buffer

	// Note: this function should return the amount of bytes you copied to buffer
	return 0;
}

static int tfs_write(const char *path, const char *buffer, size_t size, off_t offset, struct fuse_file_info *fi) {
	printf("Inside tfs_write\n");

	// Step 1: You could call get_node_by_path() to get inode from path

	// Step 2: Based on size and offset, read its data blocks from disk

	// Step 3: Write the correct amount of data from offset to disk

	// Step 4: Update the inode info and write it to disk

	// Note: this function should return the amount of bytes you write to disk
	return size;
}

static int tfs_unlink(const char *path) {
	printf("Inside tfs_unlink\n");

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


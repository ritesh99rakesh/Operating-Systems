#include<types.h>
#include<context.h>
#include<file.h>
#include<lib.h>
#include<serial.h>
#include<entry.h>
#include<memory.h>
#include<fs.h>
#include<kbd.h>
#include<pipe.h>


/************************************************************************************/
/***************************Do Not Modify below Functions****************************/
/************************************************************************************/
void free_file_object(struct file *filep)
{
    if(filep)
    {
       os_page_free(OS_DS_REG ,filep);
       stats->file_objects--;
    }
}

struct file *alloc_file()
{
  
  struct file *file = (struct file *) os_page_alloc(OS_DS_REG); 
  file->fops = (struct fileops *) (file + sizeof(struct file)); 
  bzero((char *)file->fops, sizeof(struct fileops));
  stats->file_objects++;
  return file; 
}

static int do_read_kbd(struct file* filep, char * buff, u32 count)
{
  kbd_read(buff);
  return 1;
}

static int do_write_console(struct file* filep, char * buff, u32 count)
{
  struct exec_context *current = get_current_ctx();
  return do_write(current, (u64)buff, (u64)count);
}

struct file *create_standard_IO(int type)
{
  struct file *filep = alloc_file();
  filep->type = type;
  if(type == STDIN)
     filep->mode = O_READ;
  else
      filep->mode = O_WRITE;
  if(type == STDIN){
        filep->fops->read = do_read_kbd;
  }else{
        filep->fops->write = do_write_console;
  }
  filep->fops->close = generic_close;
  filep->ref_count = 1;
  return filep;
}

int open_standard_IO(struct exec_context *ctx, int type)
{
   int fd = type;
   struct file *filep = ctx->files[type];
   if(!filep){
        filep = create_standard_IO(type);
   }else{
         filep->ref_count++;
         fd = 3;
         while(ctx->files[fd])
             fd++; 
   }
   ctx->files[fd] = filep;
   return fd;
}
/**********************************************************************************/
/**********************************************************************************/
/**********************************************************************************/
/**********************************************************************************/



void do_file_fork(struct exec_context *child) {
    /*TODO the child fds are a copy of the parent. Adjust the refcount*/
    for (int fd = 0; fd < MAX_OPEN_FILES; fd++) {
        if (child->files[fd]) child->files[fd]->ref_count++;
    }
}

void do_file_exit(struct exec_context *ctx) {
    /*TODO the process is exiting. Adjust the ref_count
      of files*/
    for (int fd = 0; fd < MAX_OPEN_FILES; fd++) {
        if (ctx->files[fd]) generic_close(ctx->files[fd]);
    }
    int fd = 0;
    while (ctx->files[fd]) {
        ctx->files[fd]->ref_count = 0;
        if (ctx->files[fd]->type == REGULAR) {
            free_file_object(ctx->files[fd]);
        } else {
            free_pipe_info(ctx->files[fd]->pipe);
        }
        fd++;
    }
}

long generic_close(struct file *filep) {
    /** TODO Implementation of close (pipe, file) based on the type
     * Adjust the ref_count, free file object
     * Incase of Error return valid Error code
     */
    if (filep == NULL) return -EINVAL;
    filep->ref_count--;
    if (filep->ref_count) return filep->ref_count;
    if (filep->type == REGULAR) {
        free_file_object(filep);
    } else if (filep->type == PIPE) {
        if (filep->mode == O_READ) {
            filep->pipe->is_ropen = 0;
            if (filep->pipe->is_wopen == 0) free_pipe_info(filep->pipe);
        } else if (filep->mode == O_WRITE) {
            filep->pipe->is_wopen = 0;
            if (filep->pipe->is_ropen == 0) free_pipe_info(filep->pipe);
        }
        free_file_object(filep);
        return 0;
    } else {
        free_file_object(filep);
    }
    return 0;
}

static int do_read_regular(struct file *filep, char *buff, u32 count) {
    /** TODO Implementation of File Read,
     *  You should be reading the content from File using file system read function call and fill the buf
     *  Validate the permission, file existence, Max length etc
     *  Incase of Error return valid Error code
     * */
    if (filep == NULL || buff == NULL) return -EINVAL;
    if (filep->mode & O_READ) {
        int size = flat_read(filep->inode, buff, count, &(filep->offp));
        filep->offp += size;
        return size;
    }
    return -EACCES;
}


static int do_write_regular(struct file *filep, char *buff, u32 count) {
    /** TODO Implementation of File write, 
    *   You should be writing the content from buff to File by using File system write function
    *   Validate the permission, file existence, Max length etc
    *   Incase of Error return valid Error code 
    * */
    if (filep == NULL || buff == NULL) return -EINVAL;
    if (filep->mode & O_WRITE) {
        int size = flat_write(filep->inode, buff, count, &(filep->offp));
        filep->offp += size;
        return size;
    }
    return -EACCES;
}

static long do_lseek_regular(struct file *filep, long offset, int whence) {
    /** TODO Implementation of lseek 
    *   Set, Adjust the ofset based on the whence
    *   Incase of Error return valid Error code 
    * */
    if (filep == NULL) return -EINVAL;
    u64 file_offset;
    if (whence == SEEK_SET) {
        file_offset = 0;
    } else if (whence == SEEK_CUR) {
        file_offset = filep->offp;
    } else if (whence == SEEK_END) {
        file_offset = filep->inode->file_size;
    } else return -EINVAL;
    file_offset += offset;
    if (file_offset >= FILE_SIZE || file_offset < 0) return -EINVAL;
    filep->offp = file_offset;
    return filep->offp;
}

int check_file_access(u64 flags, u64 mode) {
    if (!(flags & mode)) {
        return -EACCES;
    } else if (((flags & O_RDWR) == O_RDWR) && ((mode & O_RDWR) != O_RDWR)) {
        return -EACCES;
    }
    return 1;
}

extern int do_regular_file_open(struct exec_context *ctx, char *filename, u64 flags, u64 mode) {
    /**  TODO Implementation of file open,
      *  You should be creating file(use the alloc_file function to creat file),
      *  To create or Get inode use File system function calls,
      *  Handle mode and flags
      *  Validate file existence, Max File count is 32, Max Size is 4KB, etc
      *  Incase of Error return valid Error code
      * */
    struct file *filep;
    struct inode *finode;
    if ((finode = lookup_inode(filename))) {
        if (check_file_access(flags, finode->mode) == -EACCES) return -EACCES;
        finode->mode = flags;
    } else if (flags & O_CREAT) {
        finode = create_inode(filename, mode);
        if (check_file_access(flags, mode) == -EACCES) return -EACCES;
        finode->mode = flags;
    } else {
        return -EINVAL;
    }
    filep = alloc_file();
    filep->type = REGULAR;
    filep->inode = finode;
    filep->mode = flags;
    filep->ref_count = 1;
    filep->offp = 0;
    filep->pipe = NULL;
    filep->fops->read = do_read_regular;
    filep->fops->write = do_write_regular;
    filep->fops->lseek = do_lseek_regular;
    filep->fops->close = generic_close;
    int fd = 3;
    while (ctx->files[fd]) {
        fd++;
        if (fd == MAX_OPEN_FILES) return -EOTHERS;
    }
    ctx->files[fd] = filep;
    return fd;
}

int fd_dup(struct exec_context *current, int oldfd) {
    /** TODO Implementation of dup
     *  Read the man page of dup and implement accordingly
     *  return the file descriptor,
     *  Incase of Error return valid Error code
     * */
    if (oldfd >= MAX_OPEN_FILES) return -EOTHERS;
    if (!current->files[oldfd]) return -EINVAL;
    int fd = 3;
    while (current->files[fd]) {
        fd++;
        if (fd == MAX_OPEN_FILES) return -EOTHERS;
    }
    current->files[fd] = current->files[oldfd];
    current->files[oldfd]->ref_count++;
    return fd;
}


int fd_dup2(struct exec_context *current, int oldfd, int newfd) {
    /** TODO Implementation of the dup2
      *  Read the man page of dup2 and implement accordingly
      *  return the file descriptor,
      *  Incase of Error return valid Error code
      * */
    if (oldfd >= MAX_OPEN_FILES || newfd >= MAX_OPEN_FILES) return -EOTHERS;
    if (!current->files[oldfd]) return -EINVAL;
    if (oldfd == newfd) return oldfd;
    if (current->files[newfd]) {
        current->files[newfd]->fops->close(current->files[newfd]);
    }
    current->files[newfd] = current->files[oldfd];
    current->files[oldfd]->ref_count++;
    return newfd;
}

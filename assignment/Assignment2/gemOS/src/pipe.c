#include<pipe.h>
#include<context.h>
#include<memory.h>
#include<lib.h>
#include<entry.h>
#include<file.h>
/***********************************************************************
 * Use this function to allocate pipe info && Don't Modify below function
 ***********************************************************************/
struct pipe_info* alloc_pipe_info()
{
    struct pipe_info *pipe = (struct pipe_info*)os_page_alloc(OS_DS_REG);
    char* buffer = (char*) os_page_alloc(OS_DS_REG);
    pipe ->pipe_buff = buffer;
    return pipe;
}


void free_pipe_info(struct pipe_info *p_info)
{
    if(p_info)
    {
        os_page_free(OS_DS_REG ,p_info->pipe_buff);
        os_page_free(OS_DS_REG ,p_info);
    }
}
/*************************************************************************/
/*************************************************************************/


int pipe_read(struct file *filep, char *buff, u32 count) {
    /**
    *  TODO:: Implementation of Pipe Read
    *  Read the contect from buff (pipe_info -> pipe_buff) and write to the buff(argument 2);
    *  Validate size of buff, the mode of pipe (pipe_info->mode),etc
    *  Incase of Error return valid Error code 
    */
    if (filep == NULL || buff == NULL) return -EINVAL;
    if ((filep->mode & O_READ) && (filep->pipe->is_ropen)) {
        int s_pos = filep->pipe->read_pos;
        int size;
        if (count > filep->pipe->buffer_offset) size = filep->pipe->buffer_offset;
        else size = count;
        if (size <= 0) return 0;
        if (size >= PIPE_MAX_SIZE) return -EOTHERS;
        for (int i = 0; i < size; i++) {
            buff[i] = filep->pipe->pipe_buff[(i + s_pos) % PIPE_MAX_SIZE];
        }
        filep->pipe->buffer_offset -= size;
        filep->pipe->read_pos = (s_pos + size) % PIPE_MAX_SIZE;
        return size;
    }
    return -EINVAL;
}


int pipe_write(struct file *filep, char *buff, u32 count) {
    /**
    *  TODO:: Implementation of Pipe Read
    *  Write the content from   the buff(argument 2);  and write to buff(pipe_info -> pipe_buff)
    *  Validate size of buff, the mode of pipe (pipe_info->mode),etc
    *  Incase of Error return valid Error code 
    */
    if (filep == NULL || buff == NULL || count < 0) return -EINVAL;
    if ((filep->mode & O_WRITE) && (filep->pipe->is_wopen)) {
        int s_pos = filep->pipe->write_pos;
        if (filep->pipe->buffer_offset + count >= PIPE_MAX_SIZE) return -EOTHERS;
        for (int i = 0; i < count; i++) {
            filep->pipe->pipe_buff[(i + s_pos) % PIPE_MAX_SIZE] = buff[i];
        }
        filep->pipe->buffer_offset += count;
        filep->pipe->write_pos = (count + s_pos) % PIPE_MAX_SIZE;
        return count;
    }
    return -EINVAL;
}

int create_pipe(struct exec_context *current, int *fd) {
    /**
    *  TODO:: Implementation of Pipe Create
    *  Create file struct by invoking the alloc_file() function, 
    *  Create pipe_info struct by invoking the alloc_pipe_info() function
    *  fill the valid file descriptor in *fd param
    *  Incase of Error return valid Error code 
    */
    struct file *filep0 = alloc_file();
    struct file *filep1 = alloc_file();
    struct pipe_info *pipe = alloc_pipe_info();
    filep0->type = PIPE;
    filep0->mode = O_READ;
    filep0->ref_count = 1;
    filep0->offp = 0;
    filep0->fops->read = pipe_read;
    filep0->fops->close = generic_close;
    filep0->pipe = pipe;
    filep0->pipe->is_ropen = 1;
    filep0->inode = NULL;
    filep1->type = PIPE;
    filep1->mode = O_WRITE;
    filep1->ref_count = 1;
    filep1->offp = 0;
    filep1->fops->write = pipe_write;
    filep1->fops->close = generic_close;
    filep1->pipe = pipe;
    filep1->pipe->is_wopen = 1;
    filep1->inode = NULL;
    fd[0] = 3;
    while (current->files[fd[0]]) {
        fd[0]++;
        if (fd[0] >= MAX_OPEN_FILES) return -EOTHERS;
    }
    current->files[fd[0]] = filep0;
    fd[1] = fd[0];
    while (current->files[fd[1]]) {
        fd[1]++;
        if (fd[1] >= MAX_OPEN_FILES) return -EOTHERS;
    }
    current->files[fd[1]] = filep1;
    return 1;
}


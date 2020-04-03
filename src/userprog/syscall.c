#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/init.h"
#include "filesys/filesys.h"

#define MAX_NUM_FILES 100

static void       syscall_handler (struct intr_frame *);
static int        get_user (const uint8_t *uaddr);
static bool       put_user (uint8_t *udst, uint8_t byte);
int               find_fd(const char* name);

struct file* files_opened[MAX_NUM_FILES] = {NULL};
int no_of_opened_files = 0;

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  switch (read_from_mem(f->esp))
  {
  case SYS_HALT:
    shutdown_power_off();
    return ;

  case SYS_EXIT:
    //printf ("%s: exit(%d)\n", , f->error_code);
    return(read_from_mem(f->esp - 4 ));

  case SYS_CREATE:
    const char *name = read_from_mem(f->esp - 4 );
    off_t initial_size = read_from_mem(f->esp - 8 );
    if(name == -1 || initial_size == -1)
      return false;
    return(filesys_create (name, initial_size));

  case SYS_REMOVE:
    const char *name = read_from_mem(f->esp - 4 );
    if(name == -1)
      return false;
    return(filesys_remove(name));

  case SYS_OPEN:
    const char *name = read_from_mem(f->esp - 4 );
    if(name == -1)
      return -1;
    if(files_opened[no_of_opened_files++] = filesys_open(name)){
      return no_of_opened_files+1;
    }
    else
    {
      return -1;
    }

  case SYS_FILESIZE:
    int fd = read_from_mem(f->esp - 4 );
    if(fd == -1)
      return -1; 
    if(files_opened[fd-2] == NULL)
      return -1;
    else
    {
      return(file_length(files_opened[fd-2]));
    }
    
  case SYS_READ:
    int fd = read_from_mem(f->esp-4);
    void* buffer = read_from_mem(f->esp-8);
    int size = read_from_mem(f->esp-12);
    if(fd == -1 || buffer == -1 || size <= -1)
      return -1;
    if(fd == 0){
      uint8_t temp[size];
      for(int i = 0;i<size;i++){
        temp[i] = input_getc();
      }
      buffer = temp;
      return size;
    }
    return(files_opened[fd-2] ? file_read(files_opened[fd-2], buffer,
                                                 size) : -1);

  case SYS_WRITE:
    int fd = read_from_mem(f->esp-4);
    void* buffer = read_from_mem(f->esp-8);
    int size = read_from_mem(f->esp-12);
    if(fd == -1 || buffer == -1 || size <= -1)
      return -1;
    if(fd == 1){
      int i = size;
      while(i > 100){
        putbuf(buffer, size);
        i-=100;
      } 
      return size;
    }
    return(files_opened[fd-2] ? file_write(files_opened[fd-2], buffer,
                                                 size) : -1);
                                                 
  case SYS_CLOSE:
    int fd = read_from_mem(f->esp - 4 );
    if(fd == -1)
      return -1; 
    file_close(files_opened[fd -2]);
    break;

  case -1:  

  default:
    break;
  }
  thread_exit ();
}

static int
read_from_mem (const uint8_t *uaddr)
{
  if(is_user_vaddr(uaddr))
    return(get_user(uaddr));
  else
    return -1;
}

static bool
write_to_mem (uint8_t *udst, uint8_t byte)
{
  return((is_user_vaddr(udst))? put_user(udst, byte) : false);
}

/* Reads a byte at user virtual address UADDR.
   UADDR must be below PHYS_BASE.
   Returns the byte value if successful, -1 if a segfault
   occurred. */
static int
get_user (const uint8_t *uaddr)
{
  int result;
  asm ("movl $1f, %0; movzbl %1, %0; 1:"
       : "=&a" (result) : "m" (*uaddr));
  return result;
}
 
/* Writes BYTE to user address UDST.
   UDST must be below PHYS_BASE.
   Returns true if successful, false if a segfault occurred. */
static bool
put_user (uint8_t *udst, uint8_t byte)
{
  int error_code;
  asm ("movl $1f, %0; movb %b2, %1; 1:"
       : "=&a" (error_code), "=m" (*udst) : "q" (byte));
  return error_code != -1;
}

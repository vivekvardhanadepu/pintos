#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/init.h"
#include "filesys/filesys.h"

static void       syscall_handler (struct intr_frame *);
static int        get_user (const uint8_t *uaddr);
static bool       put_user (uint8_t *udst, uint8_t byte);

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
    break;
  case SYS_EXIT:
    //printf ("%s: exit(%d)\n", , f->error_code);
    return(read_from_mem(f->esp - 4 ));
  case SYS_CREATE:
    const char *name = read_from_mem(f->esp - 4 );
    off_t initial_size = read_from_mem(f->esp - 8 );
    if(name == -1 || initial_size == -1)
      return false;
    filesys_create (name, initial_size)
  case SYS_REMOVE:
    const char *name = read_from_mem(f->esp - 4 );
    if(name == -1)
      return false;
    return(filesys_remove (const char *name));
  case SYS_OPEN:

  case SYS_FILESIZE:

  case SYS_READ:

  case SYS_WRITE:

  case SYS_CLOSE:

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

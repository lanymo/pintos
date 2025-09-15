#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

void valid_addr(const void *vaddr){
	if (vaddr == NULL || !is_user_vaddr(vaddr)){
		exit(-1);
	}
}
void halt(void){
	shutdown_power_off();
}

void exit (int status){
	printf("%s: exit(%d)\n", thread_name(), status);
	thread_current() -> exit_status = status;
	thread_exit();
}

pid_t exec (const char *cmd_line){
	return process_execute(cmd_line);
}

int wait (pid_t pid){
  return process_wait(pid);
}

int read (int fd, void *buffer, unsigned size){
  if (!fd) {
    for (unsigned i = 0; i < size; i++) {
      *((uint8_t *)buffer + i) = input_getc();
    }
    return size;
  }
  return -1;
}
int write (int fd, const void *buffer, unsigned size){
  if (fd == 1){
    putbuf(buffer, size);
    return size;
  }
  else{
    return -1;
  }
}
int fibonacci(int n){
	int i1 = 0, i2 = 1, res = 0;
	if (n==0){
		return i1;
	}

	for (int i=2; i<=n; i++){
		res = i1 + i2;
		i1 = i2;
		i2 = res;
	}

	return i2;
}
int max_of_four_int(int a, int b, int c, int d){
  int Max1, Max2;
  int ret;

  if (a>b){
	Max1 = a;
  }else{
	Max2 = b;
  }

  if (c>d){
	Max2 = c;
  }else{
	Max2 = d;
  }

  if(Max1 > Max2){
	ret = Max1;
  }else{
	ret = Max2;
  }
  
  return ret;

}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
    valid_addr(f->esp);
	switch(*(uint32_t*)(f->esp)){
		case SYS_HALT:
		  halt();
		  break;
		case SYS_EXIT:
		  valid_addr(f->esp + 4);
		  exit(*(uint32_t*)(f->esp + 4));
		  break;
		case SYS_EXEC:
      	  valid_addr(f->esp + 4);
      	  f->eax = exec(*(uint32_t*)(f->esp + 4));
		  break;
		case SYS_WAIT:
      	  valid_addr(f->esp + 4);
      	  f->eax = wait( *(uint32_t*)(f->esp+4));
		  break;
		case SYS_CREATE:
		break;
		case SYS_REMOVE:
		break;
		case SYS_OPEN:
		break;
		case SYS_FILESIZE:
		break;
		case SYS_READ:
      	  valid_addr(f->esp+4);
      	  valid_addr(f->esp+8);
      	  valid_addr(f->esp+12);
      	  f->eax = read((int) *(uint32_t*)(f->esp+4),(void*)*(uint32_t*)(f->esp +8),(unsigned) * (uint32_t*)(f->esp+12));
		  break;
		case SYS_WRITE:
      	  valid_addr(f->esp+4);
      	  valid_addr(f->esp+8);
      	  valid_addr(f->esp+12);
      	  f->eax = write((int) *(uint32_t*)(f->esp+4),(const void*)*(uint32_t*)(f->esp +8),(unsigned) * (uint32_t*)(f->esp+12));
		  break;
		case SYS_SEEK:
		break;
		case SYS_TELL:
		break;
		case SYS_CLOSE:
		break;
		case SYS_FIBONACCI:
		  valid_addr(f->esp+4);
		  f->eax = fibonacci((int)*(uint32_t*)(f->esp + 4));
		break;
		case SYS_MAX_OF_FOUR_INT:
		  valid_addr(f->esp+4);
      	  valid_addr(f->esp+8);
      	  valid_addr(f->esp+12);
		  valid_addr(f->esp+16);
		  f->eax = max_of_four_int((int) *(uint32_t*)(f->esp+4),(int) *(uint32_t*)(f->esp+8),(int) *(uint32_t*)(f->esp+12),(int) *(uint32_t*)(f->esp+16));
		break;

	}
	//printf ("system call : [ %d ] \n", *(uint32_t*)(f->esp));
	//thread_exit ();
}

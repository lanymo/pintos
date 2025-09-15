#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "filesys/off_t.h"

static void syscall_handler (struct intr_frame *);

struct file {
  struct inode *inode;
  off_t pos;
  bool deny_write;
};

struct lock file_lock;

void
syscall_init (void) 
{
  lock_init(&file_lock);
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
	for(int i=3; i<128; i++){
		if(thread_current() -> fd[i] != NULL){
			close(i);
		}
	}
	thread_exit();
}
pid_t exec (const char *cmd_line){
	return process_execute(cmd_line);
}

int wait (pid_t pid){
  return process_wait(pid);
}

bool create (const char *file, unsigned initial_size){
	/*
	* create new file called file
	* initial_size bytes int size
	* return true; success / false; fail
	* removing an open file 확인; file descriptor가 남아 있음음
	*/
	valid_addr(file);
	return filesys_create(file, initial_size);
}

bool remove (const char *file){
	/*
	* file이라는 이름의 file 삭제
	* return true if success, false otherwise
	*/
	valid_addr(file);
	return filesys_remove(file);
}

int open (const char *file){
	/*
	* 여러번 열렸으면 새로운 fd를 계속 할당한다
	* 하나의 파일에 데한 다른 fd는 독립적으로 close 된다
	*/
	valid_addr(file);
	lock_acquire(&file_lock);

	struct file* f = filesys_open(file);

	if(f == NULL){
		lock_release(&file_lock);
		return -1;
	}else{
		for(int i=3; i<128; i++){
			if(thread_current() -> fd[i] == NULL){
				if(strcmp(thread_current()->name, file) == 0){
					file_deny_write(f);
				}
				thread_current()->fd[i] = f;
				lock_release(&file_lock);
				return i;
			}
		}
	}
	lock_release(&file_lock);
	return -1;
}


int filesize (int fd){
	/*
	* return file size in bytes
	*/
	if(thread_current() -> fd[fd] == NULL){
		exit(-1);
	}
	if(!fd) return -1;
	return file_length(thread_current()->fd[fd]);
}

int read (int fd, void *buffer, unsigned size){
  valid_addr(buffer);
  lock_acquire(&file_lock);

  if (!fd) {
    for (unsigned i = 0; i < size; i++) {
      *((uint8_t *)buffer + i) = input_getc();
    }
    lock_release(&file_lock);
    return size;
  }else if(fd > 2){
	if(thread_current()->fd[fd] == NULL){
		exit(-1);
	}
	int r = file_read(thread_current()->fd[fd], buffer, size);
	lock_release(&file_lock);
	return r; 
  }
  lock_release(&file_lock);
  return -1;
}

int write (int fd, const void *buffer, unsigned size){
    valid_addr(buffer);

    struct file* f;
    lock_acquire(&file_lock);
    if (fd == 1){
      putbuf(buffer, size);
      lock_release(&file_lock);
      return size;
    }
    else if (fd > 2){
      	if(thread_current()->fd[fd] == NULL){
			lock_release(&file_lock);
			exit(-1);
  		}
		int r = file_write(thread_current()->fd[fd], buffer, size);
		lock_release(&file_lock);
		return r;
    }
    lock_release(&file_lock);
    return -1;
}

void seek (int fd, unsigned position){
	if(thread_current() -> fd[fd] == NULL){
		exit(-1);
	}
	file_seek(thread_current()->fd[fd], position);
}

unsigned tell (int fd) {
	if(thread_current() -> fd[fd] == NULL){
		exit(-1);
	}
	return file_tell(thread_current()->fd[fd]);
}

void close (int fd){
	if(thread_current()->fd[fd] == NULL){
		exit(-1);
	}
	file_close(thread_current()->fd[fd]);
	thread_current()->fd[fd] = NULL;
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
		  valid_addr(f->esp + 4);
		  valid_addr(f->esp + 8);
		  f->eax = create((const char*)*(uint32_t*)(f->esp + 4), *(uint32_t*)(f->esp + 8));
		break;
		case SYS_REMOVE:
		  valid_addr(f->esp+4);
		  f->eax = remove((const char*)*(uint32_t*)(f->esp + 4));
		break;
		case SYS_OPEN:
		  valid_addr(f->esp+4);
		  f->eax = open((char*)*(uint32_t*)(f->esp + 4));
		break;
		case SYS_FILESIZE:
		  valid_addr(f->esp+4);
		  f->eax = filesize((int)*(uint32_t*)(f->esp+4));
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
		  valid_addr(f->esp + 4);
		  valid_addr(f->esp + 8);
		  seek((int)*(uint32_t*)(f->esp+4), *(uint32_t*)(f->esp+8));
		break;
		case SYS_TELL:
		  valid_addr(f->esp+4);
		  f->eax = tell((int) * (uint32_t*)(f->esp + 4));
		break;
		case SYS_CLOSE:
		  valid_addr(f->esp+4);
		  close((int)*(uint32_t*)(f->esp + 4));
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

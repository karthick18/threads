/*  
;    Copyright (C) 2003-2004 A.R.Karthick 
;    <a_r_karthic@users.sourceforge.net>
;
;    This program is free software; you can redistribute it and/or modify
;    it under the terms of the GNU General Public License as published by
;    the Free Software Foundation; either version 2 of the License, or
;    (at your option) any later version.
;
;    This program is distributed in the hope that it will be useful,
;    but WITHOUT ANY WARRANTY; without even the implied warranty of
;    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;    GNU General Public License for more details.
;
;    You should have received a copy of the GNU General Public License
;    along with this program; if not, write to the Free Software
;    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
;
;
*/
#ifndef _TASK_H
#define _TASK_H

#define NR_TASKS 64
#define BUFFER 1024
#define HW_CACHE_ALIGN 32 //align it on a cacheline boundary
#define __cacheline_aligned(bytes)  __attribute__((__aligned__((bytes))))
#define MAX_PID ( (1 << 16) - 1)
#define INIT_TASK_PID 0
#define PAGE_SHIFT 12
#define PAGE_SIZE  ( 1 << PAGE_SHIFT)
#define TASK_SIZE  ( PAGE_SIZE << 2)
#define PAGE_MASK  ( ~(PAGE_SIZE - 1)  )
#define PAGE_ALIGN(entity)    ( ( (entity) + PAGE_SIZE - 1 ) & (PAGE_MASK) )
#define TASK_STACK_START   0x30 //around 50 bytes

struct regs {
    unsigned long eax; 
    unsigned long ebx;
    unsigned long ecx;
    unsigned long edx;
    unsigned long esi;
    unsigned long edi;
    unsigned long cs; 
    unsigned long ds;
    unsigned long esp;
    unsigned long ebp;
    unsigned long eip;
    unsigned long eflags;
    unsigned long r8;
    unsigned long r9;
    unsigned long r10;
    unsigned long r11;
    unsigned long r12;
    unsigned long r13;
    unsigned long r14;
    unsigned long r15;
};

struct thread_struct {
  void (*thread_eip) ( void *); //pointer to the thread function which is the EIP
  void *thread_arg;  

  struct regs thread_regs;
};

//define the various possible states of tasks

enum task_state {
  TASK_RUNNING  =            0x1,
  TASK_SLEEPING =            0x2,
  TASK_STOPPED  =            0x4,
  TASK_INTERRUPTIBLE =       0x8,
  TASK_UNINTERRUPTIBLE =     0x10,
  TASK_ZOMBIE          =     0x20, 
  TASK_BITMASK         =     0x3f, //6 bit ON
};

struct t_sem_undo;
struct t_sem_queue;

struct task_struct {
  struct list_head next; //list heads of task structs
  enum task_state state;
  int pid; //pid of the task struct
  int ppid; //parent id of the task struct
  struct task_struct *h_next; //hash linkage of the task struct
  struct task_struct **h_prev; //hash linkage of the task struct
  struct task_struct **queue; //linkage to the main task queue
  struct list_head run_queue; //run_queue per task
  struct t_sem_undo *sem_undo; //pointer to the processes list of undo structs
  struct t_sem_queue *sleeper;//pointer to the processes sleeping queue
  unsigned long ticks; //timer for the task
  unsigned long timeout; //timer expiry for the process
  unsigned long rt_priority; //priority of the task
  unsigned long counter; //static priority of the task
  int nice_level; //nice level for the task -20 to +19
  unsigned long stack_start; 
  struct thread_struct thread_struct; //thread struct for the task
}__cacheline_aligned(HW_CACHE_ALIGN); //align on a cacheline boundary

/* Context to switch to the main programming thread */

struct main_context {
  struct thread_struct main_thread; //save the main thread context
};

extern struct main_context main_context;
extern int save_regs   (struct regs *) asm("save_regs");
extern int restore_regs(struct regs *) asm("restore_regs");
extern int restore_ret_regs(struct regs *,void *) asm("restore_ret_regs");
extern int create_thread(void (*)(void *),void *);
extern int run_thread(int);
extern int execute_threads(void);
#endif





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
/*
 Task creation and management routines. 
*/

#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<stdarg.h>
#include<errno.h>
#include "mylist.h"
#include "task.h"
#include "myhash.h"
#include "sched.h"
#include "util.h"
#include "sem.h"

struct task_struct *tasks[MAX_PID + 1]; //total nr. of tasks
struct task_struct *task_hash_table[HASH_TABLE_SIZE]; //hash linkage
int nr_tasks;
int last_pid = 0; 

DECLARE_LIST_HEAD(init_task); 
struct main_context main_context;//save the main context

/*Routines to add tasks into the Task table */

static __inline__ int get_pid(void) {
  int save_pid;
  struct list_head *head = &init_task; //head of tasks
  struct list_head *traverse;
  ++last_pid;
  save_pid = last_pid;
  rescan :
    list_for_each(traverse,head) {
    struct task_struct *task = list_entry(traverse,struct task_struct,next);
    if( ! (last_pid & MAX_PID) ) //max limit has been crossed,rescan 
      {
	last_pid = 0;
      } else if(task->pid == last_pid) {
       ++last_pid;
      }else {
        continue;
      }
    if(last_pid == save_pid) goto out; //not found
         goto rescan; //start the scan from the next point      
  }
  //we are here when the pid has been successfully found
  return last_pid;
 out:
  return -1;
}

//Not really required.But doesnt harm anyway

static __inline__ void return_pid() {
  --last_pid;
  if(last_pid < 0) last_pid = 0;
  return ;
}

static __inline__ void task_add(struct task_struct *task) { 
  struct list_head *head = &init_task;
  list_add_tail(&task->next,head); //add to the list
  //add to the hash table
  task_hash_add(task); 
  //Now add to the global pointer table.Not used.Use the hash table
  task->queue = &tasks[task->pid]; //pointer to the queue
  tasks[task->pid] = task; 
  ++nr_tasks;
  return ;
}

void task_del(struct task_struct *task) {
  list_del(&task->next); //remove the task from the list
  task_hash_del(task); //unhash the task from the hash list
  *task->queue = NULL; //zero off the queue
  return_pid(); //give up the pid
  --nr_tasks; //decrement the task count
  //release the task_struct associated with the task
  free((void*)task); 
}

/*Fill up a task struct */
struct task_struct *create_task() {
  struct task_struct *new_task;
  if(nr_tasks >= NR_TASKS) goto out;
  if( !( new_task = ALLOC_MEM(new_task,1,TASK_SIZE) ) ) 
    goto out;
  new_task->state = TASK_SLEEPING; 
  new_task->pid = get_pid(); //get the pid of the task
  if(new_task->pid < 0) goto out_free; //didnt get the pid
  new_task->ppid =    INIT_TASK_PID;
  new_task->counter = START_PRIORITY; //setup the start counter for the task
  task_add(new_task);
  return new_task;
 out_free:
  free((void*)new_task); //release the memory for the new task
 out:
  return NULL;
}

 /* Setup the task stack. There is a main controlling thread stack associated with each thread,which is a part of the task_struct itself,just in the same
 way as the Linux Kernel.
 Setup a safety limit for the task stack start as we have a return
 address for a task that is getting killed as ours,which will call schedule after resetting the current pointer to Null.
 Note: Stack is aligned on a TASK_SIZE boundary,just like in the linux kernel.
  */
static __inline__ void setup_task_stack(struct task_struct *task) {
  unsigned long esp=(unsigned long )task; 
  esp += TASK_SIZE;
  //we dont want this code as it might fail sometimes.
#if 0 
  if(! (esp & (TASK_SIZE - 1) ) ) { //already aligned.
    esp += TASK_SIZE;
  }else { //aligned on the nearest TASK_SIZE boundary
  esp = (esp + TASK_SIZE - 1) & (~(TASK_SIZE - 1) ) ;
  }
#endif
  esp -= TASK_STACK_START; //setup a safety limit
  task->stack_start = task->thread_struct.thread_regs.esp = esp; //setup the task esp 
  task->thread_struct.thread_regs.ebp = esp; //make the ebp as the stack pointer
  return;
}
  
/* Create a thread routine.
  Creates a thread and returns the thread pid 
 if successfull in creating the thread.
*/

int create_thread(void (*thread_eip) (void *),void *arg ) {
  struct task_struct *task = create_task(); //create a task
  if(! task) 
    goto out; //return -1
  task->thread_struct.thread_eip = thread_eip;
  task->thread_struct.thread_arg = arg ;
  task->thread_struct.thread_regs.eip = (unsigned long)thread_eip; //setup the EIP
  task->nice_level = DEFAULT_NICE_LEVEL; //setup the default NICE level of a task
  setup_task_stack(task);//setup the Task stack associated with each task
  //store the non modifying registers in the task struct from the main context
  task->thread_struct.thread_regs.eflags = main_context.main_thread.thread_regs.eflags;
  task->thread_struct.thread_regs.cs = main_context.main_thread.thread_regs.cs;
  task->thread_struct.thread_regs.ds = main_context.main_thread.thread_regs.ds;

  return task->pid; //return the id of the task
 out:
  return -1;
}


/* run_thread:
 This routine adds the created task into the Run queue.
 Takes the task_pid as an argument,looks up the hash table to get the pointer to the task,and then adds that task to the run queue
*/
int run_thread(int pid) {
  struct task_struct *task = task_hash_find(pid); //lookup the hash table
  if(! task)
    goto out; //return not found
 
  //task found.Now add it to the main run queue.
  task->state = TASK_RUNNING; //only RUNNing tasks allowed on the run queue
  list_add_tail(&task->run_queue,&init_run_queue);
  return 0;
 out:
  return -1;
}

static void initialise(void) {
  ipc_init(); //initialise the IPC
}

/* Execute thread will save the current main context in the main context.
  This routine will save the main context to switch to after all the threads die off.
  First save the current main context,and call schedule to pick up a new thread. Execute all the threads in the run queue
  */

int execute_threads(void) { 
  struct list_head *run_queue  = &init_run_queue;
  struct regs *ptr = &main_context.main_thread.thread_regs; //save the main threads context in this structure
  if(LIST_EMPTY(run_queue) ) { //if the run queue is empty
    fprintf(stderr,"Run Queue is Empty.Returning...\n");
    goto out;
  }
  if(! save_regs(ptr) ) { 
    fprintf(stderr,"Performing a switch to execute the threads after saving the main context:");
    initialise();
    schedule(); //run the scheduler that switches to the next process
  }else { 
    if(reaper) task_del(reaper); //release any zombie tasks
    stop_timer();//stop the timer
    ipc_release(); //release the IPC resources
    fprintf(stderr,"Returned after all the threads have exited:");
  }
  return 0;
 out:
  return -1;
}




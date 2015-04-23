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
 The scheduler for the Thread Implementation.
 Gets called at an interval of 100 ms and hence 10 times per second.
*/

#include<stdio.h>
#include "mylist.h"
#include "task.h"
#include "myhash.h"
#include "sched.h"
#include "util.h"
#include "sem.h"

DECLARE_LIST_HEAD(init_run_queue);//declare the run queue for the tasks

#define IS_STACK_START(task)   ( (task)->stack_start == (task)->thread_struct.thread_regs.esp )

#define RESTORE_REGS(process,registers,argument) do {   \
        if(IS_STACK_START(process) ) {                  \
            restore_ret_regs(registers,argument);       \
        } else {                                        \
            restore_regs(registers);                    \
        }                                               \
    }while(0)


struct task_struct *current ;

struct task_struct *reaper = NULL; //reaper to be reaped for ZOMBIES

volatile unsigned long jiffies; //nr. of ticks since the start of controlling thread

/*Compute the goodness value of a task */

static int goodness(const struct task_struct *task) {
    int points = task->counter; //give the time slice remaining as the goodness value
    if(task->rt_priority) {
        points = task->counter + 1000; //give a high goodness value to real time threads
        goto out; 
    }
    if(task->ticks > JIFFIES_LIMIT ) 
        points = task->counter >> 1 ; //reduce the points
    out:
    return points; 
}

/*The thread scheduler */

void schedule(void) {
    struct task_struct *prev_task = current; //defaults to the current thread switched out
    struct task_struct *next_task = prev_task; //defaults to the previous task
    struct list_head *run_queue = &init_run_queue; //pointer to the run queue of tasks
    struct list_head *traverse, *n;
    int max = 0;//max goodness

    /* First free of ZOMBIES */
    if(reaper && reaper->state == TASK_ZOMBIE) {
        task_del(reaper); //free the task
        reaper = NULL;
    }

    if(LIST_EMPTY(run_queue) ) { //run queue empty 
        fprintf(stderr,"Scheduler: Restoring the Main Context..\n");
        goto restore_main; //restore the main context
    }
    
    if( prev_task && prev_task->state == TASK_ZOMBIE) {
        //exiting task
        reaper = prev_task; //freeup in the next pass if possible
        next_task = prev_task = NULL; //reset the prev_task    
    }

    compute_next_task: //get the next task in the run queue

    list_for_each_safe(traverse,n,run_queue) { 
        struct task_struct *task = list_entry(traverse,struct task_struct,run_queue);
        int points = goodness(task) ;  //get the goodness of the task
        if(points > max) {
            max = points;
            next_task = task; 
        }
    
    }
  
    if(! max) { //all the tasks have equal goodness value of 0
        goto recalculate; //compute the goodness again
    }
  
    //We are here when we have found the next task
    current = next_task; //reset the value of current to the next task
    switch_to(prev_task); //switches context to the next task
    return ; //we are here when the scheduled out thread starts running

    recalculate:
    {
        struct list_head *run_queue = &init_run_queue;
        struct list_head *traverse, *n;
        list_for_each_safe(traverse,n,run_queue) {
            struct task_struct *task = list_entry(traverse,struct task_struct,run_queue);
            task->counter = (task->counter >> 1) - task->nice_level; //recalculate the counters
        }
        goto compute_next_task;
    }

    restore_main: 
    { //restore the main context
        struct regs *ptr = &main_context.main_thread.thread_regs;
        (void)restore_regs(ptr); //restore the main context back
        message(1,stderr,"Should never return here.Something wrong with saving the main context:");
    }
    return ; //should never reach here.
}

void switch_to(struct task_struct *prev_task){
    struct regs *reg1 =NULL;
    struct regs *reg2 = &current->thread_struct.thread_regs;
    start_timer(); //setup the Timer before restoring the register
    if(prev_task && prev_task == current) {
        return ; //no need to save and restore regs
    }
    if(prev_task) 
        reg1 = &prev_task->thread_struct.thread_regs; 
    if(! reg1){
        RESTORE_REGS(current,reg2,current->thread_struct.thread_arg);
    }else { 
        if(! save_regs(reg1) ) { //first pass as the thread saved its registers
            RESTORE_REGS(current,reg2,current->thread_struct.thread_arg);
        }
    }
}

void thread_reaper(void) { 
    //we align the stack before cleaning up to satisfy OSx/darwin
    __asm__ __volatile__("and %0, %%rsp" ::"i"(-TASK_STACK_ALIGN):"memory");
#if 0
    {
        unsigned long stack = 0;
        __asm__ __volatile__("mov %%rsp, %0" :"=m"(stack) : : "memory");
        fprintf(stderr, "Stack during reaper at %p, aligned : %s\n", (unsigned long*)stack,
                stack & (TASK_STACK_ALIGN-1) ? "no" : "yes");
    }
#endif
    if(current) { 
        release_thread_timer(current->pid); //release the timers for this thread
        //undo the thread semaphore
        thread_sem_exit();
        current->state = TASK_ZOMBIE; //zombie off the process
        list_del(&current->run_queue);
    }
    schedule(); //call the scheduler to switch in the next thread
}

/* Set the nice level of a thread.
   Max = -20
   Min = +19
*/

int set_nice_level(int pid,int nice_level) { 
    struct task_struct *task = task_hash_find(pid); //lookup the hash table
    if(! task) 
        goto out;
  
    if(nice_level > MIN_NICE_LEVEL) 
        nice_level = MIN_NICE_LEVEL;

    if(nice_level < MAX_NICE_LEVEL) 
        nice_level = MAX_NICE_LEVEL;
   
    task->nice_level = nice_level;
  
    return 0;

    out:
    return -1;
}

   



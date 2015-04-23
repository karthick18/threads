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
/* Timer Implementation for the Threading Lib.
   Works in the same way as Linux Kernel Timers.
*/

#include<stdio.h>
#include "mylist.h"
#include "task.h"
#include "myhash.h"
#include "sched.h"
#include "util.h"
#include "timer.h"

static DECLARE_LIST_HEAD(system_timer); //head of system timers

void init_timer(struct timer_struct *timer) {
    timer->timer.next =timer->timer.prev = NULL ; //initialise the list heads
    timer->arg = 0;
    timer->expiry = 0; //zero off the expiry
    timer->routine = NULL; //routine for the timer
}

void set_timer(struct timer_struct *ptr,unsigned long expiry,void (*func)(void *), void *arg,int timer_id) {
    ptr->expiry  = expiry; //setup the timer expiry
    ptr->routine = func; //setup the timer callback
    ptr->arg     = arg; //setup the argument
    ptr->timer_id = timer_id; //setup the timer id
    return ;
}
/* Add a timer:Adding a timer to the system activates the thread on a timer expiry.
 */
void add_timer(struct timer_struct *timer) {
  //try sorting out the timers by their expiry and add them to the list
  struct list_head *head = &system_timer;
  struct list_head *traverse;
  unsigned long expiry = timer->expiry; //timer expiry 
  //sort the list by timer expiry
  for(traverse = head->next; traverse != head; traverse=traverse->next) {
    struct timer_struct *temp = list_entry(traverse,struct timer_struct,timer);
    if(temp->expiry > expiry) {
      list_add_tail(&timer->timer,traverse); //sort and add
      goto out;
    }
  }
  //we are here when we arent able to add the timer
  list_add_tail(&timer->timer,head);
out:
  return ;
}

/*Delete a timer from the system timer */

void del_timer(struct timer_struct *timer) {
    if(timer->timer.next && timer->timer.prev ) {
        list_del(&timer->timer); //delete the timer from the list
        timer->timer.next = timer->timer.prev = NULL;
    }
    return ;
}

/* Check whether the timer list is empty or not */

int is_timer() {
  struct list_head *head = &system_timer;
  return LIST_EMPTY(head) == 0; 
}
  
/*Run the system timers*/

void run_timer(void) {
    struct list_head *head = &system_timer;
    struct list_head *traverse;
    struct list_head *next;
    //check for expired timers try running them
    for(traverse= head->next; traverse != head; traverse= next) {
        struct timer_struct *timer = list_entry(traverse,struct timer_struct,timer);
        next = traverse->next;

        if(timer->expiry && jiffies >= timer->expiry) { //the timer has expired
            void (*func)(void *) = timer->routine; //get the routine
            void *arg = timer->arg; //get the argument
            del_timer(timer); //delete the timer from the list
#ifdef DEBUG
            fprintf(stderr,"Timer expired:\n");
#endif
            (*func)(arg); //call the routine with the argument
        }
    }
    return ;
}

/*Timer expiry routine,called when the sleep_on_timeout timer expires.*/

static void handle_timeout(void *task) {
    struct task_struct *thread = (struct task_struct *)task; 
    thread = task_hash_find(thread->pid);
    if(! thread) {
        fprintf(stderr,"Unable to find the Task (%d)\n",thread->pid);
        goto out;
    }

#ifdef DEBUG
    fprintf(stderr,"Adding Task %d to the runqueue:\n",thread->pid);
#endif
    //add this task to the run queue
    list_add(&thread->run_queue,&init_run_queue); 
    out:
    return;
}
    
/* This is an interruptible sleep on timeout routine implementation.
   A thread can decide to wakeup after n ticks in the same way as the Linux Kernel.
*/

int interruptible_sleep_on_timeout(unsigned long ticks) {
   unsigned long expiry = jiffies + ticks;   //setup the timer expiry
   if(expiry > jiffies ) {
    struct timer_struct timer_struct;
    current->timeout = expiry; //set the timeout for the current process
    init_timer(&timer_struct);
    set_timer(&timer_struct,expiry,handle_timeout,(void *)current,current->pid); 
    add_timer(&timer_struct); //add the timer to the system list of timers
    list_del(&current->run_queue); //remove the task from the run queue
 #ifdef DEBUG
    fprintf(stderr,"Timeout: Scheduling out %d:\n",current->pid);
 #endif
    schedule(); //schedule out the current thread

#ifdef DEBUG
    fprintf(stderr,"Timeout: Bringing in Thread %d\n",current->pid);
#endif

    /*We will get back here when the timer routine would have installed us in the run queue */
    if(jiffies < current->timeout) {
      expiry = current->timeout - jiffies;
    }else
      expiry = 0;
   }
   return expiry;
}

/* Release a timer,called by a thread on its exit.
  Remove timers for the thread on its exit.
*/
void release_thread_timer(int timer_id) {
  struct list_head *head = &system_timer;
  struct list_head *traverse,*next;
  for(traverse= head->next; traverse != head; traverse = next) {
    struct timer_struct *timer = list_entry(traverse,struct timer_struct,timer);
    next = traverse->next;

    if(timer->timer_id == timer_id) {
 #ifdef DEBUG
      fprintf(stderr,"Releasing the Timer for the Task with Timer id %d:\n",timer->timer_id);
 #endif
       del_timer(timer);
    }
  }
  return ;
}
     

  
  





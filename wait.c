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
  Wait queue implementation routines for the thread.
  Works in the same way as the Linux Kernel.
*/

#include<stdio.h>
#include "wait.h" 
#include "task.h"
#include "sched.h"
#include "util.h"

void init_waitqueue_entry(struct wait_queue *wait_queue,struct task_struct *task){
  wait_queue->task = task; //add the task to the wait queue
  wait_queue->wait_queue_head.next = wait_queue->wait_queue_head.prev = NULL;
}

/*Add an entry to the wait queue head*/

void add_waitqueue(struct wait_queue *wait_queue,struct wait_queue_head *wait_queue_head) {
  struct list_head *element = &wait_queue->wait_queue_head;
  struct list_head *head    = &wait_queue_head->wait_queue_head;
  
  list_add_tail(element,head); 
  return ;
}

void remove_waitqueue(struct wait_queue *wait_queue) {
  struct list_head *element = &wait_queue->wait_queue_head;
  list_del(element); //remove the element from the list
  return ;
}

void init_waitqueue_head(struct wait_queue_head *wait_queue_head) {
  struct list_head *head = &wait_queue_head->wait_queue_head;
  INIT_LIST_HEAD(head); //initialise the list head
}

static __inline__ void remove_from_run_queue(struct list_head *run_queue) {
  if(run_queue->next && run_queue->prev) {
    list_del(run_queue); 
    run_queue->next = run_queue->prev = NULL;
  }
}
static __inline__ void add_to_run_queue(struct list_head *run_queue) {
  //add the thread to the run queue
   list_add(run_queue,&init_run_queue);
}  

/*Adds the current process to the wait queue */

static __inline__ void  __sleep_on(struct wait_queue *wait_queue,struct wait_queue_head *wait_queue_head) {
  add_waitqueue(wait_queue,wait_queue_head); //add to the waitqueue
  remove_from_run_queue(&current->run_queue); //remove from run queue
#ifdef DEBUG
  fprintf(stderr,"Sleep On: Scheduling out Thread %d\n",current->pid);
#endif

  schedule(); //schedule out the task 
  remove_waitqueue(wait_queue);

#ifdef DEBUG
  fprintf(stderr,"Sleep On: Bringing in Thread %d\n",current->pid);
#endif
  
}

void sleep_on(struct wait_queue *wait_queue,struct wait_queue_head *wait_queue_head) {
  current->state = TASK_UNINTERRUPTIBLE;  //sets it to uninterruptible
  //setup the wait_queue entry
  init_waitqueue_entry(wait_queue,current); 
  __sleep_on(wait_queue,wait_queue_head);
  //we are here when the scheduled out thread starts running
}

void interruptible_sleep_on(struct wait_queue *wait_queue,struct wait_queue_head *wait_queue_head) {
  current->state = TASK_INTERRUPTIBLE;
  init_waitqueue_entry(wait_queue,current); //initialise a waitqueue entry with the current task
  __sleep_on(wait_queue,wait_queue_head); //go for an interruptible sleep
  //we are here when the scheduled out thread starts running
  return ;
}

/*Uninterruptible version of wake_up*/

static __inline__ void __wake_up(struct wait_queue_head *wait_queue_head) {
  //loop through the threads in the wait queue and wake them up 
  struct list_head *head = &wait_queue_head->wait_queue_head;
  struct list_head *traverse;
  //go through the threads in the wait queue and add them to the run queue
  list_for_each(traverse,head) {
    struct wait_queue *wait_queue = list_entry(traverse,struct wait_queue,wait_queue_head); //get the wait_queue element
    struct task_struct *task = wait_queue->task; //get the thread
    if(task) { 
     task->state = TASK_RUNNING;//change the state of the TASK
     fprintf(stderr,"Adding Thread(%d) to the run queue:\n",task->pid);
     add_to_run_queue(&task->run_queue); //add the thread to the run queue  
    }
  }
  return ;
}

/*Wake up guys in the wait queue*/

void wake_up(struct wait_queue_head *wait_queue_head) {
  __wake_up(wait_queue_head);
  return;
}

void wake_up_interruptible(struct wait_queue_head *wait_queue_head) {
  wake_up(wait_queue_head); //just call wake up
  return ;
}




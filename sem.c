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
 Implementation of Semaphores for Threads.
 Done in the same way as the Linux Kernel.
 Note about the Operations on the Semaphores,for people
 who are new to the concept of Semaphores:
 Please note that this is a user level Semaphore,
 and not a Kernel Level Semaphore.
 A Semaphore is used to synchronize access to a shared resource.
 The value of a semaphore denotes the resources that are synchronized.
 There are 3 operations possible on a semaphore:
 Increment operation on a Semaphore value identified by a semaphore index:
 An increment operation is a non blocking operation,and always succeeds.
 
 Decrement Operation:
 If the decrement operation ends up decreasing the value of the semaphore to less than zero,than the operation is termed as blocking.
 In a blocking call,the thread making the call is scheduled out,and a new one is scheduled in.

Wait till Zero Operation:
 This waits till the value of semaphore becomes zero.Else goes for a block.
*/

#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<errno.h>
#include "mylist.h"
#include "task.h"
#include "sched.h"
#include "util.h"
#include "sem.h"

#define lock    sem_lock 
#define unlock  sem_unlock 

/* 
 sem_create_id:
 @key:  key for the semaphore
 @nsems:number of semaphores per sem array
 returns the semaphore id
*/
struct t_ipcids sem_ipcids;

static struct t_sem_array *sem_create_id(int key,int nsems) {
  struct t_sem_array *ptr;
  if( !(ptr = ALLOC_MEM(ptr,1,nsems * sizeof(struct t_sem) ) ) ) {
  #ifdef DEBUG
    fprintf(stderr,"sem_create_id:Unable to allocate Memory for the semaphore:\n");
  #endif
    goto out;  
  }
  ptr->key = key;
  ptr->ctime= time(NULL);
  ptr->otime= time(NULL);
  if(! ipc_alloc(ptr) ) {
  #ifdef DEBUG
    fprintf(stderr,"sem_create_id:Unable to allocate id for the semaphore:\n");
  #endif
    goto out_free;
  }
  ptr->nsems  = nsems;
  ptr->t_sem_base =  (struct t_sem*) (ptr + 1); //pointer to the sem base
  ptr->pid = current->pid; //pid of the process acting on the semaphore
  ptr->last = &ptr->first;
  return ptr;
 out_free:
  free((void*)ptr); //free up the allocated sem array
 out :
    return NULL;
}

static __inline__ struct t_sem_array *sem_get_array(int id) {
  lock(); //lock the access to the IPC
  return ipc_get(id);
}   

static __inline__ struct t_sem_array *sem_find_array(int key) {
  lock();
  return ipc_find(key);
}

/* Create a semaphore.Returns the id of the semaphore created.*/

int thread_sem_create(int key,int nsems,int flag) {
  struct t_sem_array *ptr;
  int error=-1;
  if(!key || nsems > T_SEMMNO || (flag & ~T_IPC_CREAT) )
    goto out;

  if( (ptr = sem_find_array(key) ) && (flag & T_IPC_CREAT) ) {
    //the sem_array exists for that key.Unlock and return an error
    goto out_unlock;
  }else if(!ptr && (flag & T_IPC_CREAT) ) { 
    if(!(ptr = sem_create_id(key,nsems) ) )
      goto out_unlock;
  }else if(! ptr) {  //if the flag is 0 and the sem_array doesnt exists
    goto out_unlock; 
  }else if(ptr->nsems != nsems){
    goto out_unlock;
  }
  //we are here when the call succeeds
 error = ptr->id; //return the id of the semaphore
 out_unlock: 
 unlock();
 out:
 return error;
} 
  
/*Try performing the operations on the semaphore set.
  Returns 0 on a success,1 on a block and less than 0 for errors
*/

int try_atomic_semop(struct t_sem_array *sem_array,struct t_semop *semop,int nsops,struct t_sem_undo *sem_undo,int do_undo,int pid) {
  int error=-1,i;
  struct t_semop *curr =semop;
  for(i=0;i<nsops;++i) {
    struct t_sem *ptr = sem_array->t_sem_base + curr[i].sem_num; //pointer to semaphore
    ptr->pid = ptr->pid << 16 | pid;

    if(! curr[i].sem_op && ptr->val) 
      goto would_block; //the call would block

    ptr->val += curr[i].sem_op; 

    if(curr[i].sem_flag & T_SEM_UNDO) {
      sem_undo->semadj[curr[i].sem_num] -= curr[i].sem_op; 
    }

    if(ptr->val < 0) 
      goto would_block;
    if(ptr->val > T_SEMMVAL) 
      {
	error = -ERANGE;
        goto undo;
      }
  } 
  error = 0; 
  if(do_undo) {
    i--;
    goto undo; //undo the changes made
  }

 out: 
  return error;

 would_block: 
  error = 1;

 undo: //undo the actions performed
  {
    int j;
    for(j=i; j>=0; --j) {
      struct t_semop *ptr = semop + j ; 
      struct t_sem *sem = sem_array->t_sem_base + ptr->sem_num;
      sem->pid >>= 16;
      sem->val -= ptr->sem_op; 
      if(ptr->sem_flag & T_SEM_UNDO) 
        sem_undo->semadj[ptr->sem_num] += ptr->sem_op; 
    }
  }
  goto out;
}

/*Semaphore Queue manipulation routines.
*/

static void append_to_queue(struct t_sem_queue *sem_queue,struct t_sem_array *sem_array) {
  sem_queue->prev = sem_array->last;
  *sem_array->last = sem_queue;
  sem_array->last = &sem_queue->next; //update the last element of the array
}

static void prepend_to_queue(struct t_sem_queue *sem_queue,struct t_sem_array *sem_array) { 
  if( (sem_queue->next = sem_array->first) ) {
    sem_queue->next->prev = &sem_queue->next;
  } else sem_array->last = &sem_queue->next;
  sem_array->first = sem_queue;
  sem_queue->prev = &sem_array->first;
}

static void remove_from_queue(struct t_sem_queue *sem_queue,struct t_sem_array *sem_array) {
  if(sem_queue->prev) {
  if(sem_queue->next) {
    sem_queue->next->prev = sem_queue->prev;
  } else 
    sem_array->last = sem_queue->prev; //modify sem_arrays last queue pointer
  *sem_queue->prev = sem_queue->next;
  sem_queue->prev = NULL; //indicates removal from queue
  }
  return ;
}   

/* Destroy the array. Wake up the elements sleeping on this array
   and invalidate the queue status.
*/

static int ipc_destroy_array(int id) {
  struct t_sem_queue *ptr; 
  struct t_sem_array *sem_array;
  struct t_sem_undo *sem_undo;
  int error = -1;
  if(! (sem_array = ipc_free(id) ) ) 
    goto out;
  //invalidate the undo structures for this array.Will be freed on sem_exit or next semop
  for(sem_undo = sem_array->t_sem_undo ; sem_undo ; sem_undo = sem_undo->array_next)
    sem_undo->id = -1;

  for(ptr = sem_array->first; ptr; ptr = ptr->next) {
    ptr->status = -EIDRM; //identifier removed
    ptr->sleeper->state = TASK_RUNNING;
    list_add_tail(&ptr->sleeper->run_queue,&init_run_queue); //wake up this thread
    ptr->prev = NULL; //remove the queue elements
  }
  free((void *)sem_array);
  error = 0;
  out : 
    return error;
}

/* Update the queue */

static void update_queue(struct t_sem_array *sem_array) {
  struct t_sem_queue *sem_queue;
  int error = -1;
  for(sem_queue = sem_array->first; sem_queue ; sem_queue= sem_queue->next) {
    if(sem_queue->status == 1) 
      {
	//just returned to try again.
	continue;
      }
    error = try_atomic_semop(sem_queue->sem_array,sem_queue->semop,sem_queue->nsops,sem_queue->sem_undo,sem_queue->alter,sem_queue->pid);
   if(error <= 0) { //operation possible.Wake this up
     list_add_tail(&sem_queue->sleeper->run_queue,&init_run_queue);   
     sem_queue->sleeper->state = TASK_RUNNING;
     if(!error && sem_queue->alter) {
     sem_queue->status = 1;
     return;
   }    
     sem_queue->status = error;
     remove_from_queue(sem_queue,sem_array); //remove from the queue
   }
  }
  return ;
}

/* start the semctl operations.
  Only two operations implemented.
  T_SET_VAL and T_IPC_RMID */

int thread_semctl(int semid,int semnum,int cmd,union t_semun *semun) {
  struct t_sem_array *sem_array;
  int error = -1;

  if(! (sem_array = sem_get_array(semid) ) ) {
    goto out_unlock; 
  }
   
  if(semnum >= sem_array->nsems) 
    goto out_unlock;
  
  sem_array->otime = time(NULL); 

  switch(cmd) { 
 
  case T_SET_VAL: //set the value of the semaphore
    {
      struct t_sem *sem = sem_array->t_sem_base + semnum; //pointer to correct sem
      int val = semun->sem_val; //get the value to be set
  
      if(val > T_SEMMVAL) 
	goto out_unlock;
      sem->val = val; //set the value of the sem
      sem->pid = current->pid; //set the pid of the thread modifying the sem
      
      {
	struct t_sem_undo *sem_undo;
        for(sem_undo = sem_array->t_sem_undo; sem_undo ; sem_undo = sem_undo->array_next) { 
          sem_undo->semadj[semnum] = 0;  //reset the undo values for this sem.
	}
      }
      update_queue(sem_array); //update the queue for this array,so that any thread waiting for this change might be woken up.
    }
    break;

  case T_IPC_RMID: //remove the semaphore identified by id
 
    if(ipc_destroy_array(semid) < 0 ) 
      goto out_unlock;
   
    break;

  default:;
  #ifdef DEBUG
    fprintf(stderr,"thread_semctl:Invalid semctl cmd (%d)\n",cmd);
   #endif
  }
 error = 0; //successful
 out_unlock:
  unlock(); //unlock the access
 return error;
}   

static int alloc_undo(struct t_sem_array *sem_array,struct t_sem_undo **undo) {
  struct t_sem_undo *sem_undo;
  int error = -1;
  if(! (sem_undo = ALLOC_MEM(sem_undo,1,sizeof(short) * sem_array->nsems) ) ) {
 #ifdef DEBUG
    fprintf(stderr,"Error in allocating an undo structure:\n");
 #endif
    goto out;
  }
  sem_undo->semadj = (short *) ( sem_undo + 1);
  sem_undo->id = sem_array->id; //store the id to which it belongs
  //set up the array links
  sem_undo->array_next = sem_array->t_sem_undo;
  sem_array->t_sem_undo = sem_undo;
  //set up the process links
  sem_undo->proc_next = current->sem_undo;
  current->sem_undo = sem_undo;
  *undo = sem_undo;
  error = 0;
 out:
  return error;
}

static struct t_sem_undo *free_undo(struct t_sem_undo *sem_undo,struct t_sem_undo *head) {
  struct t_sem_undo **ptr;
  struct t_sem_undo *traverse;
  for(ptr = &head; (traverse = *ptr); ptr = &(*ptr)->proc_next) 
    if(traverse == sem_undo) { //remove the struct.
      traverse = traverse->proc_next; 
      *ptr = traverse;
      free((void*)sem_undo);
      return traverse;
    }
 #ifdef DEBUG 
  fprintf(stderr,"free_undo:Undo struct not found for Thread(%d)\n",current->pid);
 #endif
  return sem_undo->proc_next;
}

/* Thread Semaphore operations.
 */

int thread_semop(int id,struct t_semop *semop,int nsops) {
  struct t_sem_array *sem_array;
  struct t_sem_queue sem_queue;
  struct t_semop *semop_ptr;
  struct t_sem_undo *sem_undo ;
  int error = -1,undos = 0,alter = 0;
  if(nsops > T_SEMMOP) 
    goto out;
  if(! (sem_array = sem_get_array(id) ) ) {
    unlock();
    goto out;
  }
  sem_array->otime = time(NULL);

  for(semop_ptr = semop; semop_ptr < semop + nsops ; ++semop_ptr) {
    if(semop_ptr->sem_num >= sem_array->nsems) {
      unlock();
      goto out;
    }
    
    if(semop_ptr->sem_flag & T_SEM_UNDO) ++undos;

    if(semop_ptr->sem_op && !alter) alter = 1; //switch on the alter flag
 
  }
  
  //check for the undo struct corresponding to this array
  if(undos) {
    struct t_sem_undo *ptr = current->sem_undo;
    while(ptr) { 
      if(ptr->id == sem_array->id) 
	break;
      if(ptr->id == -1) { //invalidated undo struct
        ptr = free_undo(ptr,current->sem_undo);
        continue;
      }
      ptr = ptr->proc_next;
    }
    sem_undo = ptr;
    if(! sem_undo) 
      {
	if(alloc_undo(sem_array,&sem_undo) < 0) 
	  goto out_unlock; //unable to allocate undo struct
      }
  }else{
     sem_undo = NULL; //no undo struct
  }

 //now try to perform the operation on the semaphore
  error = try_atomic_semop(sem_array,semop,nsops,sem_undo,0,current->pid);
  if(error <= 0) 
    goto out_unlock; //the call went through
  //else we are going to sleep on this semaphore array
   current->sleeper = &sem_queue; 
   sem_queue.next = NULL;
   sem_queue.prev = NULL;
   sem_queue.semop =    semop;
   sem_queue.sem_array = sem_array;
   sem_queue.nsops =    nsops;
   sem_queue.sem_undo = sem_undo;
   sem_queue.id       = sem_array->id;
   sem_queue.pid      = current->pid;
   sem_queue.alter    = alter;
   if(alter) 
     append_to_queue(&sem_queue,sem_array);
   else
     prepend_to_queue(&sem_queue,sem_array);
   for(;;) {
    sem_queue.status   = -EINTR;
    sem_queue.sleeper  = current; //set up the task
    current->state = TASK_INTERRUPTIBLE;
    list_del(&current->run_queue); //remove from the run queue
    unlock(); //unlock the semaphore
    #ifdef DEBUG
     fprintf(stderr,"Semop: Scheduling out Thread (%d)\n",current->pid);
   #endif

    schedule(); //call the scheduler

   #ifdef DEBUG
     fprintf(stderr,"Semop: Bringing in Thread (%d)\n",current->pid);
   #endif

    if(sem_queue.status == -EIDRM) { //semaphore array has been removed
     error = -EIDRM;
     current->sleeper = NULL;
     goto out;
   }
   error = -1;
   if(!( sem_array = sem_get_array(id) ) ) {
     current->sleeper = NULL;
     unlock(); 
     goto out;
   }

   if((error = sem_queue.status) == 1) { //indication to try again
     error = try_atomic_semop(sem_array,semop,nsops,sem_undo,0,current->pid);
     if(error <= 0 ) 
       break;
   } else {
     current->sleeper = NULL;
     goto out_unlock; //unlock
   }
   }
   current->sleeper = NULL;
   remove_from_queue(&sem_queue,sem_array);
 out_unlock:
   if(alter) //update the queue to wake up dependencies
     {
 #ifdef DEBUG
       fprintf(stderr,"Updating the queue:\n");
#endif
     update_queue(sem_array);
     }
   unlock();
 
 out:
   return error;
} 

/* thread_sem_exit:
 This routine is called by the reaper of the thread on a thread exit
 to undo the stuffs that they thread would have done in order to enable 
 other guys to work properly.
*/
        
void thread_sem_exit() { 
  struct t_sem_array *sem_array;
  struct t_sem_queue *sem_queue;
  struct t_sem_undo *sem_undo,**undo,*u,**un;
  lock(); //take the lock
  if((sem_queue = current->sleeper) ) {
    sem_array = ipc_get(sem_queue->id); 
    if(sem_array) 
      remove_from_queue(sem_queue,sem_array);
    current->sleeper = NULL;
  }
 
  for(undo = &current->sem_undo; (sem_undo = *undo) ; *undo = sem_undo->proc_next , free( (void *)sem_undo) ) {
    if(sem_undo->id == -1) 
      goto next_entry; 
   
    sem_array = ipc_get(sem_undo->id);
    if(! sem_array) 
      goto next_entry;
    
    for(un = &sem_array->t_sem_undo; (u = *un) ; un = &u->array_next ) 
      if( u == sem_undo) 
        goto found;

    #if 1
     fprintf(stderr,"thread_sem_exit:Undo structure of thread (%d) not found in the sem array with id (%d):\n",current->pid,sem_array->id);
    #endif
     goto next_entry;
  found: //make the changes in the array
     *un = u->array_next;
     {
       //now undo the changes made by this thread
       struct t_sem *sem ;
       int i;
       for(i=0;i<sem_array->nsems;++i) {
         sem = sem_array->t_sem_base + i;
         sem->pid = current->pid;
         sem_array->otime = time(NULL);
         sem->val += sem_undo->semadj[i];
         if(sem->val < 0) { //shouldnt happen
         #if 1
	   fprintf(stderr,"thread_sem_exit:Resetting semval to 0:\n");
         #endif
	   sem->val = 0;
	 }
       }
     }
     update_queue(sem_array); //maybe some guys are waiting for this change
  next_entry:;
  }
  current->sem_undo = NULL;
  unlock();

} 

  
  
    
 
    

  
 

   







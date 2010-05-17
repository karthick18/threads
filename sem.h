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
/* An implementation of semaphores for Threads.
   Works in the same way as Kernel Semaphores 
*/

#ifndef _SEM_H
#define _SEM_H

#define T_SEMMSL  0x20    //max. number of semaphores allocated 
#define T_SEMMID  0x20  //total max. ids of semaphores possible
#define T_SEMMVAL 16383 //max. val of a semaphore
#define T_SEMMNO  0x40  //max no. of semaphores that can be created per array
#define T_SEMMOP  0x20   //total max. number of operations possible at a time
#define T_SEM_UNDO   0x1    //Sem Undo flag for the semaphore.
#define T_IPC_CREAT 0x2 //create operation on a semaphore

/* Semaphore operation in progress or lock ipc sem. operations */

#define T_SEMOP        0x1
#define IS_SEMOP(ipcids)   ( ( ( (ipcids).semop_ind) & T_SEMOP ) != 0 )
#define CLEAR_SEMOP(ipcids) ( ( (ipcids).semop_ind &= ~T_SEMOP) )
#define SET_SEMOP(ipcids)   ( ( (ipcids).semop_ind |= T_SEMOP) )

/* Semaphore control operations */
#define T_SET_VAL    0x1 
#define T_IPC_RMID  0x2

#include <time.h>

/*Now define the system wide IPC structure pertaining to semaphores.*/

struct t_ipcids {
  int max_id; //max id of the semaphore
  int entries; //nr. of entries in the semaphore array
  unsigned char semop_ind; //this field gets set when a semaphore operation is in progress.This is a field to indicate that no scheduling should take place while the operation is in progress.
  struct t_sem_array **t_sem_array; //array of semaphore entries
};

/*Define the semaphore structure per semaphore array or id.*/
struct t_sem_array {
  int key; //key corresponding to this array
  int id; //id corresponding to the key of this array 
  int pid; //pid of the last thread acting on this semaphore array
  time_t ctime; //creation time of this semaphore array
  time_t otime;//last operation time on this semaphore array
  int nsems; //number of semaphores in an array on which the operations are to be performed
  struct t_sem *t_sem_base; //base of an array of semaphores
  struct t_sem_queue *first; //first guy in the queue for this array
  struct t_sem_queue **last; //last guy in the sleeping queue for this array
  struct t_sem_undo *t_sem_undo; //undo structures this array

};

/* Queue for the array.This queue gets active when a thread is to sleep on this queue,when a semaphore operation fails.*/
struct task_struct; //forward declaration.Strange C thing
struct t_sem_queue {
  struct task_struct *sleeper;//sleeper task on this queue
  struct t_sem_queue *next;
  struct t_sem_queue **prev;
  struct t_sem_array *sem_array; //the array corresponding to this queue
  struct t_sem_undo *sem_undo ;//the undo structure corresponding to this queue
  struct t_semop *semop; //the operation structure corresponding to this queue
  int id; //id of the semaphore.
  int pid; //pid of the thread sleeping
  int nsops; //number of semaphore operations corresponding to this queue
  int alter; //if the operation involves an altering of a semaphore value,then this field indicates that the operation should be undone once a sleeping thread on this queue wakes up.
  int status; //status of the queue.This status indicates the state of the semaphore identified by the id. 
};

/* The semaphore structure.
 Note that there can be more than one operation possible on a semaphore id.
 The operations are performed as they are in the Linux Kernel.
*/
struct t_sem {
  short val; //value of the semaphore
  int pid; //pid of the last thread acting on the semaphore
}; 

//Undo structure on the semaphore.
struct t_sem_undo {
 int id;
 struct t_sem_undo *proc_next; //next undo structure for this process
 struct t_sem_undo *array_next; //next undo structure this semaphore array identified by an id
 short *semadj; //adjustment values for the semaphore
};

/* SEmaphore operation structure */

struct t_semop {
int sem_num; //the index of the semaphore on which to perform the operation
int sem_op; //the operation to be performed on the semaphore
int sem_flag; //the flags on the Semaphore.Only T_SEMUNDO is used now
};

/* Semaphore structure used for semaphore control operations on a set */
union t_semun {
 int sem_val; //value of the semaphore
 short *sem_values; //GET_ALL,SET_ALL-Unused
};

extern struct t_ipcids sem_ipcids; //semaphore instance of IPC
extern void ipc_init();//initialise the IPC section
extern void ipc_release(); //release the IPC section
extern struct t_sem_array *ipc_free(int); //free up the sem. identified by id
extern struct t_sem_array *ipc_alloc(struct t_sem_array *);
extern struct t_sem_array *ipc_find(int);
extern struct t_sem_array *ipc_get(int);
extern void sem_lock(void);
extern void sem_unlock(void);
/* User interface routines*/
extern int thread_sem_create(int,int,int);
extern int thread_semctl(int,int,int,union t_semun *);
extern int thread_semop(int,struct t_semop *,int);
extern void thread_sem_exit(void);
#endif


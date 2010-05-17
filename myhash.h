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
;Define the hash table routines for the tasklets
;
;
;
*/

#ifndef _MYHASH_H
#define _MYHASH_H


#define HASH_TABLE_SIZE  ( 1 << 12) 

#define GET_HASH_INDEX(pid)   ( ( (pid) ^ MAX_PID) & (HASH_TABLE_SIZE - 1) )


extern struct task_struct *task_hash_table[HASH_TABLE_SIZE];

/*Link the task struct into the hash table */

static __inline__ void task_hash_add(struct task_struct *task) {
  struct task_struct **hash_table = &task_hash_table[GET_HASH_INDEX(task->pid)]; //get the index into the table
  
  if( (task->h_next = *hash_table) ) {
    (*hash_table)->h_prev = &task->h_next; 
  }
  *hash_table = task; //write the task into the hash index
  task->h_prev = hash_table;
  return ;
}

/*Unhash an entry */

static __inline__ void task_hash_del(struct task_struct *task) {

  if(task->h_next) {
    task->h_next->h_prev = task->h_prev; //reset the next guys prev. entry
  }
  *task->h_prev  = task->h_next ; //write out the next entry in the previous guys address
  return ;
}

/*Find an entry by pid */

static __inline__ struct task_struct *task_hash_find(int pid) {
  struct task_struct *head = task_hash_table[GET_HASH_INDEX(pid)];
  struct task_struct *ptr;
  for(ptr = head; ptr && ptr->pid != pid; ptr = ptr->h_next) ;
  return ptr;  
}

#endif
 

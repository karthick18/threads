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
; Utility routines used by some common fragments of the Code
*/

#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<stdarg.h>
#include "util.h"
#include "sem.h"

#define BUFFER 1024

/*Not used that much now as it uses a lot of stack.*/
void message(int exit_status,FILE *fptr,const char *fmt,...) {
    char buffer[BUFFER+1];
    va_list ptr;
    va_start(ptr,fmt);
    vsnprintf(buffer,BUFFER,fmt,ptr);
    va_end(ptr);
    fprintf(stderr,"%s\n",buffer);
    if(exit_status) 
        exit(exit_status);
    return ;
}

/* 
  Lock the access to the semaphore section of threads.
  This is needed to avoid context switches in the midst of a semaphore operation.
*/

void sem_lock() { 
  SET_SEMOP(sem_ipcids); 
}

/*Unlock the access to the semaphore section of threads.*/
void sem_unlock() {
  CLEAR_SEMOP(sem_ipcids);
}

/* Initialise the semaphore section for threads. */

void sem_init() {
  sem_ipcids.max_id = -1;
  sem_ipcids.entries = 0;//number of semaphore entries
  sem_ipcids.semop_ind = 0; //clear the semop indicator bit
  sem_ipcids.t_sem_array = ALLOC_MEM(sem_ipcids.t_sem_array,T_SEMMSL,0);
  return ;
}

/* Initialise the IPC section for threads.*/

void ipc_init() {
  sem_init(); //initialise the semaphore section for threads
}

/* Note: There is no grow array needed as we dont allow greater than SEMMSL number of semaphores.Allocate an id for a sem array.*/

struct t_sem_array *ipc_alloc(struct t_sem_array *sem_array) {
  int i;
  if(sem_ipcids.entries >= T_SEMMSL ) 
    goto out; //failed to allocate an entry
 
  for(i=0;i<=sem_ipcids.max_id;++i) { 
    if(! sem_ipcids.t_sem_array[i] ) 
      goto found;
  }
if(i >= T_SEMMSL) 
  goto out; //id not found
 found: //id has been found
 ++sem_ipcids.entries; //increment the entries
 if(i > sem_ipcids.max_id) sem_ipcids.max_id = i; //set the max.id
 sem_ipcids.t_sem_array[i] = sem_array;
 sem_array->id = i; //set the id of the semaphore
 return sem_array;  
 out: 
 return NULL; //id not found
}

struct t_sem_array *ipc_find(int key) {
  int i;
  for(i=0;i<=sem_ipcids.max_id;++i) {
    if(sem_ipcids.t_sem_array[i] && sem_ipcids.t_sem_array[i]->key == key) 
      goto found;
  }
 goto out; //not found
 found :
    return sem_ipcids.t_sem_array[i];
 out:
 return NULL;
}

/* Free up an ipc resource */

struct t_sem_array *ipc_free(int id) { 
  int i;
  struct t_sem_array *ptr;
  for(i=0;i<=sem_ipcids.max_id; ++i) {
    if(sem_ipcids.t_sem_array[i] && sem_ipcids.t_sem_array[i]->id == id) {
      sem_ipcids.t_sem_array[i]->key = -1; //invalidate the key
      goto found;
    }
  }
  goto out; //not found
 found:
  ptr = sem_ipcids.t_sem_array[i];
  sem_ipcids.t_sem_array[i] = NULL; 
  --sem_ipcids.entries; //decrement the no. of entries
  if( i == sem_ipcids.max_id) { 
    //readjust the max id of the semaphore
    do { 
      --sem_ipcids.max_id ;
      if(sem_ipcids.max_id < 0 ) break;
    }while(! sem_ipcids.t_sem_array[sem_ipcids.max_id] );
  }
  return ptr;
  out :
    return NULL;
}

struct t_sem_array *ipc_get(int id) {
  int i;
  for(i=0;i<=sem_ipcids.max_id;++i) {
    if(sem_ipcids.t_sem_array[i] && sem_ipcids.t_sem_array[i]->id == id) 
      goto found;
  }
  goto out;
  found :
    return sem_ipcids.t_sem_array[i];
 out:
  return NULL;
}

/* Release the t_sem_array for the IPC */

void ipc_release() {
  if(sem_ipcids.t_sem_array)  
    free((void*)sem_ipcids.t_sem_array);
  return ;
}



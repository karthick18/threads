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
;    Semaphore usage implementation for an application
*/

#include<stdio.h>
#include "mylist.h"
#include "task.h"
#include "util.h"
#include "sem.h"

struct t_semop lock_semop[] = {
  {0,0,0},
  {0,1,T_SEM_UNDO},
};

struct t_semop unlock_semop[] = { 
  {0,-1,T_SEM_UNDO},
}; 

int __sem_create(int key,int nsems,int flag) {
  return thread_sem_create(key,nsems,flag);
}

int __semop(int semid,struct t_semop *semop,int nsops) {
  return thread_semop(semid,semop,nsops); 
}

int __semctl(int semid,int semnum,int cmd,union t_semun *ptr) {
  return thread_semctl(semid,semnum,cmd,ptr);
}

/* Take the lock on the semphore */
int down_sem(int semid) {
  return __semop(semid,lock_semop,2);
}

/*Release the lock on the semaphore*/

int up_sem(int semid) {  
  return __semop(semid,unlock_semop,1);
}


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
;Implementation of wait queues for the threading library 
*/

#ifndef _WAIT_H
#define _WAIT_H

#include "mylist.h"

struct task_struct ; //forward declaration.Strange "C" thing.

/*define a wait queue head structure */

struct wait_queue_head {
  struct list_head wait_queue_head;
};

/*Define a wait queue structure */

struct wait_queue {
  struct list_head wait_queue_head;
  struct task_struct *task; //task to add in the wait queue
};

extern void init_waitqueue_entry(struct wait_queue *,struct task_struct *);
extern void init_waitqueue_head(struct wait_queue_head *);
extern void add_waitqueue(struct wait_queue *,struct wait_queue_head *);
extern void remove_waitqueue(struct wait_queue*);
extern void sleep_on(struct wait_queue *,struct wait_queue_head *);
extern void interruptible_sleep_on(struct wait_queue*,struct wait_queue_head *);
extern void wake_up(struct wait_queue_head *); 
extern void wake_up_interruptible(struct wait_queue_head *);

#endif

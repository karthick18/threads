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
#ifndef _TIMER_H
#define _TIMER_H

struct timer_struct {
  struct list_head timer;//list head of timers for the threads
  unsigned long expiry; //expiry of the timer
  void (*routine)(void *); //expiry routine callback
  void *arg; //argument for the timer
  /*Each process when it exits,just removes the timer from the system,so as to avoid stale crashes with dereferencing the current pointer */

  int timer_id; 
};

extern void init_timer(struct timer_struct *);
extern void set_timer(struct timer_struct *,unsigned long,void (*)(void *),void *,int );
extern void add_timer(struct timer_struct *);
extern void del_timer(struct timer_struct *);
extern int is_timer(void);
extern void run_timer(void);
extern int interruptible_sleep_on_timeout(unsigned long);
extern void release_thread_timer(int);
#endif

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
#ifndef _SCHED_H

#define _SCHED_H

/* Setup a 100 ms interval,so that we can get a hit rate of 10 threads per second,which should be pretty good anyway */

#define TIMER_INTERVAL ( 10000 )  

#define MILLISECS_TIMER_INTERVAL ( TIMER_INTERVAL / 1000 )
#define MILLISECS_PER_TICK 10 //usually 10 millisecs per tick

#define SWITCHES_PER_SEC (1000/MILLISECS_TIMER_INTERVAL)

#define START_PRIORITY   1 //100ms

#define DEFAULT_NICE_LEVEL  10

#define MAX_NICE_LEVEL     -19

#define MIN_NICE_LEVEL      20

#define JIFFIES_LIMIT       5 //just a cutoff to reduce the thread counter val.

extern void schedule(void);

extern struct timeval sched_timer;

extern void initialise_timer(void);
extern void stop_timer(void); //stop the timer
extern void start_timer(void); //start the timer
extern void timer_interrupt(int); //SIGPROF handler
extern int set_nice_level(int,int);
struct task_struct; 
extern void switch_to(struct task_struct *);
extern void thread_reaper(void) asm("thread_reaper");
extern void task_del(struct task_struct *);
extern void release_thread_timer(int);
extern struct list_head init_run_queue;
extern struct list_head init_task;
extern struct task_struct *current; //current task
extern struct task_struct *reaper;
extern volatile unsigned long jiffies;
#endif

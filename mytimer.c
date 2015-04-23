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
 Timer routines for the thread. The timer gets interrupted at a rate of 100ms,thus giving us a context switch overhead of 10 threads per sec.
*/

#include<stdio.h>
#include<signal.h>
#include<stdlib.h>
#include<sys/time.h>
#include<time.h>
#include <unistd.h>
#include "mylist.h"
#include "task.h"
#include "sched.h"
#include "util.h"
#include "timer.h"
#include "wait.h"
#include "sem.h"
#define MYTIMER ITIMER_PROF

/* 
  Setup a 100 ms interval,so that we can get a hit rate of 10 threads per second,which should be pretty good anyway.
  ITIMER_PROF timer is used as that includes the time spent by the thread
  in the user space and kernel space.
*/

static int flag ;

/*The timer for the scheduler.*/

struct timeval sched_timer = { 0 , TIMER_INTERVAL } ;

/* Pass in a timeval struct,and call setitimer to setup the timer.*/

void start_timer(void) { 
   struct itimerval timer; 
   if(!flag) {
     flag = 1;
     initialise_timer();
     timer.it_interval = sched_timer; //copy the timeval for the next timer to be triggered
    timer.it_value    = sched_timer; //this value would be reset to the next value on a timer expiry
   if(setitimer(MYTIMER,&timer,NULL) < 0 ) {
    message(1,stderr,"Error in setting up the timer.Exiting...");
   } //dont have to set the timer as its already set with an interval.
   }
  return ;
}

void stop_timer(void) {
     struct timeval tv = { 0, 0 }; //zero up the timeval struct
     struct itimerval timer = { tv, tv } ; //load up the itimer with a zero timer
    if(setitimer(MYTIMER,&timer,NULL) < 0) {
       fprintf(stderr,"Error in Setting up the Timer:\n");
       exit(1);
     }
    flag = 0; 
}

/* Timer has expired. Reschedule. */

void timer_interrupt(int signo) { //profile handler
#ifdef DEBUG
  fprintf(stderr,"Timer interrupt (%d) received:\n",signo);
#endif
  ++jiffies; //increment the jiffies count (100ms latency)
  run_timer(); //run the system timers
  if(current) {
    ++current->ticks; //increment the ticks
    --current->counter; //decrement the time slice
    if(current->counter < 0) current->counter = 0; //shouldnt happen
  }
#if 0
  if(current && current->counter) 
    --current->counter; //decrement the time slice of the thread
  if(! current->counter) {
    //if the time slice of the thread gets finished,reschedule
   schedule(); //run the schduler to pick up the next thread  
  } 
#endif
  if(! IS_SEMOP(sem_ipcids) )  //call the scheduler when the sem isnt locked
   schedule(); //get on to the next thread on each SIGPROF signal
  /*else continue with the same thread as its time slice isnt over.*/
}

static __inline__ void setup_signal(int signum,void (*handler)(int) ) { 
  struct sigaction sigact ; 
  sigset_t set; //the signal set
  sigact.sa_handler = handler; //sigprof handler
  sigfillset(&set); //by default block all signals
  sigdelset(&set,signum); //dont block signum.Allow signum only
  sigact.sa_mask = set; //copy the signal mask
  sigact.sa_flags =  SA_NODEFER; //amounts to a non maskable interrupt
  if( sigaction(signum,&sigact,NULL) < 0) {
    fprintf(stderr,"Failed to initialise the Signal (%d)\n",signum);
    exit(1);
  }
  return ;
}

/* Setup the timer components correctly.Handle to the timer.*/

void initialise_timer(void) {
   setup_signal(SIGPROF,timer_interrupt);
}






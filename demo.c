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
;    Sample user app. using the static thread libraries.
;
*/

#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<fcntl.h>
#include<sys/errno.h>
#include "mylist.h"
#include "task.h"
#include "myhash.h"
#include "util.h"
#include "sched.h"
#include "wait.h"
#include "timer.h"
#include "sem.h"

//#define fprintf(fileptr,fmt,args...) message(0,fileptr,fmt, ## args)

#define NR(a)  ( sizeof((a))/sizeof((a)[0]) )
#define MAX_LIMIT 50000

/*Define the wrappers for interruptible sleep on ,wakeups,and non interruptible sleep ons and wakeups */

#define INTERRUPTIBLE_SLEEP_ON(wait_queue_head) do {            \
        static struct wait_queue wait_queue;                    \
        init_waitqueue_head(&wait_queue_head);                  \
        interruptible_sleep_on(&wait_queue,&wait_queue_head);   \
    }while(0)

#define WAKE_UP_INTERRUPTIBLE(wait_queue_head) do { \
        wake_up_interruptible(&wait_queue_head);    \
    }while(0)

#define SLEEP_ON(wait_queue_head)  do {         \
        static struct wait_queue wait_queue;    \
        init_waitqueue_head(&wait_queue_head);  \
        sleep_on(&wait_queue,&wait_queue_head); \
    }while(0)

#define WAKE_UP(wait_queue_head) do {           \
        wake_up(&wait_queue_head);              \
    }while(0)

static void mythread(void *arg) {
    int i = 0;
    int tid = get_tid();
    while(i++ < MAX_LIMIT) 
        fprintf(stderr, "Thread %d: i = %d\n", tid, i);
}

int main(int argc,char **argv) {
    static int *thread_pid = NULL;
    int pid;
    int i,nr = 0;
    if(argc > 1 ) {
        nr = atoi(argv[1]);
    }
    if(nr <= 0) {
        nr = 10;
    }
    thread_pid = calloc(nr, sizeof(*thread_pid));
    for(i=0;i<nr;++i) { //create each thread
        if( (thread_pid[i] = create_thread(mythread, NULL) ) < 0)
            goto out_error;
        if(run_thread(thread_pid[i]) < 0 )  //add the threads to the run_queue
            goto out_error;
    }
#if 1
    if(execute_threads() < 0) 
        goto out_error;
    return 0;
#endif
    if(! (pid = fork()) ) {
        if(execute_threads() < 0) 
            goto out_error;
    }else if(pid > 0) {
        restart:
        while(wait((int*)0) != pid);
        if(errno == EINTR) goto restart;
        message(0,stderr,"Back after thread execution:\n");
    } else {
        message(0,stderr,"Error forking:\n");
        goto out_error;
    }
    return 0;
    out_error:
    message(0,stderr,"Failed in thread creation:\n");
    return 1;
}
    



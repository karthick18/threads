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

#define FILENAME(file)   file "1.txt"

#define NR(a)  ( sizeof((a))/sizeof((a)[0]) )
#define MAX_LIMIT 50000

typedef void (*threadptr)(void *); 

static void mythread1(void *);
static void mythread2(void *); //declare the second thread
static void mythread3(void *);
static void mythread4(void *);

static threadptr thread_functions[] = { mythread1,mythread2,mythread3,mythread4 };
static char *thread_arguments[]     = { "karthick","radha","linus","alan"  };

extern int __sem_create(int,int,int);
extern int  down_sem(int);
extern int up_sem(int);

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

static struct wait_queue_head thread1_wait_queue;
int shared = 0;

void mythread1(void *arg) {
    char *str = (char *)arg;
    int fd =-1;
    (void)str;
    if(( fd = open(FILENAME("file"),O_RDONLY) ) < 0) {
        fprintf(stderr,"Unable to open file %s",FILENAME("file") );
        goto out;
    }
    {
        char buffer[BUFFER+1]; //buffer for reading the file
        int bytes;
        while( (bytes = read(fd,buffer,BUFFER) ) ) {
            buffer[bytes] = 0;
            fprintf(stdout,"%s",buffer);
            fflush(stdout);
        }
    }
#if 0
    (void)interruptible_sleep_on_timeout(5); //wait for 5 ticks or 500 ms.
#else
    INTERRUPTIBLE_SLEEP_ON(thread1_wait_queue); //queue up the guy in the wait queue.Sleep till someone wakes us up  
    fprintf(stderr,"Thread 1 back after a sleep on:\n");
#endif

    out:
    fprintf(stderr,"\nThread 1: Exiting..\n");
    return ;
}


void mythread2(void *arg) {
    char *str = (char *)arg;
    int i=0,id;
    struct list_head *head = &thread1_wait_queue.wait_queue_head;
    if((id = __sem_create(1,1,T_IPC_CREAT) ) < 0) {
        fprintf(stderr,"Error in creating the semaphore:\n");
        id =-1;
        goto loop_start;
    }else {
        fprintf(stderr,"id=%d\n",id);
    }
  
    (void)down_sem(id); //take the semaphore
    /*start of the critical region*/
    ++shared;
    loop_start:
    while(i++ < MAX_LIMIT) {
        fprintf(stderr,"Thread 2:Arg=%s,i=%d\n",str,i);
    }
#if 1
    if(!LIST_EMPTY(head) ){
        fprintf(stderr,"Waking up the guys in the wait queue:\n");
        WAKE_UP_INTERRUPTIBLE(thread1_wait_queue); //wake up the guy in thread 1s wait queue
    }
#endif
    fprintf(stderr,"Thread 2: Exiting...\n");
    if(id >= 0){
        /* 0 this operation to test to see if the undo works*/
#if 0
        (void) up_sem(id); //release the semaphore 
#endif
    }
    /*end of the critical section*/
    return ;
}
void mythread3(void *arg) {
    char *str= (char *)arg;
    int i= 0,id;
    if((id = __sem_create(1,1,0) ) < 0) { 
        fprintf(stderr,"mythread3: Unable to access the semaphore:\n");
        id = -1;
        goto loop_start;
    }else {
        fprintf(stderr,"id=%d\n",id);
    }
    /* Start of the critical region*/

    (void)down_sem(id);
    ++shared;

    loop_start:
    while(i++ < MAX_LIMIT) { 
        fprintf(stderr,"Thread 3:Arg=%s,i=%d\n",str,i);
    }
    fprintf(stderr,"Thread 3: Exiting..\n");
    if(id >= 0) 
        (void) up_sem(id);
    /*end of the critical region*/  
    return;
}

void mythread4(void *arg) {
    char *str= (char *)arg;
    int i= 0;
    while(i++ < MAX_LIMIT) { 
        fprintf(stderr,"Thread 4:Arg=%s,i=%d\n",str,i);
    }

    fprintf(stderr,"Thread 4:Exiting..\n");
    return;
}

int main(int argc,char **argv) {
    static int thread_pid[NR(thread_functions)]; 
    int pid;
    int i,nr = NR(thread_functions);
    for(i=0;i<nr;++i) { //create each thread
        if( (thread_pid[i] = create_thread(thread_functions[i],(void *)thread_arguments[i]) ) < 0) 
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
    



##Makefile for the Multi Threading Library,which seems to be working okay.
CC=gcc
DEBUG=-g #-DDEBUG
CFLAGS=$(DEBUG) -Wall $(DEBUG)
UNAME := $(shell uname)
TARGET=libmythread.a
AR=ar
RANLIB=ranlib
MYLIB=mythread
LIBDIR=/usr/lib
INSTALL=install
USER_APP=main_prog
CPP = cpp -P
OBJECTS = task.o save_asm.o mytimer.o timer.o wait.o util.o sched.o sem.o main_sem.o
AS=as
ASFLAGS=-g
ifeq ("$(UNAME)", "Darwin")
	CFLAGS += -mdynamic-no-pic
endif
#ifeq ("$(UNAME)", "Linux")
#	LDFLAGS += -static
#endif

all:.depend $(TARGET) $(USER_APP) 

$(TARGET): $(OBJECTS)
	$(AR) cr $@ $^;\
	$(RANLIB) $@

install:
	$(INSTALL) -d $(LIBDIR)
	$(INSTALL) -m 0755 -C  $(TARGET) $(LIBDIR)/$(TARGET)

task.o: task.c
	$(CC) $(CFLAGS) -c $^ -o $@

save_asm.o: save.s
	$(AS) $(ASFLAGS) -o $@ $^

#save.i: save.s
#	$(CPP) -o $@ $^

mytimer.o:mytimer.c
	$(CC) $(CFLAGS) -c $^ -o $@

util.o: util.c
	$(CC) $(CFLAGS) -c $^ -o $@

.dep .depend:
	$(CC) $(CFLAGS) -M *.c > $@

sched.o: sched.c
	$(CC) $(CFLAGS) -c $^ -o $@

main_prog: main.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -L./ -l$(MYLIB)

main.o: main.c
	$(CC) $(CFLAGS) -c $^ -o $@

wait.o: wait.c
	$(CC) $(CFLAGS) -c $^ -o $@

timer.o:timer.c
	$(CC) $(CFLAGS) -c $^ -o $@

sem.o: sem.c
	$(CC) $(CFLAGS) -c $^ -o $@

main_sem.o: main_sem.c
	$(CC) $(CFLAGS) -c $^ -o $@

clean:
	rm -f *.o *~ *.i .dep*


ifeq (.depend,$(wildcard .depend) )
 include .depend
endif

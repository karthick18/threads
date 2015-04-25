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
USER_APP_SRCS := main.c demo.c
USER_APP_OBJS := $(USER_APP_SRCS%.c=.o)
USER_APP=main_prog demo_prog
LIB_SRCS := $(filter-out $(USER_APP_SRCS), $(wildcard *.c))
LIB_SRCS += $(wildcard *.s)
LIB_OBJS := $(LIB_SRCS:.c=.o)
LIB_OBJS += $(LIB_SRCS:.s=.o)
AS=as
ASFLAGS=-g
ifeq ("$(UNAME)", "Darwin")
	CFLAGS += -mdynamic-no-pic
endif
#ifeq ("$(UNAME)", "Linux")
#	LDFLAGS += -static
#endif

all:.depend $(TARGET) $(USER_APP) 

$(TARGET): $(LIB_OBJS)
	$(AR) cr $@ $^;\
	$(RANLIB) $@

install:
	$(INSTALL) -d $(LIBDIR)
	$(INSTALL) -m 0755 -C  $(TARGET) $(LIBDIR)/$(TARGET)

%.o:%.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o:%.s
	$(AS) $(ASFLAGS) -o $@ $<

.dep .depend:
	$(CC) $(CFLAGS) -M *.c > $@

main_prog: main.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -L./ -l$(MYLIB)

demo_prog: demo.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -L./ -l$(MYLIB)

clean:
	rm -f *.o *~ *.i .dep* $(USER_APP)


ifeq (.depend,$(wildcard .depend) )
 include .depend
endif

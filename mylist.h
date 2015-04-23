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
#ifndef _MYLIST_H
#define _MYLIST_H

struct list_head {
  struct list_head *next,*prev;
};

#define DECLARE_LIST_HEAD(head)  \
struct list_head head = { &(head),&(head) };


#define LIST_EMPTY(head)   ( (head) == (head)->next )

#define INIT_LIST_HEAD(ptr) \
do { \
(ptr)->next = ptr ; \
(ptr)->prev = ptr ; \
}while(0)


/*Add an element between a and b */

#define __list_add(e,a,b) \
do { \
(e)->next = b;\
(e)->prev = a;\
(a)->next = e;\
(b)->prev = e;\
}while(0)

/*Arrange after element deletion */

#define __list_del(a,b) \
do { \
(a)->next = b;\
(b)->prev = a;\
}while(0)

/*Add an element to the head */

static __inline__ void list_add(struct list_head *e,struct list_head *h) {
  struct list_head *n = h->next;
  __list_add(e,h,n);
}

static __inline__ void list_add_tail(struct list_head *e,struct list_head *h) {
  struct list_head *p = h->prev;
  __list_add(e,p,h); 
}

static __inline__ void list_del(struct list_head *e) {
  struct list_head *a = e->prev;
  struct list_head *b = e->next;
  __list_del(a,b); //rearrange 
}

static __inline__ void list_del_init(struct list_head *e) {
  list_del(e); 
  INIT_LIST_HEAD(e); //reinitialise the list head
  return ;
}

/*
  Splice up 2 lists:
  That is join the contents of the second list
  after the end of the first list into the first list.
  Concatenate the first list.
*/

static __inline__ void list_splice(struct list_head *a,struct list_head *b) {
  if(! LIST_EMPTY(b) && ! LIST_EMPTY(a) ) {
    struct list_head *a_next = a->next; 
    struct list_head *b_first =b->next;
    struct list_head *b_last = b->prev;
  
    a->next = b_first; 
    b_first->prev = a;
    b_last->next = a_next;
    a_next->prev = b_last;

  }
  return ;
}

#define list_for_each_safe(pos, n, head) \
    for(pos = (head)->next, (n) = (pos)->next; (pos) && (pos) != head; (pos) = (n), n = (pos) ? (pos)->next : NULL)

#define list_for_each(pos,head) \
    for(pos = (head)->next; (pos) && (pos) != (head); pos = (pos)->next)

#define list_entry(list_head,typecast,member) \
(typecast *) ( (unsigned char *)(list_head) -  ( (unsigned long)&((typecast *)0)->member) )

 
#endif



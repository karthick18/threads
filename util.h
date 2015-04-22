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
#ifndef _UTIL_H
#define _UTIL_H


/* Allocate memory in a dynamic way */

#define ALLOC_MEM(ptr,nr,opt)  ({                                       \
            __typeof__(ptr) __temp = ( __typeof__(ptr) )malloc( (nr) * ( sizeof(*(ptr) ) + (opt) ) ); \
            if(__temp) {                                                \
                memset(__temp,0,( (nr) * ( sizeof(*(ptr) ) + (opt) ) ) ); \
            }                                                           \
            __temp;                                                     \
})

extern void message(int exit_status,FILE *,const char *fmt,...);

#endif

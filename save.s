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
.text
 .globl save_regs,restore_regs,restore_ret_regs,thread_reaper

save_regs:
        pushl %ebp
        movl %esp,%ebp
        pushl %ebx
        movl 8(%ebp),%ebx
        movl %eax,0(%ebx)
        movl %ecx,8(%ebx)
        movl %edx,12(%ebx)
        movl %esi,16(%ebx)
        movl %edi,20(%ebx)
        movl %cs, 24(%ebx)
        movl %ds, 28(%ebx)
        pushfl
        popl 44(%ebx)
        movl 4(%ebp),%ecx
        movl %ecx,40(%ebx)
        movl %ebx, %ecx
        popl 4(%ecx)
        popl 36(%ecx)
        movl 36(%ecx),%ebp
        movl %esp,32(%ecx)
        xorl %eax,%eax
        ret

restore_regs:
        movl %esp,%ebp
        movl 4(%ebp), %ebx
        movl 8(%ebx), %ecx
        movl 12(%ebx),%edx
        movl 16(%ebx),%esi
        movl 20(%ebx),%edi

        pushl 44(%ebx)
        popfl
        movl 40(%ebx),%eax
        movl %eax,0(%ebp)
        movl %ebx,%eax
        movl 4(%eax),%ebx
        movl 36(%eax),%ebp
        movl 32(%eax),%esp
        pushl 40(%eax)
        movl $1,%eax
        ret



restore_ret_regs:
        movl %esp,%ebp
        movl 4(%ebp), %ebx
        movl 8(%ebx), %ecx
        movl 12(%ebx),%edx
        movl 16(%ebx),%esi
        movl 20(%ebx),%edi
        pushl 44(%ebx)
        popfl
        movl %ebx,%eax
        movl 4(%eax),%ebx
        movl 32(%eax),%esp
        pushl 8(%ebp)
        movl 36(%eax),%ebp
        call *40(%eax)
        addl $4,%esp
        pushl $thread_reaper
        movl $1,%eax
        ret

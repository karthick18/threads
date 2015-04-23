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
        push %rbp
        mov %rsp,%rbp
        push %rbx
        mov %rdi,%rbx
        mov %rax,0(%rbx)
        mov %rcx,16(%rbx)
        mov %rdx,24(%rbx)
        mov %rsi,32(%rbx)
        mov %rdi,40(%rbx)
        mov %cs, 48(%rbx)
        mov %ds, 56(%rbx)
        mov %r8, 96(%rbx)
        mov %r9, 104(%rbx)
        mov %r10, 112(%rbx)
        mov %r11, 120(%rbx)
        mov %r12, 128(%rbx)
        mov %r13, 136(%rbx)
        mov %r14, 144(%rbx)
        mov %r15, 152(%rbx)
        pushf
        pop 88(%rbx)
        mov 8(%rbp),%rcx
        mov %rcx,80(%rbx)
        mov %rbx, %rcx
        pop 8(%rcx)
        pop 72(%rcx)
        mov 72(%rcx),%rbp
        mov %rsp,64(%rcx)
        xor %rax,%rax
        ret

restore_regs:
        mov %rsp,%rbp
        mov 8(%rdi), %rbx
        mov 16(%rdi), %rcx
        mov 24(%rdi),%rdx
        mov 32(%rdi),%rsi
        mov 96(%rdi),%r8
        mov 104(%rdi),%r9
        mov 112(%rdi),%r10
        mov 120(%rdi),%r11
        mov 128(%rdi),%r12
        mov 136(%rdi),%r13
        mov 144(%rdi),%r14
        mov 152(%rdi),%r15
        push 88(%rdi)
        popf
        mov 80(%rdi),%rax
        mov %rax,0(%rbp)
        mov %rdi,%rax
        mov 40(%rax),%rdi
        mov 72(%rax),%rbp
        mov 64(%rax),%rsp
        push 80(%rax)
        mov $1,%rax
        ret

restore_ret_regs:
        mov %rsp,%rbp
        mov 8(%rdi), %rbx
        mov 16(%rdi), %rcx
        mov 24(%rdi),%rdx
        mov 96(%rdi),%r8
        mov 104(%rdi),%r9
        mov 112(%rdi),%r10
        mov 120(%rdi),%r11
        mov 128(%rdi),%r12
        mov 136(%rdi),%r13
        mov 144(%rdi),%r14
        mov 152(%rdi),%r15
        push 88(%rdi)
        popf
        mov %rdi,%rax
        mov 64(%rax),%rsp
        push %rdi
        push %rsi
        mov 32(%rax), %rsi
        mov 72(%rax),%rbp
        pop %rdi
        push %rdi
        call *80(%rax)
        pop %rdi
        pop %rdi
        add $15, %rsp
        push thread_reaper@GOTPCREL(%rip)
        mov $1,%rax
        ret

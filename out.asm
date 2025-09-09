section .data
str_0: db 100,115,102,0
section .bss
num_buf: resb 20
section .text
global _start
_start:
    call main
    mov rax,60
    xor rdi,rdi
    syscall
add:
    push rbp
    mov rbp,rsp
    sub rsp,0
    mov rax,2
    push rax
    mov rax,3
    mov rbx,rax
    pop rax
    add rax,rbx
    push rax
    mov rbx,rax
    pop rax
    add rax,rbx
    leave
    ret
    leave
    ret
main:
    push rbp
    mov rbp,rsp
    sub rsp,0
    push r12
    xor r12, r12
    push r12
    xor r12, r12
    mov rax, 1
    mov rdi, 1
    lea rsi, [rel str_0]
    mov rdx, 3
    syscall
    add r12, rdx
    mov rax, r12
    pop r12
    mov rbx, rax
    lea rdi, [rel num_buf+19]
    cmp rbx, 0
    jne .conv_0_start
    dec rdi
    mov byte [rdi], '0'
    mov r8, rdi
    jmp .conv_0_done
.conv_0_start:
    mov r8, rdi
.conv_0_loop:
    xor rdx, rdx
    mov rax, rbx
    mov rcx, 10
    div rcx
    add dl, '0'
    dec rdi
    mov [rdi], dl
    mov rbx, rax
    test rax, rax
    jnz .conv_0_loop
.conv_0_done:
    mov rsi, rdi
    mov rdx, r8
    sub rdx, rdi
    mov rax, 1
    mov rdi, 1
    syscall
    add r12, rdx
    mov rax, r12
    pop r12
    leave
    ret

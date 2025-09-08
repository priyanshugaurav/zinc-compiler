section .data
str_1: db 32,61,32,0
str_6: db 32,105,115,32,101,118,101,110,10,0
str_7: db 32,105,115,32,111,100,100,10,0
str_4: db 41,32,61,32,0
str_0: db 70,97,99,116,111,114,105,97,108,32,111,102,32,0
str_3: db 70,105,98,111,110,97,99,99,105,40,0
str_5: db 83,117,109,32,117,112,116,111,32,0
str_2: db 10,0
section .bss
num_buf: resb 20
section .text
global _start
_start:
    call main
    mov rax,60
    xor rdi,rdi
    syscall
factorial:
    push rbp
    mov rbp,rsp
    sub rsp,8
    mov [rbp-8],rdi
    mov rax,[rbp-8]
    push rax
    mov rax,1
    mov rbx,rax
    pop rax
    cmp rax,rbx
    setle al
    movzx rax,al
    cmp rax, 0
    je else_0
    mov rax,1
    leave
    ret
    jmp ifend_1
else_0:
    mov rax,[rbp-8]
    push rax
    mov rax,[rbp-8]
    push rax
    mov rax,1
    mov rbx,rax
    pop rax
    sub rax,rbx
    mov rdi,rax
    call factorial
    mov rbx,rax
    pop rax
    imul rax,rbx
    leave
    ret
ifend_1:
    leave
    ret
fib:
    push rbp
    mov rbp,rsp
    sub rsp,8
    mov [rbp-8],rdi
    mov rax,[rbp-8]
    push rax
    mov rax,1
    mov rbx,rax
    pop rax
    cmp rax,rbx
    setle al
    movzx rax,al
    cmp rax, 0
    je else_2
    mov rax,[rbp-8]
    leave
    ret
    jmp ifend_3
else_2:
    mov rax,[rbp-8]
    push rax
    mov rax,1
    mov rbx,rax
    pop rax
    sub rax,rbx
    mov rdi,rax
    call fib
    push rax
    mov rax,[rbp-8]
    push rax
    mov rax,2
    mov rbx,rax
    pop rax
    sub rax,rbx
    mov rdi,rax
    call fib
    mov rbx,rax
    pop rax
    add rax,rbx
    leave
    ret
ifend_3:
    leave
    ret
sum_upto:
    push rbp
    mov rbp,rsp
    sub rsp,24
    mov [rbp-8],rdi
    mov rax,0
    mov [rbp-16],rax
    mov rax,1
    mov [rbp-24],rax
while_start_4:
    mov rax,[rbp-24]
    push rax
    mov rax,[rbp-8]
    mov rbx,rax
    pop rax
    cmp rax,rbx
    setle al
    movzx rax,al
    cmp rax, 0
    je while_end_5
    mov rax,[rbp-16]
    push rax
    mov rax,[rbp-16]
    push rax
    mov rax,[rbp-24]
    mov rbx,rax
    pop rax
    add rax,rbx
    mov rbx,rax
    pop rax
    mov [rbp-16],rbx
    mov rax,rbx
    mov rax,[rbp-24]
    push rax
    mov rax,[rbp-24]
    push rax
    mov rax,1
    mov rbx,rax
    pop rax
    add rax,rbx
    mov rbx,rax
    pop rax
    mov [rbp-24],rbx
    mov rax,rbx
    jmp while_start_4
while_end_5:
    mov rax,[rbp-16]
    leave
    ret
    leave
    ret
is_even:
    push rbp
    mov rbp,rsp
    sub rsp,8
    mov [rbp-8],rdi
    mov rax,[rbp-8]
    push rax
    mov rax,2
    mov rbx,rax
    pop rax
    cqo
    idiv rbx
    mov rax,rdx
    push rax
    mov rax,0
    mov rbx,rax
    pop rax
    cmp rax,rbx
    sete al
    movzx rax,al
    cmp rax, 0
    je else_6
    mov rax,1
    leave
    ret
    jmp ifend_7
else_6:
    mov rax,0
    leave
    ret
ifend_7:
    leave
    ret
main:
    push rbp
    mov rbp,rsp
    sub rsp,40
    mov rax,7
    mov [rbp-8],rax
    mov rax,[rbp-8]
    mov rdi,rax
    call factorial
    mov [rbp-16],rax
    push r12
    xor r12, r12
    mov rax, 1
    mov rdi, 1
    lea rsi, [rel str_0]
    mov rdx, 13
    syscall
    add r12, rdx
    mov rax,[rbp-8]
    mov rbx, rax
    lea rdi, [rel num_buf+19]
    cmp rbx, 0
    jne .conv_8_start
    dec rdi
    mov byte [rdi], '0'
    mov r8, rdi
    jmp .conv_8_done
.conv_8_start:
    mov r8, rdi
.conv_8_loop:
    xor rdx, rdx
    mov rax, rbx
    mov rcx, 10
    div rcx
    add dl, '0'
    dec rdi
    mov [rdi], dl
    mov rbx, rax
    test rax, rax
    jnz .conv_8_loop
.conv_8_done:
    mov rsi, rdi
    mov rdx, r8
    sub rdx, rdi
    mov rax, 1
    mov rdi, 1
    syscall
    add r12, rdx
    mov rax, 1
    mov rdi, 1
    lea rsi, [rel str_1]
    mov rdx, 3
    syscall
    add r12, rdx
    mov rax,[rbp-16]
    mov rbx, rax
    lea rdi, [rel num_buf+19]
    cmp rbx, 0
    jne .conv_9_start
    dec rdi
    mov byte [rdi], '0'
    mov r8, rdi
    jmp .conv_9_done
.conv_9_start:
    mov r8, rdi
.conv_9_loop:
    xor rdx, rdx
    mov rax, rbx
    mov rcx, 10
    div rcx
    add dl, '0'
    dec rdi
    mov [rdi], dl
    mov rbx, rax
    test rax, rax
    jnz .conv_9_loop
.conv_9_done:
    mov rsi, rdi
    mov rdx, r8
    sub rdx, rdi
    mov rax, 1
    mov rdi, 1
    syscall
    add r12, rdx
    mov rax, 1
    mov rdi, 1
    lea rsi, [rel str_2]
    mov rdx, 2
    syscall
    add r12, rdx
    mov rax, r12
    pop r12
    mov rax,[rbp-8]
    mov rdi,rax
    call fib
    mov [rbp-24],rax
    push r12
    xor r12, r12
    mov rax, 1
    mov rdi, 1
    lea rsi, [rel str_3]
    mov rdx, 10
    syscall
    add r12, rdx
    mov rax,[rbp-8]
    mov rbx, rax
    lea rdi, [rel num_buf+19]
    cmp rbx, 0
    jne .conv_10_start
    dec rdi
    mov byte [rdi], '0'
    mov r8, rdi
    jmp .conv_10_done
.conv_10_start:
    mov r8, rdi
.conv_10_loop:
    xor rdx, rdx
    mov rax, rbx
    mov rcx, 10
    div rcx
    add dl, '0'
    dec rdi
    mov [rdi], dl
    mov rbx, rax
    test rax, rax
    jnz .conv_10_loop
.conv_10_done:
    mov rsi, rdi
    mov rdx, r8
    sub rdx, rdi
    mov rax, 1
    mov rdi, 1
    syscall
    add r12, rdx
    mov rax, 1
    mov rdi, 1
    lea rsi, [rel str_4]
    mov rdx, 4
    syscall
    add r12, rdx
    mov rax,[rbp-24]
    mov rbx, rax
    lea rdi, [rel num_buf+19]
    cmp rbx, 0
    jne .conv_11_start
    dec rdi
    mov byte [rdi], '0'
    mov r8, rdi
    jmp .conv_11_done
.conv_11_start:
    mov r8, rdi
.conv_11_loop:
    xor rdx, rdx
    mov rax, rbx
    mov rcx, 10
    div rcx
    add dl, '0'
    dec rdi
    mov [rdi], dl
    mov rbx, rax
    test rax, rax
    jnz .conv_11_loop
.conv_11_done:
    mov rsi, rdi
    mov rdx, r8
    sub rdx, rdi
    mov rax, 1
    mov rdi, 1
    syscall
    add r12, rdx
    mov rax, 1
    mov rdi, 1
    lea rsi, [rel str_2]
    mov rdx, 2
    syscall
    add r12, rdx
    mov rax, r12
    pop r12
    mov rax,[rbp-8]
    mov rdi,rax
    call sum_upto
    mov [rbp-32],rax
    push r12
    xor r12, r12
    mov rax, 1
    mov rdi, 1
    lea rsi, [rel str_5]
    mov rdx, 9
    syscall
    add r12, rdx
    mov rax,[rbp-8]
    mov rbx, rax
    lea rdi, [rel num_buf+19]
    cmp rbx, 0
    jne .conv_12_start
    dec rdi
    mov byte [rdi], '0'
    mov r8, rdi
    jmp .conv_12_done
.conv_12_start:
    mov r8, rdi
.conv_12_loop:
    xor rdx, rdx
    mov rax, rbx
    mov rcx, 10
    div rcx
    add dl, '0'
    dec rdi
    mov [rdi], dl
    mov rbx, rax
    test rax, rax
    jnz .conv_12_loop
.conv_12_done:
    mov rsi, rdi
    mov rdx, r8
    sub rdx, rdi
    mov rax, 1
    mov rdi, 1
    syscall
    add r12, rdx
    mov rax, 1
    mov rdi, 1
    lea rsi, [rel str_1]
    mov rdx, 3
    syscall
    add r12, rdx
    mov rax,[rbp-32]
    mov rbx, rax
    lea rdi, [rel num_buf+19]
    cmp rbx, 0
    jne .conv_13_start
    dec rdi
    mov byte [rdi], '0'
    mov r8, rdi
    jmp .conv_13_done
.conv_13_start:
    mov r8, rdi
.conv_13_loop:
    xor rdx, rdx
    mov rax, rbx
    mov rcx, 10
    div rcx
    add dl, '0'
    dec rdi
    mov [rdi], dl
    mov rbx, rax
    test rax, rax
    jnz .conv_13_loop
.conv_13_done:
    mov rsi, rdi
    mov rdx, r8
    sub rdx, rdi
    mov rax, 1
    mov rdi, 1
    syscall
    add r12, rdx
    mov rax, 1
    mov rdi, 1
    lea rsi, [rel str_2]
    mov rdx, 2
    syscall
    add r12, rdx
    mov rax, r12
    pop r12
    mov rax,[rbp-8]
    mov rdi,rax
    call is_even
    mov [rbp-40],rax
    mov rax,[rbp-40]
    push rax
    mov rax,1
    mov rbx,rax
    pop rax
    cmp rax,rbx
    sete al
    movzx rax,al
    cmp rax, 0
    je else_14
    push r12
    xor r12, r12
    mov rax,[rbp-8]
    mov rbx, rax
    lea rdi, [rel num_buf+19]
    cmp rbx, 0
    jne .conv_16_start
    dec rdi
    mov byte [rdi], '0'
    mov r8, rdi
    jmp .conv_16_done
.conv_16_start:
    mov r8, rdi
.conv_16_loop:
    xor rdx, rdx
    mov rax, rbx
    mov rcx, 10
    div rcx
    add dl, '0'
    dec rdi
    mov [rdi], dl
    mov rbx, rax
    test rax, rax
    jnz .conv_16_loop
.conv_16_done:
    mov rsi, rdi
    mov rdx, r8
    sub rdx, rdi
    mov rax, 1
    mov rdi, 1
    syscall
    add r12, rdx
    mov rax, 1
    mov rdi, 1
    lea rsi, [rel str_6]
    mov rdx, 10
    syscall
    add r12, rdx
    mov rax, r12
    pop r12
    jmp ifend_15
else_14:
    push r12
    xor r12, r12
    mov rax,[rbp-8]
    mov rbx, rax
    lea rdi, [rel num_buf+19]
    cmp rbx, 0
    jne .conv_17_start
    dec rdi
    mov byte [rdi], '0'
    mov r8, rdi
    jmp .conv_17_done
.conv_17_start:
    mov r8, rdi
.conv_17_loop:
    xor rdx, rdx
    mov rax, rbx
    mov rcx, 10
    div rcx
    add dl, '0'
    dec rdi
    mov [rdi], dl
    mov rbx, rax
    test rax, rax
    jnz .conv_17_loop
.conv_17_done:
    mov rsi, rdi
    mov rdx, r8
    sub rdx, rdi
    mov rax, 1
    mov rdi, 1
    syscall
    add r12, rdx
    mov rax, 1
    mov rdi, 1
    lea rsi, [rel str_7]
    mov rdx, 9
    syscall
    add r12, rdx
    mov rax, r12
    pop r12
ifend_15:
    leave
    ret

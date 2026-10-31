global _start
_start:
    mov rax,1
    push rax
    mov rax,2
    push rax
    pop rax
    pop rbx
    add rax, rbx
    push rax
    mov rax,1
    push rax
    mov rax,1
    push rax
    pop rax
    test rax, rax
    jz label0
    mov rax,1
    push rax
    push QWORD[rsp +8]
    pop rax
    pop rbx
    add rax, rbx
    push rax
    pop rax
    mov [rsp + 0],rax
    add rsp,0
    jmp label1
label0:
    mov rax,1
    push rax
    pop rax
    test rax, rax
    jz label2
    mov rax,2
    push rax
    push QWORD[rsp +8]
    pop rax
    pop rbx
    add rax, rbx
    push rax
    pop rax
    mov [rsp + 0],rax
    add rsp,0
    jmp label1
label2:
    mov rax,3
    push rax
    push QWORD[rsp +8]
    pop rax
    pop rbx
    add rax, rbx
    push rax
    pop rax
    mov [rsp + 0],rax
    add rsp,0
    ;; /else
    ;; /elif
label1:
    ;; /if
    ;; /exit
    push QWORD[rsp +0]
    mov rax,60
    pop rdi
    syscall
    ;; /exit
    mov rax, 60
    mov rdi, 0
    syscall
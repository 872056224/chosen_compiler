; Function: main
main:
    push bp
    mov  bp, sp
    sub  sp, 22
    mov  bx, bp
    sub  bx, 22

main_entry_0:
    mov di, 0
    mov [bx+16], di
    mov di, 0
    mov [bx+18], di
    mov di, 0
    mov [bx+20], di
    mov di, 328
    mov dx, 0
    mov si, dx
    shl si, 1
    mov [bx+si], di
    mov di, 58
    mov dx, 1
    mov si, dx
    shl si, 1
    mov [bx+si], di
    mov di, 13
    mov dx, 2
    mov si, dx
    shl si, 1
    mov [bx+si], di
    mov di, 380
    mov dx, 3
    mov si, dx
    shl si, 1
    mov [bx+si], di
    mov di, 141
    mov dx, 4
    mov si, dx
    shl si, 1
    mov [bx+si], di
    mov di, 126
    mov dx, 5
    mov si, dx
    shl si, 1
    mov [bx+si], di
    mov di, 115
    mov dx, 6
    mov si, dx
    shl si, 1
    mov [bx+si], di
    mov di, 72
    mov dx, 7
    mov si, dx
    shl si, 1
    mov [bx+si], di
    mov di, 0
    mov [bx+16], di
    jmp main_while_cond_1

main_while_cond_1:
    mov di, [bx+16]
    cmp di, 8
        jl main_while_body_2
    jmp main_while_exit_3

main_while_body_2:
    mov di, [bx+16]
    mov [bx+18], di
    jmp main_while_cond_4

main_while_exit_3:
    mov di, 0
    mov [bx+16], di
    jmp main_while_cond_9

main_while_cond_4:
    mov di, [bx+18]
    cmp di, 8
        jl main_while_body_5
    jmp main_while_exit_6

main_while_body_5:
    mov di, [bx+16]
    mov si, di
    shl si, 1
    mov di, [bx+si]
    mov dx, [bx+18]
    mov si, dx
    shl si, 1
    mov dx, [bx+si]
    cmp di, dx
        jg main_then_7
    jmp main_if_end_8

main_while_exit_6:
    mov di, [bx+16]
    mov dx, di
    add dx, 1
    mov [bx+16], dx
    jmp main_while_cond_1

main_then_7:
    mov dx, [bx+16]
    mov si, dx
    shl si, 1
    mov dx, [bx+si]
    mov [bx+20], dx
    mov dx, [bx+16]
    mov di, [bx+18]
    mov si, di
    shl si, 1
    mov di, [bx+si]
    mov si, dx
    shl si, 1
    mov [bx+si], di
    mov di, [bx+18]
    mov dx, [bx+20]
    mov si, di
    shl si, 1
    mov [bx+si], dx
    jmp main_if_end_8

main_if_end_8:
    mov dx, [bx+18]
    mov di, dx
    add di, 1
    mov [bx+18], di
    jmp main_while_cond_4

main_while_cond_9:
    mov di, [bx+16]
    cmp di, 8
        jl main_while_body_10
    jmp main_while_exit_11

main_while_body_10:
    mov di, [bx+16]
    mov si, di
    shl si, 1
    mov di, [bx+si]
    mov ax, di
    call __print
    mov di, [bx+16]
    mov dx, di
    add dx, 1
    mov [bx+16], dx
    jmp main_while_cond_9

main_while_exit_11:
    mov dx, 0
    mov si, dx
    shl si, 1
    mov dx, [bx+si]
    mov ax, dx
    mov  sp, bp
    pop  bp
    ret


; --- Runtime: __print (AX = value to print) ---
__print:
    push bp
    push bx
    push cx
    push dx
    mov  cx, 0
    mov  bx, 10
__print_loop:
    mov  dx, 0
    div  bx
    push dx
    inc  cx
    cmp  ax, 0
    jne  __print_loop
__print_disp:
    pop  dx
    add  dl, '0'
    mov  ah, 02h
    int  21h
    loop __print_disp
    mov  dl, ' '
    mov  ah, 02h
    int  21h
    pop  dx
    pop  cx
    pop  bx
    pop  bp
    ret

; --- Runtime: __read (returns in AX) ---
__read:
    push bp
    mov  bp, sp
    mov  bx, bp
    push bx
    push cx
    push dx
    mov  bx, 0
    mov  cx, 0
__read_loop:
    mov  ah, 01h
    int  21h
    cmp  al, 0Dh
    je   __read_done
    sub  al, '0'
    mov  cl, al
    mov  ch, 0
    mov  ax, bx
    mov  dx, 10
    mul  dx
    add  ax, cx
    mov  bx, ax
    jmp  __read_loop
__read_done:
    mov  ax, bx
    pop  dx
    pop  cx
    pop  bx
    pop  bp
    ret
hlt

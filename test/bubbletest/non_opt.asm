; Function: main
main:
    push bp
    mov  bp, sp
    sub  sp, 22
    mov  bx, bp
    sub  bx, 22
    mov  ax, 0
    mov  [bx+16], ax
    mov  cx, 0
    mov  [bx+18], cx
    mov  dx, 0
    mov  [bx+20], dx
    mov  ax, 328
    mov  si, 0
    shl  si, 1
    mov  [bx+si], ax
    mov  cx, 58
    mov  si, 1
    shl  si, 1
    mov  [bx+si], cx
    mov  dx, 13
    mov  si, 2
    shl  si, 1
    mov  [bx+si], dx
    mov  ax, 380
    mov  si, 3
    shl  si, 1
    mov  [bx+si], ax
    mov  cx, 141
    mov  si, 4
    shl  si, 1
    mov  [bx+si], cx
    mov  dx, 126
    mov  si, 5
    shl  si, 1
    mov  [bx+si], dx
    mov  ax, 115
    mov  si, 6
    shl  si, 1
    mov  [bx+si], ax
    mov  cx, 72
    mov  si, 7
    shl  si, 1
    mov  [bx+si], cx
    mov  dx, 0
    mov  [bx+16], dx
    jmp  main_while_cond_1
main_while_cond_1:
    mov  ax, [bx+16]
    cmp  ax, 8
    jl  main_while_body_2
    jmp  main_while_exit_3
main_while_body_2:
    mov  cx, [bx+16]
    mov  [bx+18], cx
    jmp  main_while_cond_4
main_while_exit_3:
    mov  dx, 0
    mov  [bx+16], dx
    jmp  main_while_cond_9
main_while_cond_4:
    mov  ax, [bx+18]
    cmp  ax, 8
    jl  main_while_body_5
    jmp  main_while_exit_6
main_while_body_5:
    mov  cx, [bx+16]
    mov  si, cx
    shl  si, 1
    mov  dx, [bx+si]
    mov  ax, [bx+18]
    mov  si, ax
    shl  si, 1
    mov  cx, [bx+si]
    cmp  dx, cx
    jg  main_then_7
    jmp  main_if_end_8
main_while_exit_6:
    mov  dx, [bx+16]
    add  dx, 1
    mov  [bx+16], dx
    jmp  main_while_cond_1
main_then_7:
    mov  ax, [bx+16]
    mov  si, ax
    shl  si, 1
    mov  cx, [bx+si]
    mov  [bx+20], cx
    mov  dx, [bx+16]
    mov  ax, [bx+18]
    mov  si, ax
    shl  si, 1
    mov  cx, [bx+si]
    mov  si, dx
    shl  si, 1
    mov  [bx+si], cx
    mov  dx, [bx+18]
    mov  ax, [bx+20]
    mov  si, dx
    shl  si, 1
    mov  [bx+si], ax
    jmp  main_if_end_8
main_if_end_8:
    mov  cx, [bx+18]
    add  cx, 1
    mov  [bx+18], cx
    jmp  main_while_cond_4
main_while_cond_9:
    mov  dx, [bx+16]
    cmp  dx, 8
    jl  main_while_body_10
    jmp  main_while_exit_11
main_while_body_10:
    mov  ax, [bx+16]
    mov  si, ax
    shl  si, 1
    mov  cx, [bx+si]
    mov  ax, cx
    call  __print
    mov  dx, [bx+16]
    add  dx, 1
    mov  [bx+16], dx
    jmp  main_while_cond_9
main_while_exit_11:
    mov  si, 0
    shl  si, 1
    mov  ax, [bx+si]
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
    push bx
    push cx
    push dx
    mov  ax, 0
    mov  bx, 0
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

; Function: main
main:
    push bp
    mov  bp, sp
    sub  sp, 46
    mov  bx, bp
    sub  bx, 46
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
    mov  dx, 378
    mov  si, 8
    shl  si, 1
    mov  [bx+si], dx
    mov  ax, 53
    mov  si, 9
    shl  si, 1
    mov  [bx+si], ax
    mov  cx, 347
    mov  si, 10
    shl  si, 1
    mov  [bx+si], cx
    mov  dx, 380
    mov  si, 11
    shl  si, 1
    mov  [bx+si], dx
    mov  ax, 457
    mov  si, 12
    shl  si, 1
    mov  [bx+si], ax
    mov  cx, 280
    mov  si, 13
    shl  si, 1
    mov  [bx+si], cx
    mov  dx, 45
    mov  si, 14
    shl  si, 1
    mov  [bx+si], dx
    mov  ax, 303
    mov  si, 15
    shl  si, 1
    mov  [bx+si], ax
    mov  cx, 217
    mov  si, 16
    shl  si, 1
    mov  [bx+si], cx
    mov  dx, 17
    mov  si, 17
    shl  si, 1
    mov  [bx+si], dx
    mov  ax, 16
    mov  si, 18
    shl  si, 1
    mov  [bx+si], ax
    mov  cx, 48
    mov  si, 19
    shl  si, 1
    mov  [bx+si], cx
    jmp  main_while_cond_1
main_while_cond_1:
    mov  dx, [bx+40]
    cmp  dx, 20
    jl  main_while_body_2
    jmp  main_while_exit_3
main_while_body_2:
    mov  ax, [bx+40]
    mov  [bx+42], ax
    jmp  main_while_cond_4
main_while_exit_3:
    mov  cx, 0
    mov  [bx+40], cx
    jmp  main_while_cond_9
main_while_cond_4:
    mov  dx, [bx+42]
    cmp  dx, 20
    jl  main_while_body_5
    jmp  main_while_exit_6
main_while_body_5:
    mov  ax, [bx+40]
    mov  si, ax
    shl  si, 1
    mov  cx, [bx+si]
    mov  dx, [bx+42]
    mov  si, dx
    shl  si, 1
    mov  ax, [bx+si]
    cmp  cx, ax
    jg  main_then_7
    jmp  main_if_end_8
main_while_exit_6:
    mov  cx, [bx+40]
    add  cx, 1
    mov  [bx+40], cx
    jmp  main_while_cond_1
main_then_7:
    mov  dx, [bx+40]
    mov  si, dx
    shl  si, 1
    mov  ax, [bx+si]
    mov  [bx+44], ax
    mov  cx, [bx+40]
    mov  dx, [bx+42]
    mov  si, dx
    shl  si, 1
    mov  ax, [bx+si]
    mov  si, cx
    shl  si, 1
    mov  [bx+si], ax
    mov  cx, [bx+42]
    mov  dx, [bx+44]
    mov  si, cx
    shl  si, 1
    mov  [bx+si], dx
    jmp  main_while_cond_4
main_if_end_8:
    mov  ax, [bx+42]
    add  ax, 1
    mov  [bx+42], ax
    jmp  main_while_cond_4
main_while_cond_9:
    mov  cx, [bx+40]
    cmp  cx, 20
    jl  main_while_body_10
    jmp  main_while_exit_11
main_while_body_10:
    mov  dx, [bx+40]
    mov  si, dx
    shl  si, 1
    mov  ax, [bx+si]
    call  __print
    mov  cx, [bx+40]
    add  cx, 1
    mov  [bx+40], cx
    jmp  main_while_cond_9
main_while_exit_11:
    mov  si, 0
    shl  si, 1
    mov  dx, [bx+si]
    mov  ax, dx
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

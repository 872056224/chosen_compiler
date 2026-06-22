; Function: main
main:
    push bp
    mov  bp, sp
    sub  sp, 6
    mov  bx, bp
    sub  bx, 6
    mov  ax, 0
    mov  [bx+4], ax
    mov  cx, 0
    mov  [bx+2], cx
    jmp  main_while_cond_1
main_while_cond_1:
    mov  dx, [bx+4]
    mov  ax, [bx+2]
    cmp  dx, 5
    jl  main_while_body_2
    jmp  main_while_exit_3
main_while_body_2:
    add  ax, dx
    add  dx, 1
    mov  [bx+4], dx
    mov  [bx+2], ax
    jmp  main_while_cond_1
main_while_exit_3:
    add  dx, ax
    add  dx, dx
    mov  cx, 0
    cmp  cx, 0
    jne  main_then_4
    jmp  main_if_end_5
main_then_4:
    mov  dx, 999
    mov  ax, dx
    call  __print
    jmp  main_if_end_5
main_if_end_5:
    mov  ax, 1
    cmp  ax, 0
    mov  [bx], dx
    jne  main_then_6
    jmp  main_if_end_7
main_then_6:
    add  dx, 1
    mov  [bx], dx
    jmp  main_if_end_7
main_if_end_7:
    mov  cx, [bx]
    call  __print
    mov  ax, cx
    call  __print
    mov  dx, 50
    mov  ax, dx
    call  __print
    mov  ax, 0
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

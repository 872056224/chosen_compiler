; Function: main
main:
    push bp
    mov  bp, sp
    sub  sp, 32
    mov  bx, bp
    sub  bx, 32
    jmp  main_while_cond_1
main_while_cond_1:
    mov  ax, [bx+2]
    cmp  ax, 5
    jl  main_while_body_2
    jmp  main_while_exit_3
main_while_body_2:
    mov  cx, [bx]
    mov  dx, [bx+2]
    add  cx, dx
    mov  [bx], cx
    mov  ax, [bx+2]
    add  ax, 1
    mov  [bx+2], ax
    jmp  main_while_cond_1
main_while_exit_3:
    mov  cx, 100
    mov  [bx+4], cx
    mov  dx, [bx+4]
    mov  [bx+6], dx
    mov  ax, [bx+4]
    mov  [bx+8], ax
    mov  cx, 50
    mov  [bx+10], cx
    mov  dx, [bx+10]
    mov  [bx+12], dx
    mov  ax, [bx+4]
    mov  cx, [bx+4]
    sub  ax, cx
    mov  [bx+14], ax
    mov  dx, 5
    mov  [bx+16], dx
    mov  ax, [bx+16]
    mov  cx, 4
    add  cx, ax
    mov  [bx+18], cx
    mov  dx, [bx+16]
    mov  ax, [bx+16]
    mov  cx, 2
    add  cx, ax
    add  dx, cx
    mov  [bx+20], dx
    mov  dx, [bx+2]
    mov  ax, [bx]
    add  dx, ax
    mov  [bx+22], dx
    mov  cx, [bx+2]
    mov  dx, [bx]
    add  cx, dx
    mov  [bx+24], cx
    mov  ax, [bx+22]
    mov  cx, [bx+24]
    add  ax, cx
    mov  [bx+26], ax
    mov  dx, 0
    cmp  dx, 0
    jne  main_then_4
    jmp  main_if_end_5
main_then_4:
    mov  ax, 999
    call  __print
    jmp  main_if_end_5
main_if_end_5:
    mov  cx, 1
    cmp  cx, 0
    jne  main_then_6
    jmp  main_if_end_7
main_then_6:
    mov  dx, [bx+26]
    add  dx, 1
    mov  [bx+26], dx
    jmp  main_if_end_7
main_if_end_7:
    mov  ax, 999
    mov  [bx+28], ax
    mov  cx, [bx+28]
    add  cx, 1
    mov  [bx+30], cx
    mov  dx, [bx+30]
    mov  ax, dx
    mov  ax, 10
    mul  ax
    mov  [bx+30], ax
    mov  cx, [bx]
    mov  ax, cx
    call  __print
    mov  dx, [bx+26]
    mov  ax, dx
    call  __print
    mov  ax, [bx+10]
    call  __print
    mov  cx, 0
    mov  ax, cx
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

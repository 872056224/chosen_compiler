.code

; Function: main
main proc
    push bp
    mov  bp, sp
    sub  sp, 4
    mov 	ax, 0
    mov 	[bp-2], ax
    mov 	bx, 0
    mov 	[bp-4], bx
    jmp 	.main_while_cond
.main_while_cond:
    mov 	cx, [bp-2]
    mov 	t0, cx
    mov 	dx, t0
    cmp 	dx, 10
    jl 	.main_while_body
    jmp 	.main_while_exit
.main_while_body:
    mov 	ax, [bp-4]
    mov 	t1, ax
    mov 	bx, [bp-2]
    mov 	t2, bx
    mov 	cx, t1
    add 	cx, t2
    mov 	dx, cx
    mov 	[bp-4], dx
    mov 	ax, [bp-2]
    mov 	t3, ax
    mov 	bx, t3
    add 	bx, 1
    mov 	cx, bx
    mov 	[bp-2], cx
    jmp 	.main_while_cond
.main_while_exit:
    mov 	dx, [bp-4]
    mov 	t4, dx
    mov 	ax, t4
    ret
    mov  sp, bp
    pop  bp
main endp

end

; After PhiElimination on main
define i16 @main() {
entry:
  %arr = alloca i16
  %i = alloca i16
  %j = alloca i16
  %temp = alloca i16
  arraystore i16 328, %arr, 0
  arraystore i16 58, %arr, 1
  arraystore i16 13, %arr, 2
  arraystore i16 380, %arr, 3
  arraystore i16 141, %arr, 4
  arraystore i16 126, %arr, 5
  arraystore i16 115, %arr, 6
  arraystore i16 72, %arr, 7
  arraystore i16 378, %arr, 8
  arraystore i16 53, %arr, 9
  arraystore i16 347, %arr, 10
  arraystore i16 380, %arr, 11
  arraystore i16 457, %arr, 12
  arraystore i16 280, %arr, 13
  arraystore i16 45, %arr, 14
  arraystore i16 303, %arr, 15
  arraystore i16 217, %arr, 16
  arraystore i16 17, %arr, 17
  arraystore i16 16, %arr, 18
  arraystore i16 48, %arr, 19
  br %while_cond
while_cond:
  %i = load i16, i16 %i
  %lt_tmp = icmp slt i16 %i, 20
  br i1 %lt_tmp, %while_body, %while_exit
while_body:
  %i = load i16, i16 %i
  store i16 %i, i16 %j
  br %while_cond
while_exit:
  store i16 0, i16 %i
  br %while_cond
while_cond:
  %j = load i16, i16 %j
  %lt_tmp = icmp slt i16 %j, 20
  br i1 %lt_tmp, %while_body, %while_exit
while_body:
  %i = load i16, i16 %i
  %arr_val = arrayload i16 %arr, %i
  %j = load i16, i16 %j
  %arr_val = arrayload i16 %arr, %j
  %gt_tmp = icmp sgt i16 %arr_val, %arr_val
  br i1 %gt_tmp, %then, %if_end
while_exit:
  %i = load i16, i16 %i
  %add_tmp = add i16 %i, 1
  store i16 %add_tmp, i16 %i
  br %while_cond
then:
  %i = load i16, i16 %i
  %arr_val = arrayload i16 %arr, %i
  store i16 %arr_val, i16 %temp
  %i = load i16, i16 %i
  %j = load i16, i16 %j
  %arr_val = arrayload i16 %arr, %j
  arraystore i16 %arr_val, %arr, %i
  %j = load i16, i16 %j
  %temp = load i16, i16 %temp
  arraystore i16 %temp, %arr, %j
  br %if_end
if_end:
  %j = load i16, i16 %j
  %add_tmp = add i16 %j, 1
  store i16 %add_tmp, i16 %j
  br %while_cond
while_cond:
  %i = load i16, i16 %i
  %lt_tmp = icmp slt i16 %i, 20
  br i1 %lt_tmp, %while_body, %while_exit
while_body:
  %i = load i16, i16 %i
  %arr_val = arrayload i16 %arr, %i
  call void @__print(i16 %arr_val)
  %i = load i16, i16 %i
  %add_tmp = add i16 %i, 1
  store i16 %add_tmp, i16 %i
  br %while_cond
while_exit:
  %arr_val = arrayload i16 %arr, 0
  ret i16 %arr_val
}

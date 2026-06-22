; After Mem2Reg on main
define i16 @main() {
entry:
  br %while_cond
while_cond:
  %i_phi = phi i16 [ 0, %entry ], [ %add_tmp, %while_body ]
  %sum_phi = phi i16 [ 0, %entry ], [ %add_tmp, %while_body ]
  %lt_tmp = icmp slt i16 %i_phi, 5
  br i1 %lt_tmp, %while_body, %while_exit
while_body:
  %add_tmp = add i16 %sum_phi, %i_phi
  %add_tmp = add i16 %i_phi, 1
  br %while_cond
while_exit:
  %add_tmp = add i16 100, 0
  %mul_tmp = mul i16 100, 1
  %add_tmp = add i16 20, 30
  %sub_tmp = sub i16 %add_tmp, 0
  %sub_tmp = sub i16 100, 100
  %add_tmp = add i16 1, 5
  %add_tmp = add i16 %add_tmp, 3
  %add_tmp = add i16 2, 5
  %add_tmp = add i16 5, %add_tmp
  %add_tmp = add i16 %i_phi, %sum_phi
  %add_tmp = add i16 %i_phi, %sum_phi
  %add_tmp = add i16 %add_tmp, %add_tmp
  %ne_tmp = icmp ne i16 0, 0
  br i1 %ne_tmp, %then, %if_end
then:
  call void @__print(i16 999)
  br %if_end
if_end:
  %ne_tmp = icmp ne i16 1, 0
  br i1 %ne_tmp, %then, %if_end
then:
  %add_tmp = add i16 %add_tmp, 1
  br %if_end
if_end:
  %r_phi = phi i16 [ %add_tmp, %if_end ], [ %add_tmp, %then ]
  %add_tmp = add i16 999, 1
  %mul_tmp = mul i16 %add_tmp, 10
  call void @__print(i16 %sum_phi)
  call void @__print(i16 %r_phi)
  call void @__print(i16 %add_tmp)
  ret i16 0
}

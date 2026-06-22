; After SimplifyCFG on main
define i16 @main() {
entry:
  %r_phi_slot = alloca i16
  %sum_phi_slot = alloca i16
  %i_phi_slot = alloca i16
  store i16 0, i16 %i_phi_slot
  store i16 0, i16 %sum_phi_slot
  br %while_cond
while_cond:
  %i_phi_reload = load i16, i16 %i_phi_slot
  %sum_phi_reload = load i16, i16 %sum_phi_slot
  %lt_tmp = icmp slt i16 %i_phi_reload, 5
  br i1 %lt_tmp, %while_body, %while_exit
while_body:
  %add_tmp = add i16 %sum_phi_reload, %i_phi_reload
  %add_tmp = add i16 %i_phi_reload, 1
  store i16 %add_tmp, i16 %i_phi_slot
  store i16 %add_tmp, i16 %sum_phi_slot
  br %while_cond
while_exit:
  %add_tmp = add i16 %i_phi_reload, %sum_phi_reload
  %add_tmp = add i16 %add_tmp, %add_tmp
  %ne_tmp = icmp ne i16 0, 0
  br i1 %ne_tmp, %then, %if_end
then:
  call void @__print(i16 999)
  br %if_end
if_end:
  %ne_tmp = icmp ne i16 1, 0
  store i16 %add_tmp, i16 %r_phi_slot
  br i1 %ne_tmp, %then, %if_end
then:
  %add_tmp = add i16 %add_tmp, 1
  store i16 %add_tmp, i16 %r_phi_slot
  br %if_end
if_end:
  %r_phi_reload = load i16, i16 %r_phi_slot
  call void @__print(i16 %sum_phi_reload)
  call void @__print(i16 %r_phi_reload)
  call void @__print(i16 50)
  ret i16 0
}

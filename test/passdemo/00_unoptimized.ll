; Module: ll1_module

define i16 @main() {
entry:
  %sum = alloca i16
  store i16 0, i16 %sum
  %i = alloca i16
  store i16 0, i16 %i
  br %while_cond
while_cond:
  %i = load i16, i16 %i
  %lt_tmp = icmp slt i16 %i, 5
  br i1 %lt_tmp, %while_body, %while_exit
while_body:
  %sum = load i16, i16 %sum
  %i = load i16, i16 %i
  %add_tmp = add i16 %sum, %i
  store i16 %add_tmp, i16 %sum
  %i = load i16, i16 %i
  %add_tmp = add i16 %i, 1
  store i16 %add_tmp, i16 %i
  br %while_cond
while_exit:
  %a = alloca i16
  store i16 100, i16 %a
  %b = alloca i16
  %a = load i16, i16 %a
  %add_tmp = add i16 %a, 0
  store i16 %add_tmp, i16 %b
  %c = alloca i16
  %a = load i16, i16 %a
  %mul_tmp = mul i16 %a, 1
  store i16 %mul_tmp, i16 %c
  %d = alloca i16
  %add_tmp = add i16 20, 30
  store i16 %add_tmp, i16 %d
  %e = alloca i16
  %d = load i16, i16 %d
  %sub_tmp = sub i16 %d, 0
  store i16 %sub_tmp, i16 %e
  %f = alloca i16
  %a = load i16, i16 %a
  %a = load i16, i16 %a
  %sub_tmp = sub i16 %a, %a
  store i16 %sub_tmp, i16 %f
  %x = alloca i16
  store i16 5, i16 %x
  %y = alloca i16
  %x = load i16, i16 %x
  %add_tmp = add i16 1, %x
  %add_tmp = add i16 %add_tmp, 3
  store i16 %add_tmp, i16 %y
  %z = alloca i16
  %x = load i16, i16 %x
  %x = load i16, i16 %x
  %add_tmp = add i16 2, %x
  %add_tmp = add i16 %x, %add_tmp
  store i16 %add_tmp, i16 %z
  %p = alloca i16
  %i = load i16, i16 %i
  %sum = load i16, i16 %sum
  %add_tmp = add i16 %i, %sum
  store i16 %add_tmp, i16 %p
  %q = alloca i16
  %i = load i16, i16 %i
  %sum = load i16, i16 %sum
  %add_tmp = add i16 %i, %sum
  store i16 %add_tmp, i16 %q
  %r = alloca i16
  %p = load i16, i16 %p
  %q = load i16, i16 %q
  %add_tmp = add i16 %p, %q
  store i16 %add_tmp, i16 %r
  %ne_tmp = icmp ne i16 0, 0
  br i1 %ne_tmp, %then, %if_end
then:
  call void @__print(i16 999)
  br %if_end
if_end:
  %ne_tmp = icmp ne i16 1, 0
  br i1 %ne_tmp, %then, %if_end
then:
  %r = load i16, i16 %r
  %add_tmp = add i16 %r, 1
  store i16 %add_tmp, i16 %r
  br %if_end
if_end:
  %dead1 = alloca i16
  store i16 999, i16 %dead1
  %dead2 = alloca i16
  %dead1 = load i16, i16 %dead1
  %add_tmp = add i16 %dead1, 1
  store i16 %add_tmp, i16 %dead2
  %dead2 = load i16, i16 %dead2
  %mul_tmp = mul i16 %dead2, 10
  store i16 %mul_tmp, i16 %dead2
  %sum = load i16, i16 %sum
  call void @__print(i16 %sum)
  %r = load i16, i16 %r
  call void @__print(i16 %r)
  %d = load i16, i16 %d
  call void @__print(i16 %d)
  ret i16 0
}

define void @__print(i16 %val) {
}


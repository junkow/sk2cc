#!/bin/bash

count=0

error() {
  echo "Case: #$count"
  echo "[failed]"
  echo "exit with error."
  echo "[input]"
  cat tmp/as_test.s
  echo "[log]"
  cat tmp/as_test.log
  exit 1
}

invalid() {
  echo "Case: #$count"
  echo "[failed]"
  echo "invalid output: failed to link with gcc."
  echo "[input]"
  cat tmp/as_test.s
  echo "[output]"
  objdump -d tmp/as_test.o | sed '/^$/d'
  exit 1
}

failed() {
  expect=$1
  actual=$2
  echo "Case: #$count"
  echo "[failed]"
  echo "$expect is expected, but got $actual."
  echo "[input]"
  cat tmp/as_test.s
  echo "[output]"
  objdump -d tmp/as_test.o | sed '/^$/d'
  exit 1
}

expect() {
  expect=$1
  cat - > tmp/as_test.s
  ./as tmp/as_test.s tmp/as_test.o 2> tmp/as_test.log || error
  gcc tmp/as_test.o -o tmp/as_test || invalid
  ./tmp/as_test
  actual=$?
  [ $actual -ne $expect ] && failed $expect $actual
  count=$((count+1))
}

expect 42 << EOS
main:
  movq \$42, %rax
  ret
EOS

expect 12 << EOS
main:
  movq \$12, %rax
  movq \$23, %rcx
  movq \$34, %rdx
  movq \$45, %rbx
  movq \$56, %rsi
  movq \$67, %rdi
  ret
EOS

expect 12 << EOS
main:
  movq \$12, %rax
  movq \$23, %r8
  movq \$34, %r9
  movq \$45, %r10
  movq \$56, %r11
  movq \$67, %r12
  movq \$78, %r13
  movq \$89, %r14
  movq \$90, %r15
  ret
EOS

expect 34 << EOS
main:
  movq \$34, %rdx
  movq %rdx, %r9
  movq %r9, %rax
  ret
EOS

expect 63 << EOS
main:
  movq \$63, %rbx
  pushq %rbx
  popq %rax
  pushq %r13
  popq %r14
  ret
EOS

expect 13 << EOS
main:
  pushq %rbp
  movq %rsp, %rbp
  movq \$13, %rax
  leave
  ret
EOS

expect 85 << EOS
main:
  movq %rsp, %rdx
  movq (%rdx), %r11
  movq \$85, %rcx
  movq %rcx, (%rdx)
  movq (%rdx), %rax
  movq %r11, (%rdx)
  ret
EOS

expect 84 << EOS
main:
  movq %rsp, %r13
  movq (%r13), %r11
  movq \$84, %rcx
  movq %rcx, (%r13)
  movq (%r13), %rax
  movq %r11, (%r13)
  ret
EOS

expect 39 << EOS
main:
  movq %rsp, %rdx
  movq \$39, %rsi
  movq %rsi, -8(%rdx)
  movq -8(%rdx), %rdi
  movq %rdi, -144(%rdx)
  movq -144(%rdx), %rax
  ret
EOS

expect 46 << EOS
func:
  movq %rdi, %rax
  ret
main:
  movq \$64, %rax
  movq \$46, %rdi
  call func
  ret
EOS

expect 53 << EOS
main:
  movq %rsp, %rcx
  movq \$53, -8(%rcx)
  movq -8(%rcx), %rdx
  movq %rdx, -144(%rcx)
  movq -144(%rcx), %rax
  ret
EOS

expect 16 << EOS
main:
  movq \$16, -8(%rsp)
  movq -8(%rsp), %rcx
  movq %rcx, -144(%rsp)
  movq -144(%rsp), %rax
  ret
EOS

expect 22 << EOS
main:
  movq \$2, %rsi
  movq \$8, %r14
  movq \$22, -32(%rsp, %rsi, 8)
  movq -32(%rsp, %rsi, 8), %rcx
  movq %rcx, -144(%rsp, %r14)
  movq -144(%rsp, %r14), %rax
  ret
EOS

expect 25 << EOS
main:
  movl \$25, %r12d
  movl %r12d, %eax
  ret
EOS

expect 31 << EOS
main:
  movw \$31, %r12w
  movw %r12w, %ax
  ret
EOS

expect 57 << EOS
main:
  movb \$57, %r12b
  movb %r12b, %al
  ret
EOS

expect 87 << EOS
main:
  movb \$87, %sil
  movb %sil, %al
  ret
EOS

gcc as_string.c as_vector.c as_map.c as_binary.c as_error.c as_scan.c as_lex.c as_parse.c as_gen.c as_elf.c tests/as_driver.c -o tmp/as_driver

encoding_failed() {
  asm=$1
  expected=$2
  actual=$3
  echo "[failed]"
  echo "instruction encoding check failed."
  echo "input:    $asm"
  echo "expected: $expected"
  echo "actual:   $actual"
  exit 1
}

test_encoding() {
  asm=$1
  expected=$2
  actual=`echo "$asm" | ./tmp/as_driver`
  [ "$actual" != "$expected" ] && encoding_failed "$asm" "$expected" "$actual"
}

# pushq %r64
test_encoding 'pushq %rax' '50'
test_encoding 'pushq %rsp' '54'
test_encoding 'pushq %rbp' '55'
test_encoding 'pushq %rdi' '57'
test_encoding 'pushq %r8' '41 50'
test_encoding 'pushq %r12' '41 54'
test_encoding 'pushq %r13' '41 55'
test_encoding 'pushq %r15' '41 57'

# popq %r64
test_encoding 'popq %rax' '58'
test_encoding 'popq %rsp' '5c'
test_encoding 'popq %rbp' '5d'
test_encoding 'popq %rdi' '5f'
test_encoding 'popq %r8' '41 58'
test_encoding 'popq %r12' '41 5c'
test_encoding 'popq %r13' '41 5d'
test_encoding 'popq %r15' '41 5f'

# leave
test_encoding 'leave' 'c9'

# ret
test_encoding 'ret' 'c3'

# movq $imm32, %r64
test_encoding 'movq $42, %rax' '48 c7 c0 2a 00 00 00'
test_encoding 'movq $42, %rsp' '48 c7 c4 2a 00 00 00'
test_encoding 'movq $42, %rbp' '48 c7 c5 2a 00 00 00'
test_encoding 'movq $42, %rdi' '48 c7 c7 2a 00 00 00'
test_encoding 'movq $42, %r8' '49 c7 c0 2a 00 00 00'
test_encoding 'movq $42, %r12' '49 c7 c4 2a 00 00 00'
test_encoding 'movq $42, %r13' '49 c7 c5 2a 00 00 00'
test_encoding 'movq $42, %r15' '49 c7 c7 2a 00 00 00'

# movq $imm32, (%r64)
test_encoding 'movq $42, (%rax)' '48 c7 00 2a 00 00 00'
test_encoding 'movq $42, (%rsp)' '48 c7 04 24 2a 00 00 00' # Scale: 0, Index: 4, Base: 4
test_encoding 'movq $42, (%rbp)' '48 c7 45 00 2a 00 00 00' # Mod: 1, disp8: 0
test_encoding 'movq $42, (%rdi)' '48 c7 07 2a 00 00 00'
test_encoding 'movq $42, (%r8)' '49 c7 00 2a 00 00 00'
test_encoding 'movq $42, (%r12)' '49 c7 04 24 2a 00 00 00' # Scale: 0, Index: 4, Base: 4
test_encoding 'movq $42, (%r13)' '49 c7 45 00 2a 00 00 00' # Mod: 1, disp8: 0
test_encoding 'movq $42, (%r15)' '49 c7 07 2a 00 00 00'

# movq $imm32, disp8(%r64)
test_encoding 'movq $42, 8(%rax)' '48 c7 40 08 2a 00 00 00'
test_encoding 'movq $42, 8(%rsp)' '48 c7 44 24 08 2a 00 00 00' # Scale: 0, Index: 4, Base: 4
test_encoding 'movq $42, 8(%rbp)' '48 c7 45 08 2a 00 00 00'
test_encoding 'movq $42, 8(%rdi)' '48 c7 47 08 2a 00 00 00'
test_encoding 'movq $42, 8(%r8)' '49 c7 40 08 2a 00 00 00'
test_encoding 'movq $42, 8(%r12)' '49 c7 44 24 08 2a 00 00 00' # Scale: 0, Index: 4, Base: 4
test_encoding 'movq $42, 8(%r13)' '49 c7 45 08 2a 00 00 00'
test_encoding 'movq $42, 8(%r15)' '49 c7 47 08 2a 00 00 00'

# movq $imm32, disp32(%r64)
test_encoding 'movq $42, 144(%rax)' '48 c7 80 90 00 00 00 2a 00 00 00'
test_encoding 'movq $42, 144(%rsp)' '48 c7 84 24 90 00 00 00 2a 00 00 00' # Scale: 0, Index: 4, Base: 4
test_encoding 'movq $42, 144(%rbp)' '48 c7 85 90 00 00 00 2a 00 00 00'
test_encoding 'movq $42, 144(%rdi)' '48 c7 87 90 00 00 00 2a 00 00 00'
test_encoding 'movq $42, 144(%r8)' '49 c7 80 90 00 00 00 2a 00 00 00'
test_encoding 'movq $42, 144(%r12)' '49 c7 84 24 90 00 00 00 2a 00 00 00' # Scale: 0, Index: 4, Base: 4
test_encoding 'movq $42, 144(%r13)' '49 c7 85 90 00 00 00 2a 00 00 00'
test_encoding 'movq $42, 144(%r15)' '49 c7 87 90 00 00 00 2a 00 00 00'

# movq $imm32, (%rax, %r64)
test_encoding 'movq $42, (%rax, %rax)' '48 c7 04 00 2a 00 00 00'
test_encoding 'movq $42, (%rax, %rbp)' '48 c7 04 28 2a 00 00 00'
test_encoding 'movq $42, (%rax, %rdi)' '48 c7 04 38 2a 00 00 00'
test_encoding 'movq $42, (%rax, %r8)' '4a c7 04 00 2a 00 00 00'
test_encoding 'movq $42, (%rax, %r12)' '4a c7 04 20 2a 00 00 00'
test_encoding 'movq $42, (%rax, %r13)' '4a c7 04 28 2a 00 00 00'
test_encoding 'movq $42, (%rax, %r15)' '4a c7 04 38 2a 00 00 00'

# movq $imm32, (%rsp, %r64)
test_encoding 'movq $42, (%rsp, %rax)' '48 c7 04 04 2a 00 00 00'
test_encoding 'movq $42, (%rsp, %rbp)' '48 c7 04 2c 2a 00 00 00'
test_encoding 'movq $42, (%rsp, %rdi)' '48 c7 04 3c 2a 00 00 00'
test_encoding 'movq $42, (%rsp, %r8)' '4a c7 04 04 2a 00 00 00'
test_encoding 'movq $42, (%rsp, %r12)' '4a c7 04 24 2a 00 00 00'
test_encoding 'movq $42, (%rsp, %r13)' '4a c7 04 2c 2a 00 00 00'
test_encoding 'movq $42, (%rsp, %r15)' '4a c7 04 3c 2a 00 00 00'

# movq $imm32, (%rbp, %r64)
test_encoding 'movq $42, (%rbp, %rax)' '48 c7 44 05 00 2a 00 00 00'
test_encoding 'movq $42, (%rbp, %rbp)' '48 c7 44 2d 00 2a 00 00 00'
test_encoding 'movq $42, (%rbp, %rdi)' '48 c7 44 3d 00 2a 00 00 00'
test_encoding 'movq $42, (%rbp, %r8)' '4a c7 44 05 00 2a 00 00 00'
test_encoding 'movq $42, (%rbp, %r12)' '4a c7 44 25 00 2a 00 00 00'
test_encoding 'movq $42, (%rbp, %r13)' '4a c7 44 2d 00 2a 00 00 00'
test_encoding 'movq $42, (%rbp, %r15)' '4a c7 44 3d 00 2a 00 00 00'

# movq $imm32, (%rdi, %r64)
test_encoding 'movq $42, (%rdi, %rax)' '48 c7 04 07 2a 00 00 00'
test_encoding 'movq $42, (%rdi, %rbp)' '48 c7 04 2f 2a 00 00 00'
test_encoding 'movq $42, (%rdi, %rdi)' '48 c7 04 3f 2a 00 00 00'
test_encoding 'movq $42, (%rdi, %r8)' '4a c7 04 07 2a 00 00 00'
test_encoding 'movq $42, (%rdi, %r12)' '4a c7 04 27 2a 00 00 00'
test_encoding 'movq $42, (%rdi, %r13)' '4a c7 04 2f 2a 00 00 00'
test_encoding 'movq $42, (%rdi, %r15)' '4a c7 04 3f 2a 00 00 00'

# movq $imm32, (%8, %r64)
test_encoding 'movq $42, (%r8, %rax)' '49 c7 04 00 2a 00 00 00'
test_encoding 'movq $42, (%r8, %rbp)' '49 c7 04 28 2a 00 00 00'
test_encoding 'movq $42, (%r8, %rdi)' '49 c7 04 38 2a 00 00 00'
test_encoding 'movq $42, (%r8, %r8)' '4b c7 04 00 2a 00 00 00'
test_encoding 'movq $42, (%r8, %r12)' '4b c7 04 20 2a 00 00 00'
test_encoding 'movq $42, (%r8, %r13)' '4b c7 04 28 2a 00 00 00'
test_encoding 'movq $42, (%r8, %r15)' '4b c7 04 38 2a 00 00 00'

# movq $imm32, (%r12, %r64)
test_encoding 'movq $42, (%r12, %rax)' '49 c7 04 04 2a 00 00 00'
test_encoding 'movq $42, (%r12, %rbp)' '49 c7 04 2c 2a 00 00 00'
test_encoding 'movq $42, (%r12, %rdi)' '49 c7 04 3c 2a 00 00 00'
test_encoding 'movq $42, (%r12, %r8)' '4b c7 04 04 2a 00 00 00'
test_encoding 'movq $42, (%r12, %r12)' '4b c7 04 24 2a 00 00 00'
test_encoding 'movq $42, (%r12, %r13)' '4b c7 04 2c 2a 00 00 00'
test_encoding 'movq $42, (%r12, %r15)' '4b c7 04 3c 2a 00 00 00'

# movq $imm32, (%r13, %r64)
test_encoding 'movq $42, (%r13, %rax)' '49 c7 44 05 00 2a 00 00 00'
test_encoding 'movq $42, (%r13, %rbp)' '49 c7 44 2d 00 2a 00 00 00'
test_encoding 'movq $42, (%r13, %rdi)' '49 c7 44 3d 00 2a 00 00 00'
test_encoding 'movq $42, (%r13, %r8)' '4b c7 44 05 00 2a 00 00 00'
test_encoding 'movq $42, (%r13, %r12)' '4b c7 44 25 00 2a 00 00 00'
test_encoding 'movq $42, (%r13, %r13)' '4b c7 44 2d 00 2a 00 00 00'
test_encoding 'movq $42, (%r13, %r15)' '4b c7 44 3d 00 2a 00 00 00'

# movq $imm32, (%r15, %r64)
test_encoding 'movq $42, (%r15, %rax)' '49 c7 04 07 2a 00 00 00'
test_encoding 'movq $42, (%r15, %rbp)' '49 c7 04 2f 2a 00 00 00'
test_encoding 'movq $42, (%r15, %rdi)' '49 c7 04 3f 2a 00 00 00'
test_encoding 'movq $42, (%r15, %r8)' '4b c7 04 07 2a 00 00 00'
test_encoding 'movq $42, (%r15, %r12)' '4b c7 04 27 2a 00 00 00'
test_encoding 'movq $42, (%r15, %r13)' '4b c7 04 2f 2a 00 00 00'
test_encoding 'movq $42, (%r15, %r15)' '4b c7 04 3f 2a 00 00 00'

# movq $imm32, (%rax, %r64, 2)
test_encoding 'movq $42, (%rax, %rax, 2)' '48 c7 04 40 2a 00 00 00'
test_encoding 'movq $42, (%rax, %rbp, 2)' '48 c7 04 68 2a 00 00 00'
test_encoding 'movq $42, (%rax, %rdi, 2)' '48 c7 04 78 2a 00 00 00'
test_encoding 'movq $42, (%rax, %r8, 2)' '4a c7 04 40 2a 00 00 00'
test_encoding 'movq $42, (%rax, %r12, 2)' '4a c7 04 60 2a 00 00 00'
test_encoding 'movq $42, (%rax, %r13, 2)' '4a c7 04 68 2a 00 00 00'
test_encoding 'movq $42, (%rax, %r15, 2)' '4a c7 04 78 2a 00 00 00'

# movq $imm32, (%rsp, %r64, 2)
test_encoding 'movq $42, (%rsp, %rax, 2)' '48 c7 04 44 2a 00 00 00'
test_encoding 'movq $42, (%rsp, %rbp, 2)' '48 c7 04 6c 2a 00 00 00'
test_encoding 'movq $42, (%rsp, %rdi, 2)' '48 c7 04 7c 2a 00 00 00'
test_encoding 'movq $42, (%rsp, %r8, 2)' '4a c7 04 44 2a 00 00 00'
test_encoding 'movq $42, (%rsp, %r12, 2)' '4a c7 04 64 2a 00 00 00'
test_encoding 'movq $42, (%rsp, %r13, 2)' '4a c7 04 6c 2a 00 00 00'
test_encoding 'movq $42, (%rsp, %r15, 2)' '4a c7 04 7c 2a 00 00 00'

# movq $imm32, (%rbp, %r64, 2)
test_encoding 'movq $42, (%rbp, %rax, 2)' '48 c7 44 45 00 2a 00 00 00'
test_encoding 'movq $42, (%rbp, %rbp, 2)' '48 c7 44 6d 00 2a 00 00 00'
test_encoding 'movq $42, (%rbp, %rdi, 2)' '48 c7 44 7d 00 2a 00 00 00'
test_encoding 'movq $42, (%rbp, %r8, 2)' '4a c7 44 45 00 2a 00 00 00'
test_encoding 'movq $42, (%rbp, %r12, 2)' '4a c7 44 65 00 2a 00 00 00'
test_encoding 'movq $42, (%rbp, %r13, 2)' '4a c7 44 6d 00 2a 00 00 00'
test_encoding 'movq $42, (%rbp, %r15, 2)' '4a c7 44 7d 00 2a 00 00 00'

# movq $imm32, (%rdi, %r64, 2)
test_encoding 'movq $42, (%rdi, %rax, 2)' '48 c7 04 47 2a 00 00 00'
test_encoding 'movq $42, (%rdi, %rbp, 2)' '48 c7 04 6f 2a 00 00 00'
test_encoding 'movq $42, (%rdi, %rdi, 2)' '48 c7 04 7f 2a 00 00 00'
test_encoding 'movq $42, (%rdi, %r8, 2)' '4a c7 04 47 2a 00 00 00'
test_encoding 'movq $42, (%rdi, %r12, 2)' '4a c7 04 67 2a 00 00 00'
test_encoding 'movq $42, (%rdi, %r13, 2)' '4a c7 04 6f 2a 00 00 00'
test_encoding 'movq $42, (%rdi, %r15, 2)' '4a c7 04 7f 2a 00 00 00'

# movq $imm32, (%8, %r64, 2)
test_encoding 'movq $42, (%r8, %rax, 2)' '49 c7 04 40 2a 00 00 00'
test_encoding 'movq $42, (%r8, %rbp, 2)' '49 c7 04 68 2a 00 00 00'
test_encoding 'movq $42, (%r8, %rdi, 2)' '49 c7 04 78 2a 00 00 00'
test_encoding 'movq $42, (%r8, %r8, 2)' '4b c7 04 40 2a 00 00 00'
test_encoding 'movq $42, (%r8, %r12, 2)' '4b c7 04 60 2a 00 00 00'
test_encoding 'movq $42, (%r8, %r13, 2)' '4b c7 04 68 2a 00 00 00'
test_encoding 'movq $42, (%r8, %r15, 2)' '4b c7 04 78 2a 00 00 00'

# movq $imm32, (%r12, %r64, 2)
test_encoding 'movq $42, (%r12, %rax, 2)' '49 c7 04 44 2a 00 00 00'
test_encoding 'movq $42, (%r12, %rbp, 2)' '49 c7 04 6c 2a 00 00 00'
test_encoding 'movq $42, (%r12, %rdi, 2)' '49 c7 04 7c 2a 00 00 00'
test_encoding 'movq $42, (%r12, %r8, 2)' '4b c7 04 44 2a 00 00 00'
test_encoding 'movq $42, (%r12, %r12, 2)' '4b c7 04 64 2a 00 00 00'
test_encoding 'movq $42, (%r12, %r13, 2)' '4b c7 04 6c 2a 00 00 00'
test_encoding 'movq $42, (%r12, %r15, 2)' '4b c7 04 7c 2a 00 00 00'

# movq $imm32, (%r13, %r64, 2)
test_encoding 'movq $42, (%r13, %rax, 2)' '49 c7 44 45 00 2a 00 00 00'
test_encoding 'movq $42, (%r13, %rbp, 2)' '49 c7 44 6d 00 2a 00 00 00'
test_encoding 'movq $42, (%r13, %rdi, 2)' '49 c7 44 7d 00 2a 00 00 00'
test_encoding 'movq $42, (%r13, %r8, 2)' '4b c7 44 45 00 2a 00 00 00'
test_encoding 'movq $42, (%r13, %r12, 2)' '4b c7 44 65 00 2a 00 00 00'
test_encoding 'movq $42, (%r13, %r13, 2)' '4b c7 44 6d 00 2a 00 00 00'
test_encoding 'movq $42, (%r13, %r15, 2)' '4b c7 44 7d 00 2a 00 00 00'

# movq $imm32, (%r15, %r64, 2)
test_encoding 'movq $42, (%r15, %rax, 2)' '49 c7 04 47 2a 00 00 00'
test_encoding 'movq $42, (%r15, %rbp, 2)' '49 c7 04 6f 2a 00 00 00'
test_encoding 'movq $42, (%r15, %rdi, 2)' '49 c7 04 7f 2a 00 00 00'
test_encoding 'movq $42, (%r15, %r8, 2)' '4b c7 04 47 2a 00 00 00'
test_encoding 'movq $42, (%r15, %r12, 2)' '4b c7 04 67 2a 00 00 00'
test_encoding 'movq $42, (%r15, %r13, 2)' '4b c7 04 6f 2a 00 00 00'
test_encoding 'movq $42, (%r15, %r15, 2)' '4b c7 04 7f 2a 00 00 00'

# movq $imm32, (%rax, %r64, 4)
test_encoding 'movq $42, (%rax, %rax, 4)' '48 c7 04 80 2a 00 00 00'
test_encoding 'movq $42, (%rax, %rbp, 4)' '48 c7 04 a8 2a 00 00 00'
test_encoding 'movq $42, (%rax, %rdi, 4)' '48 c7 04 b8 2a 00 00 00'
test_encoding 'movq $42, (%rax, %r8, 4)' '4a c7 04 80 2a 00 00 00'
test_encoding 'movq $42, (%rax, %r12, 4)' '4a c7 04 a0 2a 00 00 00'
test_encoding 'movq $42, (%rax, %r13, 4)' '4a c7 04 a8 2a 00 00 00'
test_encoding 'movq $42, (%rax, %r15, 4)' '4a c7 04 b8 2a 00 00 00'

# movq $imm32, (%rsp, %r64, 4)
test_encoding 'movq $42, (%rsp, %rax, 4)' '48 c7 04 84 2a 00 00 00'
test_encoding 'movq $42, (%rsp, %rbp, 4)' '48 c7 04 ac 2a 00 00 00'
test_encoding 'movq $42, (%rsp, %rdi, 4)' '48 c7 04 bc 2a 00 00 00'
test_encoding 'movq $42, (%rsp, %r8, 4)' '4a c7 04 84 2a 00 00 00'
test_encoding 'movq $42, (%rsp, %r12, 4)' '4a c7 04 a4 2a 00 00 00'
test_encoding 'movq $42, (%rsp, %r13, 4)' '4a c7 04 ac 2a 00 00 00'
test_encoding 'movq $42, (%rsp, %r15, 4)' '4a c7 04 bc 2a 00 00 00'

# movq $imm32, (%rbp, %r64, 4)
test_encoding 'movq $42, (%rbp, %rax, 4)' '48 c7 44 85 00 2a 00 00 00'
test_encoding 'movq $42, (%rbp, %rbp, 4)' '48 c7 44 ad 00 2a 00 00 00'
test_encoding 'movq $42, (%rbp, %rdi, 4)' '48 c7 44 bd 00 2a 00 00 00'
test_encoding 'movq $42, (%rbp, %r8, 4)' '4a c7 44 85 00 2a 00 00 00'
test_encoding 'movq $42, (%rbp, %r12, 4)' '4a c7 44 a5 00 2a 00 00 00'
test_encoding 'movq $42, (%rbp, %r13, 4)' '4a c7 44 ad 00 2a 00 00 00'
test_encoding 'movq $42, (%rbp, %r15, 4)' '4a c7 44 bd 00 2a 00 00 00'

# movq $imm32, (%rdi, %r64, 4)
test_encoding 'movq $42, (%rdi, %rax, 4)' '48 c7 04 87 2a 00 00 00'
test_encoding 'movq $42, (%rdi, %rbp, 4)' '48 c7 04 af 2a 00 00 00'
test_encoding 'movq $42, (%rdi, %rdi, 4)' '48 c7 04 bf 2a 00 00 00'
test_encoding 'movq $42, (%rdi, %r8, 4)' '4a c7 04 87 2a 00 00 00'
test_encoding 'movq $42, (%rdi, %r12, 4)' '4a c7 04 a7 2a 00 00 00'
test_encoding 'movq $42, (%rdi, %r13, 4)' '4a c7 04 af 2a 00 00 00'
test_encoding 'movq $42, (%rdi, %r15, 4)' '4a c7 04 bf 2a 00 00 00'

# movq $imm32, (%8, %r64, 4)
test_encoding 'movq $42, (%r8, %rax, 4)' '49 c7 04 80 2a 00 00 00'
test_encoding 'movq $42, (%r8, %rbp, 4)' '49 c7 04 a8 2a 00 00 00'
test_encoding 'movq $42, (%r8, %rdi, 4)' '49 c7 04 b8 2a 00 00 00'
test_encoding 'movq $42, (%r8, %r8, 4)' '4b c7 04 80 2a 00 00 00'
test_encoding 'movq $42, (%r8, %r12, 4)' '4b c7 04 a0 2a 00 00 00'
test_encoding 'movq $42, (%r8, %r13, 4)' '4b c7 04 a8 2a 00 00 00'
test_encoding 'movq $42, (%r8, %r15, 4)' '4b c7 04 b8 2a 00 00 00'

# movq $imm32, (%r12, %r64, 4)
test_encoding 'movq $42, (%r12, %rax, 4)' '49 c7 04 84 2a 00 00 00'
test_encoding 'movq $42, (%r12, %rbp, 4)' '49 c7 04 ac 2a 00 00 00'
test_encoding 'movq $42, (%r12, %rdi, 4)' '49 c7 04 bc 2a 00 00 00'
test_encoding 'movq $42, (%r12, %r8, 4)' '4b c7 04 84 2a 00 00 00'
test_encoding 'movq $42, (%r12, %r12, 4)' '4b c7 04 a4 2a 00 00 00'
test_encoding 'movq $42, (%r12, %r13, 4)' '4b c7 04 ac 2a 00 00 00'
test_encoding 'movq $42, (%r12, %r15, 4)' '4b c7 04 bc 2a 00 00 00'

# movq $imm32, (%r13, %r64, 4)
test_encoding 'movq $42, (%r13, %rax, 4)' '49 c7 44 85 00 2a 00 00 00'
test_encoding 'movq $42, (%r13, %rbp, 4)' '49 c7 44 ad 00 2a 00 00 00'
test_encoding 'movq $42, (%r13, %rdi, 4)' '49 c7 44 bd 00 2a 00 00 00'
test_encoding 'movq $42, (%r13, %r8, 4)' '4b c7 44 85 00 2a 00 00 00'
test_encoding 'movq $42, (%r13, %r12, 4)' '4b c7 44 a5 00 2a 00 00 00'
test_encoding 'movq $42, (%r13, %r13, 4)' '4b c7 44 ad 00 2a 00 00 00'
test_encoding 'movq $42, (%r13, %r15, 4)' '4b c7 44 bd 00 2a 00 00 00'

# movq $imm32, (%r15, %r64, 4)
test_encoding 'movq $42, (%r15, %rax, 4)' '49 c7 04 87 2a 00 00 00'
test_encoding 'movq $42, (%r15, %rbp, 4)' '49 c7 04 af 2a 00 00 00'
test_encoding 'movq $42, (%r15, %rdi, 4)' '49 c7 04 bf 2a 00 00 00'
test_encoding 'movq $42, (%r15, %r8, 4)' '4b c7 04 87 2a 00 00 00'
test_encoding 'movq $42, (%r15, %r12, 4)' '4b c7 04 a7 2a 00 00 00'
test_encoding 'movq $42, (%r15, %r13, 4)' '4b c7 04 af 2a 00 00 00'
test_encoding 'movq $42, (%r15, %r15, 4)' '4b c7 04 bf 2a 00 00 00'

# movq $imm32, (%rax, %r64, 8)
test_encoding 'movq $42, (%rax, %rax, 8)' '48 c7 04 c0 2a 00 00 00'
test_encoding 'movq $42, (%rax, %rbp, 8)' '48 c7 04 e8 2a 00 00 00'
test_encoding 'movq $42, (%rax, %rdi, 8)' '48 c7 04 f8 2a 00 00 00'
test_encoding 'movq $42, (%rax, %r8, 8)' '4a c7 04 c0 2a 00 00 00'
test_encoding 'movq $42, (%rax, %r12, 8)' '4a c7 04 e0 2a 00 00 00'
test_encoding 'movq $42, (%rax, %r13, 8)' '4a c7 04 e8 2a 00 00 00'
test_encoding 'movq $42, (%rax, %r15, 8)' '4a c7 04 f8 2a 00 00 00'

# movq $imm32, (%rsp, %r64, 8)
test_encoding 'movq $42, (%rsp, %rax, 8)' '48 c7 04 c4 2a 00 00 00'
test_encoding 'movq $42, (%rsp, %rbp, 8)' '48 c7 04 ec 2a 00 00 00'
test_encoding 'movq $42, (%rsp, %rdi, 8)' '48 c7 04 fc 2a 00 00 00'
test_encoding 'movq $42, (%rsp, %r8, 8)' '4a c7 04 c4 2a 00 00 00'
test_encoding 'movq $42, (%rsp, %r12, 8)' '4a c7 04 e4 2a 00 00 00'
test_encoding 'movq $42, (%rsp, %r13, 8)' '4a c7 04 ec 2a 00 00 00'
test_encoding 'movq $42, (%rsp, %r15, 8)' '4a c7 04 fc 2a 00 00 00'

# movq $imm32, (%rbp, %r64, 8)
test_encoding 'movq $42, (%rbp, %rax, 8)' '48 c7 44 c5 00 2a 00 00 00'
test_encoding 'movq $42, (%rbp, %rbp, 8)' '48 c7 44 ed 00 2a 00 00 00'
test_encoding 'movq $42, (%rbp, %rdi, 8)' '48 c7 44 fd 00 2a 00 00 00'
test_encoding 'movq $42, (%rbp, %r8, 8)' '4a c7 44 c5 00 2a 00 00 00'
test_encoding 'movq $42, (%rbp, %r12, 8)' '4a c7 44 e5 00 2a 00 00 00'
test_encoding 'movq $42, (%rbp, %r13, 8)' '4a c7 44 ed 00 2a 00 00 00'
test_encoding 'movq $42, (%rbp, %r15, 8)' '4a c7 44 fd 00 2a 00 00 00'

# movq $imm32, (%rdi, %r64, 8)
test_encoding 'movq $42, (%rdi, %rax, 8)' '48 c7 04 c7 2a 00 00 00'
test_encoding 'movq $42, (%rdi, %rbp, 8)' '48 c7 04 ef 2a 00 00 00'
test_encoding 'movq $42, (%rdi, %rdi, 8)' '48 c7 04 ff 2a 00 00 00'
test_encoding 'movq $42, (%rdi, %r8, 8)' '4a c7 04 c7 2a 00 00 00'
test_encoding 'movq $42, (%rdi, %r12, 8)' '4a c7 04 e7 2a 00 00 00'
test_encoding 'movq $42, (%rdi, %r13, 8)' '4a c7 04 ef 2a 00 00 00'
test_encoding 'movq $42, (%rdi, %r15, 8)' '4a c7 04 ff 2a 00 00 00'

# movq $imm32, (%8, %r64, 8)
test_encoding 'movq $42, (%r8, %rax, 8)' '49 c7 04 c0 2a 00 00 00'
test_encoding 'movq $42, (%r8, %rbp, 8)' '49 c7 04 e8 2a 00 00 00'
test_encoding 'movq $42, (%r8, %rdi, 8)' '49 c7 04 f8 2a 00 00 00'
test_encoding 'movq $42, (%r8, %r8, 8)' '4b c7 04 c0 2a 00 00 00'
test_encoding 'movq $42, (%r8, %r12, 8)' '4b c7 04 e0 2a 00 00 00'
test_encoding 'movq $42, (%r8, %r13, 8)' '4b c7 04 e8 2a 00 00 00'
test_encoding 'movq $42, (%r8, %r15, 8)' '4b c7 04 f8 2a 00 00 00'

# movq $imm32, (%r12, %r64, 8)
test_encoding 'movq $42, (%r12, %rax, 8)' '49 c7 04 c4 2a 00 00 00'
test_encoding 'movq $42, (%r12, %rbp, 8)' '49 c7 04 ec 2a 00 00 00'
test_encoding 'movq $42, (%r12, %rdi, 8)' '49 c7 04 fc 2a 00 00 00'
test_encoding 'movq $42, (%r12, %r8, 8)' '4b c7 04 c4 2a 00 00 00'
test_encoding 'movq $42, (%r12, %r12, 8)' '4b c7 04 e4 2a 00 00 00'
test_encoding 'movq $42, (%r12, %r13, 8)' '4b c7 04 ec 2a 00 00 00'
test_encoding 'movq $42, (%r12, %r15, 8)' '4b c7 04 fc 2a 00 00 00'

# movq $imm32, (%r13, %r64, 8)
test_encoding 'movq $42, (%r13, %rax, 8)' '49 c7 44 c5 00 2a 00 00 00'
test_encoding 'movq $42, (%r13, %rbp, 8)' '49 c7 44 ed 00 2a 00 00 00'
test_encoding 'movq $42, (%r13, %rdi, 8)' '49 c7 44 fd 00 2a 00 00 00'
test_encoding 'movq $42, (%r13, %r8, 8)' '4b c7 44 c5 00 2a 00 00 00'
test_encoding 'movq $42, (%r13, %r12, 8)' '4b c7 44 e5 00 2a 00 00 00'
test_encoding 'movq $42, (%r13, %r13, 8)' '4b c7 44 ed 00 2a 00 00 00'
test_encoding 'movq $42, (%r13, %r15, 8)' '4b c7 44 fd 00 2a 00 00 00'

# movq $imm32, (%r15, %r64, 8)
test_encoding 'movq $42, (%r15, %rax, 8)' '49 c7 04 c7 2a 00 00 00'
test_encoding 'movq $42, (%r15, %rbp, 8)' '49 c7 04 ef 2a 00 00 00'
test_encoding 'movq $42, (%r15, %rdi, 8)' '49 c7 04 ff 2a 00 00 00'
test_encoding 'movq $42, (%r15, %r8, 8)' '4b c7 04 c7 2a 00 00 00'
test_encoding 'movq $42, (%r15, %r12, 8)' '4b c7 04 e7 2a 00 00 00'
test_encoding 'movq $42, (%r15, %r13, 8)' '4b c7 04 ef 2a 00 00 00'
test_encoding 'movq $42, (%r15, %r15, 8)' '4b c7 04 ff 2a 00 00 00'

# movq %rcx, %r64
test_encoding 'movq %rcx, %rax' '48 89 c8'
test_encoding 'movq %rcx, %rsp' '48 89 cc'
test_encoding 'movq %rcx, %rbp' '48 89 cd'
test_encoding 'movq %rcx, %rdi' '48 89 cf'
test_encoding 'movq %rcx, %r8' '49 89 c8'
test_encoding 'movq %rcx, %r12' '49 89 cc'
test_encoding 'movq %rcx, %r13' '49 89 cd'
test_encoding 'movq %rcx, %r15' '49 89 cf'

# movq %r9, %r64
test_encoding 'movq %r9, %rax' '4c 89 c8'
test_encoding 'movq %r9, %rsp' '4c 89 cc'
test_encoding 'movq %r9, %rbp' '4c 89 cd'
test_encoding 'movq %r9, %rdi' '4c 89 cf'
test_encoding 'movq %r9, %r8' '4d 89 c8'
test_encoding 'movq %r9, %r12' '4d 89 cc'
test_encoding 'movq %r9, %r13' '4d 89 cd'
test_encoding 'movq %r9, %r15' '4d 89 cf'

# movq %r64, %rcx
test_encoding 'movq %rax, %rcx' '48 89 c1'
test_encoding 'movq %rsp, %rcx' '48 89 e1'
test_encoding 'movq %rbp, %rcx' '48 89 e9'
test_encoding 'movq %rdi, %rcx' '48 89 f9'
test_encoding 'movq %r8, %rcx' '4c 89 c1'
test_encoding 'movq %r12, %rcx' '4c 89 e1'
test_encoding 'movq %r13, %rcx' '4c 89 e9'
test_encoding 'movq %r15, %rcx' '4c 89 f9'

# movq %r64, %r9
test_encoding 'movq %rax, %r9' '49 89 c1'
test_encoding 'movq %rsp, %r9' '49 89 e1'
test_encoding 'movq %rbp, %r9' '49 89 e9'
test_encoding 'movq %rdi, %r9' '49 89 f9'
test_encoding 'movq %r8, %r9' '4d 89 c1'
test_encoding 'movq %r12, %r9' '4d 89 e1'
test_encoding 'movq %r13, %r9' '4d 89 e9'
test_encoding 'movq %r15, %r9' '4d 89 f9'

# movq %rcx, (%r64)
test_encoding 'movq %rcx, (%rax)' '48 89 08'
test_encoding 'movq %rcx, (%rsp)' '48 89 0c 24' # Scale: 0, Index: 4, Base: 4
test_encoding 'movq %rcx, (%rbp)' '48 89 4d 00' # Mod: 1, disp8: 0
test_encoding 'movq %rcx, (%rdi)' '48 89 0f'
test_encoding 'movq %rcx, (%r8)' '49 89 08'
test_encoding 'movq %rcx, (%r12)' '49 89 0c 24' # Scale: 0, Index: 4, Base: 4
test_encoding 'movq %rcx, (%r13)' '49 89 4d 00' # Mod: 1, disp8: 0
test_encoding 'movq %rcx, (%r15)' '49 89 0f'

# movq %rcx, (%rsp, %r64)
test_encoding 'movq %rcx, (%rsp, %rax)' '48 89 0c 04'
test_encoding 'movq %rcx, (%rsp, %rbp)' '48 89 0c 2c'
test_encoding 'movq %rcx, (%rsp, %rdi)' '48 89 0c 3c'
test_encoding 'movq %rcx, (%rsp, %r8)' '4a 89 0c 04'
test_encoding 'movq %rcx, (%rsp, %r12)' '4a 89 0c 24'
test_encoding 'movq %rcx, (%rsp, %r13)' '4a 89 0c 2c'
test_encoding 'movq %rcx, (%rsp, %r15)' '4a 89 0c 3c'

# movq %rcx, (%rbp, %r64)
test_encoding 'movq %rcx, (%rbp, %rax)' '48 89 4c 05 00'
test_encoding 'movq %rcx, (%rbp, %rbp)' '48 89 4c 2d 00'
test_encoding 'movq %rcx, (%rbp, %rdi)' '48 89 4c 3d 00'
test_encoding 'movq %rcx, (%rbp, %r8)' '4a 89 4c 05 00'
test_encoding 'movq %rcx, (%rbp, %r12)' '4a 89 4c 25 00'
test_encoding 'movq %rcx, (%rbp, %r13)' '4a 89 4c 2d 00'
test_encoding 'movq %rcx, (%rbp, %r15)' '4a 89 4c 3d 00'

# movq %rcx, (%r12, %r64)
test_encoding 'movq %rcx, (%r12, %rax)' '49 89 0c 04'
test_encoding 'movq %rcx, (%r12, %rbp)' '49 89 0c 2c'
test_encoding 'movq %rcx, (%r12, %rdi)' '49 89 0c 3c'
test_encoding 'movq %rcx, (%r12, %r8)' '4b 89 0c 04'
test_encoding 'movq %rcx, (%r12, %r12)' '4b 89 0c 24'
test_encoding 'movq %rcx, (%r12, %r13)' '4b 89 0c 2c'
test_encoding 'movq %rcx, (%r12, %r15)' '4b 89 0c 3c'

# movq %rcx, (%r13, %r64)
test_encoding 'movq %rcx, (%r13, %rax)' '49 89 4c 05 00'
test_encoding 'movq %rcx, (%r13, %rbp)' '49 89 4c 2d 00'
test_encoding 'movq %rcx, (%r13, %rdi)' '49 89 4c 3d 00'
test_encoding 'movq %rcx, (%r13, %r8)' '4b 89 4c 05 00'
test_encoding 'movq %rcx, (%r13, %r12)' '4b 89 4c 25 00'
test_encoding 'movq %rcx, (%r13, %r13)' '4b 89 4c 2d 00'
test_encoding 'movq %rcx, (%r13, %r15)' '4b 89 4c 3d 00'

# movq %r9, (%r64)
test_encoding 'movq %r9, (%rax)' '4c 89 08'
test_encoding 'movq %r9, (%rsp)' '4c 89 0c 24' # Scale: 0, Index: 4, Base: 4
test_encoding 'movq %r9, (%rbp)' '4c 89 4d 00' # Mod: 1, disp8: 0
test_encoding 'movq %r9, (%rdi)' '4c 89 0f'
test_encoding 'movq %r9, (%r8)' '4d 89 08'
test_encoding 'movq %r9, (%r12)' '4d 89 0c 24' # Scale: 0, Index: 4, Base: 4
test_encoding 'movq %r9, (%r13)' '4d 89 4d 00' # Mod: 1, disp8: 0
test_encoding 'movq %r9, (%r15)' '4d 89 0f'

# movq %r9, (%rsp, %r64)
test_encoding 'movq %r9, (%rsp, %rax)' '4c 89 0c 04'
test_encoding 'movq %r9, (%rsp, %rbp)' '4c 89 0c 2c'
test_encoding 'movq %r9, (%rsp, %rdi)' '4c 89 0c 3c'
test_encoding 'movq %r9, (%rsp, %r8)' '4e 89 0c 04'
test_encoding 'movq %r9, (%rsp, %r12)' '4e 89 0c 24'
test_encoding 'movq %r9, (%rsp, %r13)' '4e 89 0c 2c'
test_encoding 'movq %r9, (%rsp, %r15)' '4e 89 0c 3c'

# movq %r9, (%rbp, %r64)
test_encoding 'movq %r9, (%rbp, %rax)' '4c 89 4c 05 00'
test_encoding 'movq %r9, (%rbp, %rbp)' '4c 89 4c 2d 00'
test_encoding 'movq %r9, (%rbp, %rdi)' '4c 89 4c 3d 00'
test_encoding 'movq %r9, (%rbp, %r8)' '4e 89 4c 05 00'
test_encoding 'movq %r9, (%rbp, %r12)' '4e 89 4c 25 00'
test_encoding 'movq %r9, (%rbp, %r13)' '4e 89 4c 2d 00'
test_encoding 'movq %r9, (%rbp, %r15)' '4e 89 4c 3d 00'

# movq %r9, (%r12, %r64)
test_encoding 'movq %r9, (%r12, %rax)' '4d 89 0c 04'
test_encoding 'movq %r9, (%r12, %rbp)' '4d 89 0c 2c'
test_encoding 'movq %r9, (%r12, %rdi)' '4d 89 0c 3c'
test_encoding 'movq %r9, (%r12, %r8)' '4f 89 0c 04'
test_encoding 'movq %r9, (%r12, %r12)' '4f 89 0c 24'
test_encoding 'movq %r9, (%r12, %r13)' '4f 89 0c 2c'
test_encoding 'movq %r9, (%r12, %r15)' '4f 89 0c 3c'

# movq %r9, (%r13, %r64)
test_encoding 'movq %r9, (%r13, %rax)' '4d 89 4c 05 00'
test_encoding 'movq %r9, (%r13, %rbp)' '4d 89 4c 2d 00'
test_encoding 'movq %r9, (%r13, %rdi)' '4d 89 4c 3d 00'
test_encoding 'movq %r9, (%r13, %r8)' '4f 89 4c 05 00'
test_encoding 'movq %r9, (%r13, %r12)' '4f 89 4c 25 00'
test_encoding 'movq %r9, (%r13, %r13)' '4f 89 4c 2d 00'
test_encoding 'movq %r9, (%r13, %r15)' '4f 89 4c 3d 00'

# movq (%r64), %rcx
test_encoding 'movq (%rax), %rcx' '48 8b 08'
test_encoding 'movq (%rsp), %rcx' '48 8b 0c 24' # Scale: 0, Index: 4, Base: 4
test_encoding 'movq (%rbp), %rcx' '48 8b 4d 00' # Mod: 1, disp8: 0
test_encoding 'movq (%rdi), %rcx' '48 8b 0f'
test_encoding 'movq (%r8), %rcx' '49 8b 08'
test_encoding 'movq (%r12), %rcx' '49 8b 0c 24' # Scale: 0, Index: 4, Base: 4
test_encoding 'movq (%r13), %rcx' '49 8b 4d 00' # Mod: 1, disp8: 0
test_encoding 'movq (%r15), %rcx' '49 8b 0f'

# movq (%rsp, %r64), %rcx
test_encoding 'movq (%rsp, %rax), %rcx' '48 8b 0c 04'
test_encoding 'movq (%rsp, %rbp), %rcx' '48 8b 0c 2c'
test_encoding 'movq (%rsp, %rdi), %rcx' '48 8b 0c 3c'
test_encoding 'movq (%rsp, %r8), %rcx'  '4a 8b 0c 04'
test_encoding 'movq (%rsp, %r12), %rcx' '4a 8b 0c 24'
test_encoding 'movq (%rsp, %r13), %rcx' '4a 8b 0c 2c'
test_encoding 'movq (%rsp, %r15), %rcx' '4a 8b 0c 3c'

# movq (%rbp, %r64), %rcx
test_encoding 'movq (%rbp, %rax), %rcx' '48 8b 4c 05 00'
test_encoding 'movq (%rbp, %rbp), %rcx' '48 8b 4c 2d 00'
test_encoding 'movq (%rbp, %rdi), %rcx' '48 8b 4c 3d 00'
test_encoding 'movq (%rbp, %r8), %rcx' '4a 8b 4c 05 00'
test_encoding 'movq (%rbp, %r12), %rcx' '4a 8b 4c 25 00'
test_encoding 'movq (%rbp, %r13), %rcx' '4a 8b 4c 2d 00'
test_encoding 'movq (%rbp, %r15), %rcx' '4a 8b 4c 3d 00'

# movq (%r12, %r64), %rcx
test_encoding 'movq (%r12, %rax), %rcx' '49 8b 0c 04'
test_encoding 'movq (%r12, %rbp), %rcx' '49 8b 0c 2c'
test_encoding 'movq (%r12, %rdi), %rcx' '49 8b 0c 3c'
test_encoding 'movq (%r12, %r8), %rcx' '4b 8b 0c 04'
test_encoding 'movq (%r12, %r12), %rcx' '4b 8b 0c 24'
test_encoding 'movq (%r12, %r13), %rcx' '4b 8b 0c 2c'
test_encoding 'movq (%r12, %r15), %rcx' '4b 8b 0c 3c'

# movq (%r13, %r64), %rcx
test_encoding 'movq (%r13, %rax), %rcx' '49 8b 4c 05 00'
test_encoding 'movq (%r13, %rbp), %rcx' '49 8b 4c 2d 00'
test_encoding 'movq (%r13, %rdi), %rcx' '49 8b 4c 3d 00'
test_encoding 'movq (%r13, %r8), %rcx' '4b 8b 4c 05 00'
test_encoding 'movq (%r13, %r12), %rcx' '4b 8b 4c 25 00'
test_encoding 'movq (%r13, %r13), %rcx' '4b 8b 4c 2d 00'
test_encoding 'movq (%r13, %r15), %rcx' '4b 8b 4c 3d 00'

# movq (%r64), %r9
test_encoding 'movq (%rax), %r9' '4c 8b 08'
test_encoding 'movq (%rsp), %r9' '4c 8b 0c 24' # Scale: 0, Index: 4, Base: 4
test_encoding 'movq (%rbp), %r9' '4c 8b 4d 00' # Mod: 1, disp8: 0
test_encoding 'movq (%rdi), %r9' '4c 8b 0f'
test_encoding 'movq (%r8), %r9' '4d 8b 08'
test_encoding 'movq (%r12), %r9' '4d 8b 0c 24' # Scale: 0, Index: 4, Base: 4
test_encoding 'movq (%r13), %r9' '4d 8b 4d 00' # Mod: 1, disp8: 0
test_encoding 'movq (%r15), %r9' '4d 8b 0f'

# movq (%rsp, %r64), %r9
test_encoding 'movq (%rsp, %rax), %r9' '4c 8b 0c 04'
test_encoding 'movq (%rsp, %rbp), %r9' '4c 8b 0c 2c'
test_encoding 'movq (%rsp, %rdi), %r9' '4c 8b 0c 3c'
test_encoding 'movq (%rsp, %r8), %r9' '4e 8b 0c 04'
test_encoding 'movq (%rsp, %r12), %r9' '4e 8b 0c 24'
test_encoding 'movq (%rsp, %r13), %r9' '4e 8b 0c 2c'
test_encoding 'movq (%rsp, %r15), %r9' '4e 8b 0c 3c'

# movq (%rbp, %r64), %r9
test_encoding 'movq (%rbp, %rax), %r9' '4c 8b 4c 05 00'
test_encoding 'movq (%rbp, %rbp), %r9' '4c 8b 4c 2d 00'
test_encoding 'movq (%rbp, %rdi), %r9' '4c 8b 4c 3d 00'
test_encoding 'movq (%rbp, %r8), %r9' '4e 8b 4c 05 00'
test_encoding 'movq (%rbp, %r12), %r9' '4e 8b 4c 25 00'
test_encoding 'movq (%rbp, %r13), %r9' '4e 8b 4c 2d 00'
test_encoding 'movq (%rbp, %r15), %r9' '4e 8b 4c 3d 00'

# movq (%r12, %r64), %r9
test_encoding 'movq (%r12, %rax), %r9' '4d 8b 0c 04'
test_encoding 'movq (%r12, %rbp), %r9' '4d 8b 0c 2c'
test_encoding 'movq (%r12, %rdi), %r9' '4d 8b 0c 3c'
test_encoding 'movq (%r12, %r8), %r9' '4f 8b 0c 04'
test_encoding 'movq (%r12, %r12), %r9' '4f 8b 0c 24'
test_encoding 'movq (%r12, %r13), %r9' '4f 8b 0c 2c'
test_encoding 'movq (%r12, %r15), %r9' '4f 8b 0c 3c'

# movq (%r13, %r64), %r9
test_encoding 'movq (%r13, %rax), %r9' '4d 8b 4c 05 00'
test_encoding 'movq (%r13, %rbp), %r9' '4d 8b 4c 2d 00'
test_encoding 'movq (%r13, %rdi), %r9' '4d 8b 4c 3d 00'
test_encoding 'movq (%r13, %r8), %r9' '4f 8b 4c 05 00'
test_encoding 'movq (%r13, %r12), %r9' '4f 8b 4c 25 00'
test_encoding 'movq (%r13, %r13), %r9' '4f 8b 4c 2d 00'
test_encoding 'movq (%r13, %r15), %r9' '4f 8b 4c 3d 00'

echo "[OK]"
exit 0

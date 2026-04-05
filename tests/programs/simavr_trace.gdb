target remote | simavr -g -m atmega328p alu.elf
set pagination off
set logging file /tmp/simavr_trace.csv
set logging enabled on

printf "step,pc,cycles"
set $i = 0
while $i < 32
  printf ",r%d", $i
  set $i = $i + 1
end
printf ",sreg,sp\n"

define dump_state
  printf "%d,", $step
  printf "%d,", $pc
  printf "%d", $cycles
  set $i = 0
  while $i < 32
    printf ",%d", $r$i
    set $i = $i + 1
  end
  printf ",%d", $sreg
  printf ",%d\n", $sp
end

set $step = 0
set $pc = $pc
set $cycles = 0
set $sp = $sp
set $sreg = $sreg
dump_state

set $step = $step + 1
stepi
info registers
set $pc = $pc
set $sp = $sp
set $sreg = $sreg
dump_state

set $step = $step + 1
stepi
info registers
set $pc = $pc
set $sp = $sp
set $sreg = $sreg
dump_state

set $step = $step + 1
stepi
info registers
set $pc = $pc
set $sp = $sp
set $sreg = $sreg
dump_state

set $step = $step + 1
stepi
info registers
set $pc = $pc
set $sp = $sp
set $sreg = $sreg
dump_state

quit

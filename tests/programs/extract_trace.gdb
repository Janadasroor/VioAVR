# GDB script to extract SimAVR trace
# Usage: avr-gdb -batch -x extract_trace.gdb

set pagination off
set confirm off
set logging file /tmp/simavr_trace.csv
set logging enabled on

# Print header
printf "step,pc,sreg,sp"
set $i = 0
while $i < 32
  printf ",r%d", $i
  set $i = $i + 1
end
printf "\n"

# Dump initial state
set $step = 0
printf "%d,0x%x,0x%x,0x%x", $step, $pc, $sreg, $sp
set $i = 0
while $i < 32
  printf ",0x%x", $r0 + $i
  set $i = $i + 1
end
printf "\n"

# Step and dump 50 times
set $step = 1
while $step <= 50
  stepi
  printf "%d,0x%x,0x%x,0x%x", $step, $pc, $sreg, $sp
  set $i = 0
  while $i < 32
    # Can't dynamically access r$i, need explicit
    printf ",0x%x", $r0
    set $i = $i + 1
  end
  printf "\n"
  set $step = $step + 1
end

quit

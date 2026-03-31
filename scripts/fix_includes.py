import os
import glob

files = glob.glob('/home/jnd/cpp_projects/VioAVR/tests/core/*.cpp')

def add_include(filepath, include_line, trigger_word):
    with open(filepath, 'r') as f:
        lines = f.readlines()
    
    if trigger_word not in "".join(lines):
        return
    
    if include_line in "".join(lines):
        return
        
    # Find last vioavr/core include
    last_idx = -1
    for i, line in enumerate(lines):
        if '#include "vioavr/core/' in line:
            last_idx = i
            
    if last_idx != -1:
        lines.insert(last_idx + 1, include_line + '\n')
    else:
        # If no vioavr/core includes, insert after doctest.h or at top
        found_doctest = -1
        for i, line in enumerate(lines):
            if 'doctest.h' in line:
                found_doctest = i
                break
        if found_doctest != -1:
            lines.insert(found_doctest + 1, include_line + '\n')
        else:
            lines.insert(0, include_line + '\n')
            
    with open(filepath, 'w') as f:
        f.writelines(lines)

for f in files:
    add_include(f, '#include "vioavr/core/devices/atmega328.hpp"', 'atmega328')
    add_include(f, '#include "vioavr/core/devices/atmega8.hpp"', 'atmega8')

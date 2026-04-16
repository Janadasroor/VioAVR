import os
import subprocess

meta_dir = 'scripts/metadata'
out_dir = 'include/vioavr/core/devices'

if not os.path.exists(out_dir):
    os.makedirs(out_dir)

files = [f for f in os.listdir(meta_dir) if f.endswith('_metadata.json') and '_debug' not in f]

for f in files:
    mcu = f.replace('_metadata.json', '')
    dest = os.path.join(out_dir, mcu.lower() + '.hpp')
    src = os.path.join(meta_dir, f)
    print(f"Generating {dest}...")
    subprocess.run(['python3', 'scripts/gen_device.py', src, dest])

print("Done.")

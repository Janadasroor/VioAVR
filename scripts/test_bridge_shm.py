import mmap
import struct
import time
import ctypes
import os

class VioBridgeClient:
    def __init__(self, instance_name="mega4809"):
        self.shm_path = f"/dev/shm/vioavr_shm_{instance_name}"
        
        # Open SHM file
        self.f = open(self.shm_path, "r+b")
        self.map = mmap.mmap(self.f.fileno(), 0)
        
        # We need a way to call sem_post/sem_wait on the memory addresses
        self.libc = ctypes.CDLL("libpthread.so.0")
        
        # Offsets
        self.OFF_SEM_REQ = 8
        self.OFF_SEM_ACK = 40 # Assuming 32-byte sem_t
        self.OFF_SYNC = 72
        self.OFF_STATUS = 80
        self.OFF_CMD = 84
        self.OFF_REQ_CYCLES = 88
        self.OFF_DIGITAL_INS = 352
        self.OFF_DIGITAL_OUTS = 480
        
    def set_digital_input(self, pin, high):
        self.map[self.OFF_DIGITAL_INS + pin] = 1 if high else 0
        
    def get_digital_output(self, pin):
        return self.map[self.OFF_DIGITAL_OUTS + pin] == 1
        
    def step(self, cycles=1000):
        # Set cycles
        struct.pack_into("<Q", self.map, self.OFF_REQ_CYCLES, cycles)
        # Signal Req
        req_ptr = ctypes.c_void_p(ctypes.addressof(ctypes.c_char.from_buffer(self.map, self.OFF_SEM_REQ)))
        self.libc.sem_post(req_ptr)
        
        # Wait Ack
        ack_ptr = ctypes.c_void_p(ctypes.addressof(ctypes.c_char.from_buffer(self.map, self.OFF_SEM_ACK)))
        self.libc.sem_wait(ack_ptr)
        
    def get_sync_counter(self):
        return struct.unpack_from("<Q", self.map, self.OFF_SYNC)[0]

    def close(self):
        self.map.close()
        self.f.close()

if __name__ == "__main__":
    print("Connecting to VioAVR Bridge...")
    try:
        client = VioBridgeClient()
        print(f"Connected. Sync Counter: {client.get_sync_counter()}")
        
        print("Stepping 10,000 cycles...")
        start = time.time()
        for _ in range(10):
            client.step(1000)
        end = time.time()
        
        print(f"Finished. New Sync Counter: {client.get_sync_counter()}")
        print(f"Time: {end-start:.4f}s")
        
        client.close()
    except Exception as e:
        print(f"Error: {e}")

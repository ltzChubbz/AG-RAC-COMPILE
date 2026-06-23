import struct
import sys

def probe_vertices(file_path, start_offset, end_offset):
    with open(file_path, 'rb') as f:
        f.seek(start_offset)
        data = f.read(end_offset - start_offset)
    
    print(f"Probing Vertices in {file_path} from 0x{start_offset:X}...")
    
    # Try S16 (16-bit signed integer) usually used for fixed-point
    # PS2 geometry often has X, Y, Z, W (4 x 2 bytes = 8 bytes per vertex)
    for i in range(0, len(data) - 8, 8):
        v = struct.unpack('<hhhh', data[i:i+8])
        # If it looks like normalized geometry (small values but not zero)
        if all(abs(x) < 30000 for x in v) and any(abs(x) > 10 for x in v):
            print(f"Possible S16 verts at 0x{start_offset+i:X}: {v}")
            if i > 64: break # Only show first few

    # Try F32 (32-bit float)
    for i in range(0, len(data) - 16, 16):
        try:
            v = struct.unpack('<ffff', data[i:i+16])
            if all(-1000.0 < x < 1000.0 for x in v) and any(abs(x) > 0.001 for x in v):
                print(f"Possible F32 verts at 0x{start_offset+i:X}: {v}")
                if i > 64: break
        except: pass

if __name__ == "__main__":
    probe_vertices('assets/wads/wad_006.bin', 0x22F00, 0x23F00)

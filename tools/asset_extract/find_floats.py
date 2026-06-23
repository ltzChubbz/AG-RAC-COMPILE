import struct

def find_floats(file_path):
    with open(file_path, 'rb') as f:
        data = f.read()
    
    print(f"Scanning {file_path} for F32 geometry patterns...")
    count = 0
    # Search every 4 bytes (aligned)
    for i in range(0, len(data)-16, 4):
        try:
            v = struct.unpack('<ffff', data[i:i+16])
            # Geometry usually isn't massive or tiny
            if all(-1000.0 < x < 1000.0 for x in v) and any(abs(x) > 0.01 for x in v):
                # Check for "sequence" - multiple F32 vectors in a row
                seq_count = 0
                for j in range(1, 4):
                    v2 = struct.unpack('<ffff', data[i+j*16: i+j*16+16])
                    if all(-1000.0 < x < 1000.0 for x2 in v2) and any(abs(x2) > 0.01 for x2 in v2):
                        seq_count += 1
                
                if seq_count >= 2:
                    print(f"Candidate F32 block at 0x{i:X}: {v}")
                    count += 1
                    if count > 10: break
        except: pass

if __name__ == "__main__":
    find_floats('assets/wads/wad_006.bin')

import struct

def analyze_flags(file_path, offset):
    with open(file_path, 'rb') as f:
        f.seek(offset)
        raw = f.read(1024)
    
    print(f"Analyzing vertices at 0x{offset:X}...")
    for i in range(0, 512, 8):
        v = struct.unpack('<hhhh', raw[i:i+8])
        # Look for bits in the 4th component
        x, y, z, flag = v
        binary_flag = format(flag & 0xFFFF, '016b')
        print(f"V[{i//8:03d}]: ({x:6d}, {y:6d}, {z:6d}) | Flag: 0x{flag & 0xFFFF:04X} ({binary_flag})")

if __name__ == "__main__":
    analyze_flags('assets/wads/wad_006.bin', 0x22F00)

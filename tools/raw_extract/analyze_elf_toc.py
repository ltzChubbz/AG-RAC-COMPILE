import struct
import sys

def main():
    elf_path = sys.argv[1]
    with open(elf_path, 'rb') as f:
        f.seek(0x942C0)
        data = f.read(141 * 16)
        
    print("Dumping actual Volume Table at 0x942C0 (141 entries):")
    print("Idx |   Word0   |   Word1   |   Word2   |   Word3")
    print("-------------------------------------------------")
    for i in range(15):
        w0, w1, w2, w3 = struct.unpack_from('<IIII', data, i * 16)
        print(f"{i:3d} | {w0:9d} | {w1:9d} | {w2:9d} | {w3:9d}")

if __name__ == "__main__":
    if len(sys.argv) > 1:
        main()

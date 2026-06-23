import sys

def search_elf_refs(elf_path, target_addr):
    with open(elf_path, 'rb') as f:
        data = f.read()
    
    # Target value as 4-byte little-endian
    target_bytes = target_addr.to_bytes(4, 'little')
    
    count = 0
    pos = 0
    while True:
        pos = data.find(target_bytes, pos)
        if pos == -1:
            break
        print(f"Found reference to 0x{target_addr:08X} at offset 0x{pos:X}")
        pos += 4
        count += 1
    
    if count == 0:
        print(f"No references to 0x{target_addr:08X} found.")

if __name__ == "__main__":
    search_elf_refs('tools/iso_extract/dist/extracted/SCUS_971.99', 0x942C0)

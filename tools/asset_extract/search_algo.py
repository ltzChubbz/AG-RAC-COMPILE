import os

def search_strings(file_path):
    with open(file_path, 'rb') as f:
        data = f.read()
    
    # Common compression/geometry strings
    targets = [b'lzss', b'inflate', b'RLE', b'decompress', b'MESH', b'VIF', b'GIF', b'KICK']
    
    for t in targets:
        pos = data.find(t)
        if pos != -1:
            # Get some context around the string
            start = max(0, pos-32)
            end = min(len(data), pos+32)
            context = data[start:end]
            print(f"Found '{t.decode()}' at 0x{pos:X}. Context: {context}")

if __name__ == "__main__":
    search_strings('tools/iso_extract/dist/extracted/SCUS_971.99')

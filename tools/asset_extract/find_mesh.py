import glob
import os

def find_mesh_signatures():
    wad_files = sorted(glob.glob('assets/wads/*.bin'))
    print(f"Scanning {len(wad_files)} WAD files for 'MESH' signature...")
    
    for f in wad_files:
        try:
            with open(f, 'rb') as fd:
                data = fd.read()
                pos = data.find(b'MESH')
                if pos != -1:
                    print(f"FOUND MESH in {os.path.basename(f)} at 0x{pos:X} (Total size: {len(data)})")
                
                # Also check for 'VIF' or 'GIF' tags which are common in Insomniac engines
                pos_vif = data.find(b'VIF')
                if pos_vif != -1:
                     print(f"FOUND VIF in {os.path.basename(f)} at 0x{pos_vif:X}")
        except Exception as e:
            print(f"Error reading {f}: {e}")

if __name__ == "__main__":
    find_mesh_signatures()

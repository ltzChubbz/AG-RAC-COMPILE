import urllib.request
import tarfile
import os

url = "https://github.com/ps2dev/ps2dev/releases/download/v1.3.0/ps2dev-windows-latest.tar.gz"
tar_path = "ps2dev.tar.gz"
out_dir = "tools"

print(f"Downloading {url}...")
urllib.request.urlretrieve(url, tar_path)

print("Extracting...")
with tarfile.open(tar_path, "r:gz") as tar:
    tar.extractall(path=out_dir)

# Clean up
os.remove(tar_path)

# Rename if necessary
src = os.path.join(out_dir, "ps2dev")
if os.path.exists(os.path.join(out_dir, "usr")):
    # The tarball might extract directly to tools/usr or tools/ps2dev
    print("Done extracting!")

import tkinter as tk
from tkinter import filedialog, messagebox, ttk
import threading
import sys
import os
from pathlib import Path
import io

# Add current dir to path to import iso_extract if running as script
current_dir = os.path.dirname(os.path.abspath(__file__))
if current_dir not in sys.path:
    sys.path.append(current_dir)

try:
    import iso_extract
except ImportError as e:
    messagebox.showerror("Error", f"Could not import iso_extract: {e}")
    sys.exit(1)

class PrintRedirector(io.StringIO):
    def __init__(self, log_func):
        super().__init__()
        self.log_func = log_func

    def write(self, string):
        if string.strip('\n') and string.strip('\r'):
            self.log_func(string.strip('\n'))
        
    def flush(self):
        pass

class IsoExtractApp:
    def __init__(self, root):
        self.root = root
        self.root.title("AG-RAC ISO Extractor")
        self.root.geometry("680x480")
        
        self.iso_path = None
        # Default to a generic extracted folder next to the executable if frozen
        if getattr(sys, 'frozen', False):
            base_dir = Path(sys.executable).parent
        else:
            base_dir = Path(__file__).parent.parent.parent

        self.out_path = (base_dir / "extracted").resolve()
        self.game_id = None
        self.elf_name = None
        
        self.setup_ui()
        
    def setup_ui(self):
        frame = ttk.Frame(self.root, padding=10)
        frame.pack(fill=tk.BOTH, expand=True)
        
        # ISO Select
        ttk.Label(frame, text="PS2 ISO Image:").grid(row=0, column=0, sticky=tk.W, pady=5)
        self.iso_var = tk.StringVar()
        self.iso_entry = ttk.Entry(frame, textvariable=self.iso_var, state='readonly', width=60)
        self.iso_entry.grid(row=0, column=1, padx=5, pady=5, sticky=tk.EW)
        ttk.Button(frame, text="Browse...", command=self.browse_iso).grid(row=0, column=2, pady=5)
        
        # Output Select
        ttk.Label(frame, text="Output Directory:").grid(row=1, column=0, sticky=tk.W, pady=5)
        self.out_var = tk.StringVar(value=str(self.out_path))
        self.out_entry = ttk.Entry(frame, textvariable=self.out_var, state='readonly', width=60)
        self.out_entry.grid(row=1, column=1, padx=5, pady=5, sticky=tk.EW)
        ttk.Button(frame, text="Browse...", command=self.browse_out).grid(row=1, column=2, pady=5)
        
        # Info
        self.info_var = tk.StringVar(value="Select an ISO to check version.")
        ttk.Label(frame, textvariable=self.info_var, font=('Helvetica', 10, 'bold')).grid(row=2, column=0, columnspan=3, pady=10)
        
        # Extract Button
        self.extract_btn = ttk.Button(frame, text="Extract ISO", command=self.start_extract, state=tk.DISABLED)
        self.extract_btn.grid(row=3, column=0, columnspan=3, pady=10)
        
        # Log
        self.log_text = tk.Text(frame, height=15, width=70, state=tk.DISABLED, bg="#1e1e1e", fg="#00ff00", font=("Consolas", 9))
        self.log_text.grid(row=4, column=0, columnspan=3, pady=5, sticky=tk.NSEW)
        frame.rowconfigure(4, weight=1)
        frame.columnconfigure(1, weight=1)
        
    def log(self, text):
        self.log_text.config(state=tk.NORMAL)
        self.log_text.insert(tk.END, text + "\n")
        self.log_text.see(tk.END)
        self.log_text.config(state=tk.DISABLED)

    def log_thread_safe(self, text):
        self.root.after(0, self.log, text)
        
    def browse_iso(self):
        path = filedialog.askopenfilename(filetypes=[("PS2 ISO/BIN", "*.iso *.bin"), ("All files", "*.*")])
        if path:
            self.iso_path = Path(path)
            self.iso_var.set(str(self.iso_path))
            self.check_version()
            
    def browse_out(self):
        path = filedialog.askdirectory(initialdir=self.out_var.get())
        if path:
            self.out_path = Path(path)
            self.out_var.set(str(self.out_path))
            
    def check_version(self):
        self.log(f"Checking ISO: {self.iso_path.name}...")
        try:
            iso = iso_extract.open_iso(self.iso_path)
            entries = iso_extract.list_iso_contents(iso)
            self.game_id, self.elf_name = iso_extract.detect_game(entries)
            iso.close()
            
            if self.game_id == "unknown":
                self.info_var.set("Warning: Could not auto-detect game. Unknown version.")
                self.game_id = "rac1"  # Default fallback
                self.extract_btn.config(state=tk.NORMAL)
                self.log("Game auto-detection failed. Unknown ELF - falling back to rac1 parsing.")
            else:
                self.info_var.set(f"Detected Game: {self.game_id.upper()} (ELF: {self.elf_name})")
                self.extract_btn.config(state=tk.NORMAL)
                self.log(f"Detected game: {self.game_id.upper()} (ELF: {self.elf_name})")
        except Exception as e:
            self.info_var.set(f"Error reading ISO: {e}")
            self.extract_btn.config(state=tk.DISABLED)
            self.log(f"Error checking ISO: {e}")

    def start_extract(self):
        self.extract_btn.config(state=tk.DISABLED)
        self.log(f"Starting extraction for {self.game_id}...")
        threading.Thread(target=self.run_extraction, daemon=True).start()
        
    def run_extraction(self):
        old_stdout = sys.stdout
        sys.stdout = PrintRedirector(self.log_thread_safe)
        
        try:
            self.out_path.mkdir(parents=True, exist_ok=True)
            iso = iso_extract.open_iso(self.iso_path)
            entries = iso_extract.list_iso_contents(iso)
            extracted, skipped = iso_extract.extract_all(iso, entries, self.out_path, self.game_id)
            iso.close()
            
            elf_path = iso_extract.verify_elf(self.out_path, self.game_id)
            iso_extract.write_extraction_manifest(self.out_path, self.game_id, self.iso_path, extracted)
            
            self.log_thread_safe("\nExtraction complete!")
            if elf_path:
                self.log_thread_safe(f"ELF verified: {elf_path.name}")
            else:
                self.log_thread_safe(f"WARNING: ELF not found or invalid.")
                
            self.root.after(0, messagebox.showinfo, "Success", "Extraction complete!")
        except Exception as e:
            self.log_thread_safe(f"Error: {e}")
            self.root.after(0, messagebox.showerror, "Error", f"Extraction failed: {e}")
        finally:
            sys.stdout = old_stdout
            self.root.after(0, lambda: self.extract_btn.config(state=tk.NORMAL))

if __name__ == "__main__":
    root = tk.Tk()
    # Apply a modern styling
    style = ttk.Style(root)
    if "clam" in style.theme_names():
        style.theme_use("clam")
    app = IsoExtractApp(root)
    root.mainloop()

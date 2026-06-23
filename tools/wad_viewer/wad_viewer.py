import sys
import struct
import tkinter as tk
from tkinter import ttk, messagebox

class WadViewer:
    def __init__(self, root, file_path):
        self.root = root
        self.root.title(f"WAD Viewer - {file_path}")
        self.root.geometry("1200x850")
        
        try:
            with open(file_path, 'rb') as f:
                self.data = f.read()
        except Exception as e:
            messagebox.showerror("Error", f"Failed to load file: {e}")
            sys.exit(1)
            
        self.file_size = len(self.data)
        
        self.view_modes = ["Raw Hex", "32-bit Floats (f32)", "32-bit Integers (s32)", "16-bit Integers (s16)", "VIF Unpack Decoder"]
        self.current_mode = tk.StringVar(value=self.view_modes[0])
        
        self.setup_ui()
        self.scan_regions()
        
    def setup_ui(self):
        # PanedWindow for Sidebar and Main Content
        self.paned = ttk.PanedWindow(self.root, orient=tk.HORIZONTAL)
        self.paned.pack(fill=tk.BOTH, expand=True)
        
        # Sidebar for Regions
        self.sidebar = ttk.Frame(self.paned, width=300)
        self.paned.add(self.sidebar, weight=1)
        
        ttk.Label(self.sidebar, text="Detected Regions:").pack(anchor=tk.W, padx=5, pady=5)
        
        self.region_listbox = tk.Listbox(self.sidebar)
        self.region_listbox.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        self.region_listbox.bind('<<ListboxSelect>>', self.on_region_select)
        
        # Main Content: Right Paned Window (Top=Text, Bottom=Canvas)
        self.right_paned = ttk.PanedWindow(self.paned, orient=tk.VERTICAL)
        self.paned.add(self.right_paned, weight=4)
        
        # Top Frame for Controls and Text
        self.top_frame = ttk.Frame(self.right_paned)
        self.right_paned.add(self.top_frame, weight=1)
        
        # Toolbar
        toolbar = ttk.Frame(self.top_frame)
        toolbar.pack(fill=tk.X, padx=5, pady=5)
        
        ttk.Label(toolbar, text="Offset (Hex):").pack(side=tk.LEFT)
        self.offset_var = tk.StringVar(value="0")
        self.offset_entry = ttk.Entry(toolbar, textvariable=self.offset_var, width=12)
        self.offset_entry.pack(side=tk.LEFT, padx=5)
        self.offset_entry.bind('<Return>', self.on_offset_entered)
        
        ttk.Button(toolbar, text="Go", command=self.on_offset_entered).pack(side=tk.LEFT)
        ttk.Button(toolbar, text="Page Up", command=self.page_up).pack(side=tk.LEFT, padx=5)
        ttk.Button(toolbar, text="Page Down", command=self.page_down).pack(side=tk.LEFT)
        
        ttk.Label(toolbar, text="  View Mode:").pack(side=tk.LEFT)
        mode_cb = ttk.Combobox(toolbar, textvariable=self.current_mode, values=self.view_modes, state="readonly", width=25)
        mode_cb.pack(side=tk.LEFT, padx=5)
        mode_cb.bind('<<ComboboxSelected>>', lambda e: self.display_hex(self.current_offset))
        
        # Text area for Hex
        self.hex_text = tk.Text(self.top_frame, font=("Consolas", 11), wrap=tk.NONE)
        self.hex_text.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        self.hex_text.bind("<MouseWheel>", self.on_mousewheel)
        
        # Bottom Frame for Visualizer Canvas
        self.bottom_frame = ttk.Frame(self.right_paned)
        self.right_paned.add(self.bottom_frame, weight=2)
        
        self.canvas = tk.Canvas(self.bottom_frame, bg="#1E1E1E")
        self.canvas.pack(fill=tk.BOTH, expand=True)
        # Bind resize to redraw
        self.canvas.bind("<Configure>", lambda e: self.redraw_visualizer())
        
        self.current_offset = 0
        self.lines_to_show = 40
        self.visualizer_points = []
        self.display_hex(0)
        
    def scan_regions(self):
        self.regions = []
        self.regions.append(("0x00000000", "MIPS Executable Segment"))
        self.regions.append(("0x0042C400", "Veldin Palette"))
        self.regions.append(("0x0042CC00", "Veldin Texture Pixels"))
        
        # Enhanced VIF Scanner
        print("Scanning for VIF UNPACK packets...")
        vif_count = 0
        for i in range(0, min(self.file_size - 4, 0x1000000), 4): # Scan first 16MB
            cmd = self.data[i+3]
            if (cmd & 0x60) == 0x60:
                v_type = (cmd >> 2) & 0x03
                d_type = cmd & 0x03
                num = self.data[i+2]
                
                # Check for likely R&C geometry unpacks
                if ((v_type == 3 and d_type == 1) or (v_type == 1 and d_type == 1)) and 0 < num < 255:
                    vif_count += 1
                    if vif_count <= 200:
                        vname = f"V{v_type+1}-{(32 if d_type==0 else 16 if d_type==1 else 8 if d_type==2 else 5)}"
                        self.regions.append((hex(i), f"VIF UNPACK {vname} (x{num})"))
                        
        print(f"Total VIF packets found: {vif_count}")
        
        # Populate UI
        for r in self.regions:
            self.region_listbox.insert(tk.END, f"{r[0]:>10}: {r[1]}")
            
    def on_region_select(self, event):
        selection = self.region_listbox.curselection()
        if selection:
            idx = selection[0]
            offset_str = self.regions[idx][0].strip()
            self.offset_var.set(offset_str)
            self.display_hex(int(offset_str, 16))
            
    def on_offset_entered(self, event=None):
        try:
            offset = int(self.offset_var.get(), 16)
            self.display_hex(offset)
        except ValueError:
            messagebox.showerror("Invalid Offset", "Please enter a valid hex offset.")
            
    def page_up(self):
        self.display_hex(max(0, self.current_offset - (self.lines_to_show * 16)))
        
    def page_down(self):
        self.display_hex(min(self.file_size, self.current_offset + (self.lines_to_show * 16)))
        
    def on_mousewheel(self, event):
        step = 16
        mode = self.current_mode.get()
        if mode == "VIF Unpack Decoder": step = 4
        
        if event.delta > 0:
            self.display_hex(max(0, self.current_offset - step))
        else:
            self.display_hex(min(self.file_size, self.current_offset + step))
        return "break"
            
    def display_hex(self, start_offset):
        self.current_offset = start_offset
        self.offset_var.set(f"0x{start_offset:08X}")
        self.hex_text.delete(1.0, tk.END)
        self.visualizer_points = []
        self.redraw_visualizer()
        
        if start_offset < 0: start_offset = 0
        if start_offset >= self.file_size: return
        
        mode = self.current_mode.get()
        
        if mode == "VIF Unpack Decoder":
            self.display_vif(start_offset)
            return
            
        end_offset = min(start_offset + (self.lines_to_show * 16), self.file_size)
        output = []
        
        for i in range(start_offset, end_offset, 16):
            chunk = self.data[i:i+16]
            line_str = f"{i:08X} | "
            
            if mode == "Raw Hex":
                hex_parts = []
                for j in range(0, 16, 4):
                    if j < len(chunk):
                        sub = chunk[j:j+4]
                        hex_parts.append(" ".join(f"{b:02X}" for b in sub))
                    else:
                        hex_parts.append("           ")
                hex_str = "  ".join(hex_parts).ljust(50)
                ascii_str = "".join((chr(b) if 32 <= b <= 126 else '.') for b in chunk)
                line_str += f"{hex_str} | {ascii_str}"
                
            elif mode == "32-bit Floats (f32)":
                parts = []
                for j in range(0, 16, 4):
                    if j + 4 <= len(chunk):
                        val = struct.unpack('<f', chunk[j:j+4])[0]
                        parts.append(f"{val:12.4f}")
                line_str += " ".join(parts)
                
            elif mode == "32-bit Integers (s32)":
                parts = []
                for j in range(0, 16, 4):
                    if j + 4 <= len(chunk):
                        val = struct.unpack('<i', chunk[j:j+4])[0]
                        parts.append(f"{val:12d}")
                line_str += " ".join(parts)
                
            elif mode == "16-bit Integers (s16)":
                parts = []
                for j in range(0, 16, 2):
                    if j + 2 <= len(chunk):
                        val = struct.unpack('<h', chunk[j:j+2])[0]
                        parts.append(f"{val:6d}")
                line_str += " ".join(parts)
                
            output.append(line_str)
            
        self.hex_text.insert(tk.END, "\n".join(output))

    def display_vif(self, offset):
        if offset + 4 > self.file_size:
            self.hex_text.insert(tk.END, "End of file.")
            return
            
        cmd = self.data[offset+3]
        if (cmd & 0x60) != 0x60:
            self.hex_text.insert(tk.END, f"Offset {offset:08X} is not a VIF UNPACK command. (Cmd: {cmd:02X})\nPlease select a valid VIF packet from the regions list or change View Mode.")
            return
            
        v_type = (cmd >> 2) & 0x03
        d_type = cmd & 0x03
        num = self.data[offset+2]
        
        vname = f"V{v_type+1}-{(32 if d_type==0 else 16 if d_type==1 else 8 if d_type==2 else 5)}"
        
        output = [f"=== VIF UNPACK DECODER ==="]
        output.append(f"Offset : 0x{offset:08X}")
        output.append(f"Command: 0x{cmd:02X}")
        output.append(f"Format : {vname}")
        output.append(f"Count  : {num} elements")
        output.append("-" * 40)
        
        ptr = offset + 4
        
        if v_type == 3 and d_type == 1: # V4-16 (X, Y, Z, Flag)
            scale = 1.0 / 1024.0
            for i in range(num):
                if ptr + 8 > self.file_size: break
                x, y, z, flag = struct.unpack('<hhhh', self.data[ptr:ptr+8])
                output.append(f"[{i:03d}] X: {x*scale:8.3f}  Y: {y*scale:8.3f}  Z: {z*scale:8.3f}  Flag: 0x{flag:04X}")
                self.visualizer_points.append((x*scale, y*scale, z*scale, flag))
                ptr += 8
                
        elif v_type == 1 and d_type == 1: # V2-16 (U, V)
            scale = 1.0 / 4096.0
            for i in range(num):
                if ptr + 4 > self.file_size: break
                u, v = struct.unpack('<hh', self.data[ptr:ptr+4])
                output.append(f"[{i:03d}] U: {u*scale:8.4f}  V: {v*scale:8.4f}")
                self.visualizer_points.append((u*scale, v*scale))
                ptr += 4
        else:
            output.append(f"Decoder for {vname} is not yet implemented.")
            
        self.hex_text.insert(tk.END, "\n".join(output))
        self.redraw_visualizer()

    def redraw_visualizer(self):
        self.canvas.delete("all")
        if not self.visualizer_points:
            self.canvas.create_text(20, 20, anchor=tk.NW, text="Visualizer (Waiting for VIF Packet)", fill="#666666", font=("Arial", 12))
            return
            
        width = self.canvas.winfo_width()
        height = self.canvas.winfo_height()
        if width <= 1 or height <= 1:
            width, height = 800, 400
            
        margin = 30
        
        xs = [p[0] for p in self.visualizer_points]
        ys = [p[1] for p in self.visualizer_points]
        
        min_x, max_x = min(xs), max(xs)
        min_y, max_y = min(ys), max(ys)
        
        dx = max_x - min_x
        dy = max_y - min_y
        
        if dx == 0: dx = 0.001
        if dy == 0: dy = 0.001
        
        scale_x = (width - margin * 2) / dx
        scale_y = (height - margin * 2) / dy
        scale = min(scale_x, scale_y)
        
        cx = (width - dx * scale) / 2
        cy = (height - dy * scale) / 2
        
        prev_x, prev_y = None, None
        
        # Determine if points are V4 (X,Y,Z,Flag) or V2 (U,V)
        is_v4 = len(self.visualizer_points[0]) == 4
        
        if is_v4:
            self.canvas.create_text(20, 20, anchor=tk.NW, text=f"3D Geometry Wireframe ({len(self.visualizer_points)} Vertices) - X/Y Orthographic", fill="cyan", font=("Arial", 12, "bold"))
        else:
            self.canvas.create_text(20, 20, anchor=tk.NW, text=f"2D Texture UVs ({len(self.visualizer_points)} Coords)", fill="yellow", font=("Arial", 12, "bold"))
            
        for p in self.visualizer_points:
            # Orthogonal projection: map X and Y.
            # Y is flipped because canvas Y goes down.
            px = cx + (p[0] - min_x) * scale
            py = height - (cy + (p[1] - min_y) * scale)
            
            # Draw point
            r = 2
            color = "white" if is_v4 else "yellow"
            self.canvas.create_oval(px-r, py-r, px+r, py+r, fill=color, outline="")
            
            draw_line = True
            if is_v4:
                flag = p[3]
                # If ADC bit (bit 15) is 0, it starts a new triangle strip
                if (flag & 0x8000) == 0:
                    draw_line = False
                    
            if prev_x is not None and prev_y is not None and draw_line:
                self.canvas.create_line(prev_x, prev_y, px, py, fill="cyan" if is_v4 else "#666600", width=1.5)
                
            prev_x, prev_y = px, py

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python wad_viewer.py <path_to_wad>")
        sys.exit(1)
        
    path = sys.argv[1]
    root = tk.Tk()
    app = WadViewer(root, path)
    root.mainloop()

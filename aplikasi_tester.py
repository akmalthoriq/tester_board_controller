import tkinter as tk
import customtkinter as ctk
import serial
import serial.tools.list_ports
import threading
import re
import time
from matplotlib.figure import Figure
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg

# ========== KONFIGURASI SERIAL ==========
BAUDRATE = 115200
PULSE_PATTERN = r'Pulse X:(\-?\d+)\s*\|\s*Y:(\-?\d+)\s*\|\s*Z:(\-?\d+)\s*\|\s*A:(\-?\d+)\s*\|\s*B:(\-?\d+)\s*\|\s*C:(\-?\d+)'
# ========================================

class TesterGUI(ctk.CTk):
    def __init__(self):
        super().__init__()
        self.title("Tester Board Controller v1.5 - Pro Edition + MDI")
        self.geometry("1200x740")
        ctk.set_appearance_mode("dark")
        ctk.set_default_color_theme("blue")

        # ---- variabel ----
        self.serial_port = None
        self.running = False
        self.pulse_history = {"X": [], "Y": [], "Z": []}
        self.max_points = 100  # titik tampilan grafik

        # ---- layout utama ----
        self.columnconfigure(0, weight=1)
        self.rowconfigure(0, weight=1)
        frame = ctk.CTkFrame(self)
        frame.grid(row=0, column=0, sticky="nsew", padx=10, pady=10)

        # top bar
        top = ctk.CTkFrame(frame)
        top.pack(fill="x", pady=5)
        self.port_var = tk.StringVar(value="Pilih Port")
        self.port_menu = ctk.CTkOptionMenu(top, variable=self.port_var, values=self.get_ports())
        self.port_menu.pack(side="left", padx=10, pady=5)
        self.connect_btn = ctk.CTkButton(top, text="Connect", command=self.toggle_connection)
        self.connect_btn.pack(side="left", padx=5)
        self.status_lbl = ctk.CTkLabel(top, text="Status: Disconnected", text_color="red")
        self.status_lbl.pack(side="left", padx=10)
        self.refresh_btn = ctk.CTkButton(top, text="Refresh Port", command=self.refresh_ports, width=100)
        self.refresh_btn.pack(side="right", padx=10)

        # --- bagian utama terbagi dua: kiri kontrol, kanan grafik + indikator ---
        body = ctk.CTkFrame(frame)
        body.pack(fill="both", expand=True, padx=10, pady=10)
        body.columnconfigure(0, weight=1)
        body.columnconfigure(1, weight=2)
        body.rowconfigure(0, weight=1)

        # LEFT PANEL (Kontrol & Log)
        left_panel = ctk.CTkFrame(body)
        left_panel.grid(row=0, column=0, sticky="nswe", padx=10)

        # axis value
        self.axis_labels = {}
        axis_row = ctk.CTkFrame(left_panel)
        axis_row.pack(pady=5)
        for ax in ["X", "Y", "Z", "A", "B", "C"]:
            box = ctk.CTkFrame(axis_row, corner_radius=8)
            box.pack(side="left", padx=5, pady=10)
            ctk.CTkLabel(box, text=ax, font=("Consolas", 18, "bold")).pack()
            lbl = ctk.CTkLabel(box, text="0", font=("Consolas", 22, "bold"), text_color="#00FFAA")
            lbl.pack()
            self.axis_labels[ax] = lbl

        # buttons row 1
        row1 = ctk.CTkFrame(left_panel)
        row1.pack(pady=5)
        btns = [
            ("1️⃣ Tes Output", "1"),
            ("2️⃣ Cek Pulse", "2"),
            ("3️⃣ Stop Pulse", "3"),
            ("4️⃣ Reset", "4"),
        ]
        for txt, cmd in btns:
            ctk.CTkButton(row1, text=txt, width=120, command=lambda c=cmd: self.send_command(c)).pack(side="left", padx=4)

        # buttons row 2
        row2 = ctk.CTkFrame(left_panel)
        row2.pack(pady=5)
        for txt, cmd in [("5️⃣ Mulai Encoder", "5"), ("6️⃣ Stop Encoder", "6"), ("❓ Help", "?")]:
            ctk.CTkButton(row2, text=txt, width=160, command=lambda c=cmd: self.send_command(c)).pack(side="left", padx=4)

        # --- MDI (Manual Data Input) ---
        mdi_frame = ctk.CTkFrame(left_panel)
        mdi_frame.pack(fill="x", pady=10, padx=10)
        ctk.CTkLabel(mdi_frame, text="MDI (Manual Data Input):", font=("Consolas", 14, "bold")).pack(anchor="w", padx=5, pady=2)
        mdi_inner = ctk.CTkFrame(mdi_frame)
        mdi_inner.pack(fill="x", pady=5)
        self.mdi_entry = ctk.CTkEntry(mdi_inner, placeholder_text="Ketik perintah manual (contoh: OUT_CLAMP)", width=280)
        self.mdi_entry.pack(side="left", padx=5, pady=5, fill="x", expand=True)
        self.mdi_btn = ctk.CTkButton(mdi_inner, text="Kirim", width=80, command=self.send_mdi)
        self.mdi_btn.pack(side="left", padx=5)
        self.mdi_entry.bind("<Return>", lambda e: self.send_mdi())

        # log
        self.log_box = tk.Text(left_panel, height=10, bg="#111", fg="#0f0", insertbackground="white")
        self.log_box.pack(fill="x", padx=10, pady=10)

        # RIGHT PANEL (indikator dan grafik)
        right_panel = ctk.CTkFrame(body)
        right_panel.grid(row=0, column=1, sticky="nswe", padx=10)

        # --- indikator input ---
        ind_frame = ctk.CTkFrame(right_panel)
        ind_frame.pack(fill="x", pady=10)

        ctk.CTkLabel(ind_frame, text="Input Indicators", font=("Consolas", 16, "bold"), text_color="#00ffff").pack(anchor="w", padx=10)

        self.input_pins = [
            "DIR_X", "DIR_Y", "DIR_Z","DIR_A", "DIR_B", "DIR_C",
            "CLEAR_ALRM_DRVR", "SPINDLE_MODE", "OUT_ORIENT","OUT_UMB","OUT_CLAMP",
            "LED_MERAH", "LED_HIJAU", "FLOOD", "HYDRAULIC","OUT_ATC_CW","OUT_ATC_CCW"    
        ]
        self.indicators = {}
        grid = ctk.CTkFrame(ind_frame)
        grid.pack(padx=10, pady=10)
        for i, name in enumerate(self.input_pins):
            lbl = ctk.CTkLabel(grid, text=name, width=140, anchor="w")
            lbl.grid(row=i // 2, column=(i % 2) * 2, padx=5, pady=2)
            dot = ctk.CTkLabel(grid, text="●", text_color="gray", font=("Arial", 18, "bold"))
            dot.grid(row=i // 2, column=(i % 2) * 2 + 1, padx=5)
            self.indicators[name] = dot

        # --- grafik realtime pulse ---
        ctk.CTkLabel(right_panel, text="Realtime Pulse Graph", font=("Consolas", 16, "bold"), text_color="#00ffff").pack(anchor="w", padx=10, pady=(10, 0))
        self.fig = Figure(figsize=(6, 3), dpi=100)
        self.ax = self.fig.add_subplot(111)
        self.ax.set_facecolor("#111")
        self.ax.grid(True, color="#333")
        self.ax.set_title("X-Y-Z Axis Pulse Monitor")
        self.ax.set_xlabel("Waktu")
        self.ax.set_ylabel("Pulse Count")
        self.lines = {
            "X": self.ax.plot([], [], color="cyan", label="X")[0],
            "Y": self.ax.plot([], [], color="magenta", label="Y")[0],
            "Z": self.ax.plot([], [], color="yellow", label="Z")[0],
        }
        self.ax.legend()
        self.canvas = FigureCanvasTkAgg(self.fig, master=right_panel)
        self.canvas.get_tk_widget().pack(fill="both", expand=True, padx=10, pady=10)

        self.update_graph()
        self.after(3000, self.refresh_ports)

    # =====================================================
    # SERIAL & THREAD
    # =====================================================
    def get_ports(self):
        return [p.device for p in serial.tools.list_ports.comports()]

    def refresh_ports(self):
        ports = self.get_ports()
        self.port_menu.configure(values=ports)
        if ports and self.port_var.get() not in ports:
            self.port_var.set(ports[0])
        self.after(3000, self.refresh_ports)

    def toggle_connection(self):
        if self.serial_port and self.serial_port.is_open:
            self.disconnect_serial()
        else:
            try:
                port = self.port_var.get()
                self.serial_port = serial.Serial(port, BAUDRATE, timeout=1)
                self.status_lbl.configure(text="Status: Connected", text_color="green")
                self.connect_btn.configure(text="Disconnect")
                threading.Thread(target=self.read_serial, daemon=True).start()
            except Exception as e:
                self.status_lbl.configure(text=f"Gagal konek: {e}", text_color="red")

    def disconnect_serial(self):
        self.running = False
        if self.serial_port and self.serial_port.is_open:
            self.serial_port.close()
        self.status_lbl.configure(text="Status: Disconnected", text_color="red")
        self.connect_btn.configure(text="Connect")

    def send_command(self, cmd):
        if not self.serial_port or not self.serial_port.is_open:
            self.log_box.insert("end", "⚠️ Belum terhubung ke port.\n")
            self.log_box.see("end")
            return
        self.serial_port.write(f"{cmd}\n".encode())
        self.log_box.insert("end", f">>> {cmd}\n")
        self.log_box.see("end")

    def send_mdi(self):
        """Kirim perintah manual dari kolom MDI"""
        cmd = self.mdi_entry.get().strip()
        if not cmd:
            return
        if not self.serial_port or not self.serial_port.is_open:
            self.log_box.insert("end", "⚠️ Belum terhubung ke port.\n")
            self.log_box.see("end")
            return
        try:
            self.serial_port.write(f"{cmd}\n".encode())
            self.log_box.insert("end", f">>> MDI: {cmd}\n")
            self.log_box.see("end")
            self.mdi_entry.delete(0, "end")
        except Exception as e:
            self.log_box.insert("end", f"❌ Gagal kirim: {e}\n")

    # =====================================================
    # PEMBACAAN SERIAL
    # =====================================================
    def read_serial(self):
        self.running = True
        while self.running:
            try:
                line = self.serial_port.readline().decode(errors="ignore").strip()
                if not line:
                    continue
                self.log_box.insert("end", line + "\n")
                self.log_box.see("end")

                # deteksi pulse
                match = re.search(PULSE_PATTERN, line)
                if match:
                    vals = list(map(int, match.groups()))
                    for i, ax in enumerate(["X", "Y", "Z", "A", "B", "C"]):
                        self.axis_labels[ax].configure(text=str(vals[i]))
                        if ax in self.pulse_history:
                            self.pulse_history[ax].append(vals[i])
                            if len(self.pulse_history[ax]) > self.max_points:
                                self.pulse_history[ax].pop(0)
                # deteksi input tertrigger
                for name in self.indicators:
                    if name in line:
                        dot = self.indicators[name]
                        dot.configure(text_color="lime")
                        self.after(500, lambda d=dot: d.configure(text_color="gray"))

            except Exception:
                pass
            time.sleep(0.05)

    # =====================================================
    # GRAFIK
    # =====================================================
    def update_graph(self):
        for ax in ["X", "Y", "Z"]:
            data = self.pulse_history[ax]
            self.lines[ax].set_data(range(len(data)), data)
        self.ax.relim()
        self.ax.autoscale_view()
        self.canvas.draw_idle()
        self.after(200, self.update_graph)

    def on_close(self):
        self.running = False
        if self.serial_port and self.serial_port.is_open:
            self.serial_port.close()
        self.destroy()


if __name__ == "__main__":
    app = TesterGUI()
    app.mainloop()

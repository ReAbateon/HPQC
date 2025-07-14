import tkinter as tk
from tkinter import ttk, filedialog
import serial.tools.list_ports
import time
import serial
import struct
import threading
import os
import json

class bootloader_talks:

    def __init__(self, root):
        self.root = root
        self.root.title("Bootloader Talks")
        self.root.geometry("600x400")

        self.combo_porte= tk.StringVar()
        self.cronology_file_path = "recent_files.json"

        self.ser = None
        self.serial_lock = threading.Lock()
        self.ack_event = threading.Event()
        self.running = False
        self.serial_thread = None
        

        self.create_widgets()
        self.aggiorna_lista_porte()
    

    def carica_file_recenti(self):
        if os.path.exists(self.cronology_file_path):
            with open(self.cronology_file_path, "r") as f:
                try:
                    return json.load(f)
                except json.JSONDecodeError:
                    return []
        return []
    
    def salva_file_recente(self, path):
        files = self.carica_file_recenti()
        if path not in files:
            files.insert(0, path)
            files = files[:10]  # Teniamo solo gli ultimi 10
            with open(self.cronology_file_path, "w") as f:
                json.dump(files, f)


    def crc_calculator(self):

        def crc32mpeg2(buf, crc=0xffffffff):
            for val in buf:
                crc ^= val << 24
                for _ in range(8):
                    crc = crc << 1 if (crc & 0x80000000) == 0 else (crc << 1) ^ 0x104c11db7
            return crc


        file_path = self.file_path_var.get()

        try:
            with open(file_path, "rb") as f:
                data = f.read()
            
            padding_len = (4 - len(data) % 4) % 4
            data += b'\xFF' * padding_len

            big_endian_bytes = bytearray()

            for i in range(0, len(data), 4):
                word = data[i:i+4]
                big_endian_bytes.extend(word[::-1])  # Inverti i 4 byte

            # Pulisce l'entry e inserisce il CRC in formato esadecimale
            crc_val = crc32mpeg2(big_endian_bytes)
            self.crc_entry.delete(0, tk.END)
            self.crc_entry.insert(0, f"0x{crc_val:08X}")


        except Exception as e:
            self.log_console(f"Errore nel calcolo CRC: {e}")


    def trova_porte_seriali(self):
        porte = serial.tools.list_ports.comports()
        return porte

    def aggiorna_lista_porte(self):
        porte = self.trova_porte_seriali()
        self.combo_porte['values'] = porte
        if porte:
            self.combo_porte.set(porte[0])
        else:
            self.combo_porte.set('Nessuna porta trovata')

    def talk(self):
        try:
            port_line = self.combo_porte.get()
            port = port_line.split(' - ')[0]

            self.ser = serial.Serial(port, 115200, timeout=20)
            self.log_console(f"[+] Porta {port} aperta correttamente.")
            time.sleep(0.1)  # tempo per inizializzare la comunicazione
            
            if hasattr(self, 'extra_win') and self.extra_win.winfo_exists():
                self.btn_invia.config(state="normal")

            self.log_console("In attesa di messaggi")
            
            while self.running:
                with self.serial_lock:
                    try:
                        if self.ser and self.ser.in_waiting >= 4:
                            mess = self.ser.read(4)
                            if len(mess) < 4:
                                self.log_console("[!] Timeout: Disconnessione effettuata.")
                                self.btn_send.config(state="disabled")
                                self.running = False
                                if hasattr(self, 'extra_win') and self.extra_win.winfo_exists():
                                    self.extra_win.destroy()
                                return
                            val = struct.unpack('<I', mess)[0]
                            if val == 0x00000079:
                                self.ack_event.set()
                                self.log_console("ACK ricevuto")
                            elif val == 0x00000000:
                                self.log_console("Il bootloader sta partendo")
                            elif val == 0x01000000:
                                self.log_console("Il bootloader è in attesa del firmware")
                            elif val == 0x02000000:
                                self.log_console("Il bootloader ha trovato App")
                            elif val == 0x03000000:
                                self.log_console("Il bootloader ha completato il caricamento del firmware")
                            elif val == 0x04000000:
                                self.log_console("Il bootloader non ha trovato App")
                            elif val == 0x05000000:
                                self.log_console("Il bootloader ha ricevuto il chunk")
                            elif val == 0x06000000:
                                self.log_console("Il bootloader sta cancellando la copia precedente")
                            elif val == 0x07000000:
                                self.log_console("Il bootloader sta copiando il vecchio firmware")
                            else:
                                self.log_console(f"Non ho riconosciuto il messaggio: 0x{val:08X}")
                    except serial.SerialException as e:
                        self.log_console(f"[!] Errore di comunicazione seriale: {e}")
                        if hasattr(self, 'extra_win') and self.extra_win.winfo_exists():
                            self.btn_invia.config(state="disabled")
                        self.running = False
                        break
                    except OSError as e:
                        self.log_console(f"[!] Errore di I/O: {e}")
                        self.log_console(f"[!] Errore di comunicazione seriale: {e}")
                        if hasattr(self, 'extra_win') and self.extra_win.winfo_exists():
                            self.btn_invia.config(state="disabled")
                        self.running = False
                        break
                
                time.sleep(0.05)

        except serial.SerialException as e:
            self.log_console(f"[!] Errore di comunicazione seriale: {e}")
            if hasattr(self, 'extra_win') and self.extra_win.winfo_exists():
                self.btn_invia.config(state="disabled")
            self.running = False

        finally:
            if 'ser' in locals() and self.ser.is_open:
                self.ser.close()

    def seleziona_file_bin(self):
        path = filedialog.askopenfilename(filetypes=[("Binary files", "*.bin")])
        if path:
            self.file_path_var.set(path)
            self.salva_file_recente(path)
            self.file_combobox['values'] = self.carica_file_recenti()


    def invia_firmware(self, file_path, firmware_v, chunk_size, firmware_crc):
        with open(file_path, "rb") as f:
            firmware_data = f.read()


        header = struct.pack("<LLL", firmware_v, firmware_crc, len(firmware_data))

        # Padding con zeri
        padding = bytes(64 - len(header))

        # Pacchetto completo da 64 byte
        packet = header + padding

        self.ack_event.clear()

        with self.serial_lock:
            self.ser.write(packet)
            self.log_console("[>] Inviato chunk header")
        
        ack_arrivato = self.ack_event.wait(timeout=20)

        if not ack_arrivato:
            self.log_console("[!] Timeout: ACK non ricevuto")
            return 
        
        for i in range(0, len(firmware_data), chunk_size):

            chunk = firmware_data[i:i+chunk_size]

            if(len(chunk) < 64):
                # Padding con 0xFF invece di 0x00
                padding_length = 64 - len(chunk)
                padding = bytes([0xFF] * padding_length)  # Riempie con 0xFF invece di 0x00
                chunk = chunk + padding
            
            self.ack_event.clear()

            with self.serial_lock:
                self.ser.write(chunk)
                self.log_console(f"[>] Inviato chunk {i // chunk_size + 1}")

            ack_arrivato = self.ack_event.wait(timeout=20)

            if not ack_arrivato:
                self.log_console("[!] Timeout: ACK non ricevuto")

        self.log_console("[✓] Invio Completato")                    
        

    def apri_finestra_extra(self):
        self.extra_win = tk.Toplevel(self.root)
        self.extra_win.title("Firmware Send")
        self.extra_win.geometry("530x250")
        self.extra_win.attributes('-topmost', True)
        self.extra_win.resizable(False, False)

        # Label + Entry per il percorso file bin
        lbl_file = tk.Label(self.extra_win, text="File .bin da inviare:", width=20, anchor="w")
        lbl_file.grid(row=0, column=0, padx=10, pady=10)

        self.file_path_var = tk.StringVar()
        self.file_combobox = ttk.Combobox(self.extra_win, textvariable=self.file_path_var, width=40, state="readonly")
        self.file_combobox.grid(row=0, column=1, padx=10, pady=10, sticky="w")

        # Carica file recenti nella combo
        file_list = self.carica_file_recenti()
        self.file_combobox['values'] = file_list
        if file_list:
            self.file_combobox.set(file_list[0])

        # Bottone per scegliere file
        btn_scegli = tk.Button(self.extra_win, text="Sfoglia", command=self.seleziona_file_bin)
        btn_scegli.grid(row=0, column=2, padx=5, pady=10)

        #Label + Entry per chunk size
        lbl_chunk = tk.Label(self.extra_win, text="Chunk size (byte):", width=20, anchor="w")
        lbl_chunk.grid(row=1, column=0, padx=10, pady=10)

        self.chunk_entry = tk.Entry(self.extra_win, width=10)
        self.chunk_entry.grid(row=1, column=1, padx=10, pady=10, sticky="w")
        self.chunk_entry.insert(0, "64")  # valore di default

        #Label + Entry per versione
        lbl_version = tk.Label(self.extra_win, text="Version (hex):", width=20, anchor="w")
        lbl_version.grid(row=2, column=0, padx=10, pady=10)

        self.version_entry = tk.Entry(self.extra_win, width=10)
        self.version_entry.grid(row=2, column=1, padx=10, pady=10, sticky="w")
        self.version_entry.insert(0, "0x00010000")  # valore di default

        #Label + Entry per CRC + Pulsante Calcolo
        lbl_crc = tk.Label(self.extra_win, text="CRC:", width=20, anchor="w")
        lbl_crc.grid(row=3, column=0, padx=10, pady=10)

        self.crc_entry = tk.Entry(self.extra_win, width=10)
        self.crc_entry.grid(row=3, column=1, padx=10, pady=10, sticky="w")

        btn_crc = tk.Button(self.extra_win, text="Calcola", command=self.crc_calculator)
        btn_crc.grid(row=3, column=2, padx=5, pady=10)

        # Bottone "Invia"
        self.btn_invia = tk.Button(self.extra_win, text="Send Firmware", state="disabled", command=self.start_send)
        self.btn_invia.grid(row=4, column=0, columnspan=3, pady=20)
        if self.running:
            self.btn_invia.config(state="normal")


    def create_widgets(self):

        # --- Frame superiore per controlli ---
        frame_top = tk.Frame(self.root)
        frame_top.pack(side="top", fill="x", padx=10, pady=10)

        # Label
        label = tk.Label(frame_top, text="Porte seriali:")
        label.grid(row=0, column=0, padx=10, pady=10)

        # Combo box (menu a tendina per porte)
        self.combo_porte = ttk.Combobox(frame_top, state="readonly")
        self.combo_porte.grid(row=0, column=1, padx=10, pady=10)

        # Bottone per aggiornare
        btn_refresh = tk.Button(frame_top, text="🔄", command=self.aggiorna_lista_porte)
        btn_refresh.grid(row=0, column=2, padx=5, pady=5)

        # Bottone per collegarsi 
        btn_connect = tk.Button(frame_top, text="Connetti", command=self.start_talk)
        btn_connect.grid(row = 0, column=3, padx=5, pady=5)

        spacer = tk.Label(frame_top, text="", width=4)
        spacer.grid(row=0, column=4)

        self.btn_send = tk.Button(frame_top, text="⚙", width=3, state="normal", command=self.apri_finestra_extra)
        self.btn_send.grid(row=0, column=5, padx=5, pady=5)

        # --- Frame inferiore per console + scrollbar ---
        frame_console = tk.Frame(self.root)
        frame_console.pack(side="top", fill="both", expand=True, padx=10, pady=(0, 10))

        # Console output (Text + Scrollbar)
        self.console = tk.Text(frame_console, height=15, width=70, state="disabled", bg="black", fg="lime", font=("Courier", 10))
        self.console.pack(side="left", fill="both", expand=True)

        scrollbar = tk.Scrollbar(frame_console, command=self.console.yview)
        scrollbar.pack(side="right", fill="y")

        self.console.config(yscrollcommand=scrollbar.set)

    def log_console(self, text):
        self.console.configure(state="normal")
        self.console.insert(tk.END, text + "\n")
        self.console.see(tk.END)  # scroll automatico alla fine
        self.console.configure(state="disabled")


    def start_talk(self):
        if not self.running:
            self.running = True
            self.serial_thread = threading.Thread(target=self.talk, daemon=True)
            self.serial_thread.start()

    def start_send(self):
        path = self.file_path_var.get()
        chunk_str = self.chunk_entry.get()
        crc_str = self.crc_entry.get()
        version_str = self.version_entry.get()

        chunk = int(chunk_str)
        crc = int(crc_str, 16)
        version = int(version_str, 16)

        if self.extra_win is not None:
            self.extra_win.destroy()
            self.extra_win = None
        
        thread = threading.Thread(target=self.invia_firmware, args=(path, version, chunk, crc), daemon=True)
        thread.start()

if __name__ == "__main__":
    root = tk.Tk()
    app = bootloader_talks(root)
    root.mainloop()

# 🧭 Tester Board Controller v1.2.1

Program ini merupakan firmware pengujian untuk **board controller CNC** berbasis **STM32 (misal Nucleo-F446RE)** dengan fungsi utama:
- Menguji seluruh **pin output** satu per satu.
- Menampilkan **status pin input** ketika berubah.
- Menghitung **pulsa stepper (6 axis)**.
- Menyimulasikan **sinyal encoder** dengan pengaturan **PPR** dan **RPM maksimum**.

---

## ⚙️ Fitur Utama

| Fitur | Deskripsi |
|-------|------------|
| 🔹 Tes Output Otomatis | Menyalakan seluruh pin output secara bergantian selama 2 detik. |
| 🔹 Tes Output Per Pin | Mengetes satu pin output tertentu dengan perintah `Tes <NAMA_PIN>`. |
| 🔹 Counter Pulsa Stepper | Menghitung pulsa dari pin `STEP_X`–`STEP_C` disertai arah berdasarkan sinyal `DIR_x`. |
| 🔹 Simulasi Encoder | Menghasilkan sinyal encoder **A dan Z** dengan frekuensi sesuai nilai **RPM dan PPR**. |
| 🔹 Deteksi Input | Menampilkan nama pin input yang berubah status secara real-time. |

---

## 🔌 Daftar Pin GPIO

### 🟢 Input (dibaca dari CNC/driver)
| Nama | Pin | Catatan |
|------|-----|---------|
| FLOOD | PC3 | Input aktif HIGH |
| HYDRAULIC | PC12 | Input aktif HIGH |
| DIR_X – DIR_C | PB4, PB10, PA8, PB2, PB15, PB13 | Arah putaran motor step |
| STEP_X – STEP_C | PA10, PB3, PB5, PB1, PB14, PB12 | Pulsa step counter |
| CLEAR_ALRM_DRVR | PC5 | Reset alarm driver |
| SPINDLE_MODE | PC13 | Mode spindle (CW/CCW) |
| OUT_ORIENT – OUT_CLAMP | PC6–PC8 | Output status dari sistem |
| OUT_ATC_CW / CCW | PC9 / PC10 | Status ATC motor |
| LED_MERAH / LED_HIJAU | PC11 / PA9 | Indikator visual |

---

### 🔴 Output (ke mesin/aktuator)
| Nama | Pin | Fungsi |
|------|-----|---------|
| TOMBOL_HOLD / START | PA1 / PB0 | Simulasi tombol manual |
| LIMIT_X – LIMIT_B | PA14, PA4, PA5, PC2, PC4 | Limit switch |
| SPINDLE_DIR | PC1 | Arah spindle |
| ALRM_VFD / ALRM_DRVR | PA6 / PA7 | Alarm indikator |
| TOMBOL_CLAMP / UNCLAMP | PA12 / PA13 | Kontrol penjepit |
| PROXY_* | PC0, PA15, PB6, PB7, PA0, PA11 | Sensor proximity |
| ORIENT_OKE | PD2 | Orientasi spindle |
| OIL_LVL | PC15 | Sensor level oli |

---

### ⚙️ Encoder Simulation
| Nama | Pin | Deskripsi |
|------|-----|------------|
| ENC_A | PB8 | Channel A (toggle) |
| ENC_Z | PB9 | Pulse Z (1 per revolution) |

---

## 🧮 Parameter Encoder

| Parameter | Default | Deskripsi |
|------------|----------|-----------|
| `PPR` | 100 | Pulse per revolution |
| `MAXRPM` | 12000 | Kecepatan maksimum simulasi |
| `S<RPM>` | Contoh: `S1000` | Menyetel RPM aktual simulasi (maks = MAXRPM) |

Timer hardware digunakan:
- `TIM3` → menghasilkan sinyal **A (PB8)**
- `TIM4` → menghasilkan sinyal **Z (PB9)**

---

## 💻 Perintah Serial (115200 baud)

| Perintah | Fungsi |
|-----------|--------|
| `1` | Menjalankan tes otomatis semua output |
| `Tes <NAMA_PIN>` | Mengetes satu output tertentu |
| `2` | Menampilkan data counter pulse stepper setiap 300 ms |
| `3` | Menghentikan tampilan counter pulse |
| `4` | Mereset seluruh counter pulse |
| `5` | Memulai simulasi encoder |
| `6` | Menghentikan simulasi encoder |
| `7` | Menampilkan daftar semua pin yang digunakan |
| `PPR <nilai>` | Mengubah nilai pulse per revolution encoder |
| `MAXRPM <nilai>` | Mengatur kecepatan maksimum encoder |
| `S<RPM>` | Menjalankan simulasi encoder pada RPM tertentu |
| `?` | Menampilkan daftar perintah bantuan |

---

## 🧠 Cara Kerja Singkat

1. **Inisialisasi:**
   - Semua pin input diset `INPUT_PULLUP` atau `INPUT_PULLDOWN` sesuai jenisnya.  
   - Semua pin output diset `OUTPUT` dan diberi default HIGH (non-aktif).  
   - Timer encoder (`TIM3`, `TIM4`) disiapkan tetapi dipause hingga simulasi dimulai.

2. **Loop utama:**
   - Memantau perubahan status pin input.
   - Menampilkan jumlah pulsa setiap 300 ms jika mode “cek pulse” aktif.
   - Memproses perintah dari **Serial Monitor**.

3. **Interrupt Handling:**
   - Setiap pulsa pada pin `STEP_*` memicu interrupt yang menambah/mengurangi counter sesuai arah `DIR_*`.

4. **Simulasi Encoder:**
   - Perintah `S<RPM>` menghitung interval toggle berdasarkan rumus  
     ```
     freq = (RPM × PPR) / 60
     interval = 1e6 / (2 × freq)
     ```
   - `ENC_A` menghasilkan sinyal kuadrat, `ENC_Z` memberi satu pulsa per rotasi.

---

## 🧰 Persyaratan
- **Board:** STM32 (misalnya Nucleo-F446RE)    
- **Baud rate:** 115200  
- **Port serial:** USB virtual COM
- **Serial Monitor:** Bisa menggunakan aplikasi Arduino IDE

---

## 🏁 Versi
**Firmware:** `v1.2.1`  
**Author:** Thoriq Akmal  
**Tanggal:** 5 November 2025

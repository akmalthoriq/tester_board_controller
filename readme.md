# 🧭 Tester Board Controller v1.2.1 (STM32F446RE)

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
| 🔹 Counter Pulsa Stepper | Menghitung pulsa dari pin `STEP_X`–`STEP_C` disertai arah berdasarkan sinyal `DIR_X`-`DIR_C`. |
| 🔹 Simulasi Encoder | Menghasilkan sinyal encoder **A dan Z** dengan frekuensi sesuai nilai **RPM dan PPR**. |
| 🔹 Deteksi Input | Menampilkan nama pin input yang berubah status secara real-time. |

---

## 🔌 Daftar Pin GPIO

### 🟢 Input (dibaca dari CNC/driver)
| Nama | Pin | Catatan |
|------|-----|---------|
| FLOOD | PC3 | Input aktif HIGH |
| HYDRAULIC | PC12 | Input aktif HIGH |
| DIR_X | PB4 | Arah putaran motor step X |
| DIR_Y | PB10 | Arah putaran motor step Y |
| DIR_Z | PA8 | Arah putaran motor step Z |
| DIR_A | PB2 | Arah putaran motor step A |
| DIR_B | PB15 | Arah putaran motor step B |
| DIR_C | PB13 | Arah putaran motor step C |
| STEP_X | PA10 | Pulsa step counter X |
| STEP_Y | PB3 | Pulsa step counter Y |
| STEP_Z | PB5 | Pulsa step counter Z |
| STEP_A | PB1 | Pulsa step counter A |
| STEP_B | PB14 | Pulsa step counter B |
| STEP_C | PB12 | Pulsa step counter C |
| CLEAR_ALRM_DRVR | PC5 | Reset alarm driver |
| SPINDLE_MODE | PC13 | Mode spindle (CW/CCW) |
| OUT_ORIENT | PC6 | Output status spindle orient |
| OUT_CLAMP | PC8 | Output status spindle clamp |
| OUT_ATC_CW | PC9 | Status ATC motor CW |
| OUT_ATC_CCW | PC10 | Status ATC motor CCW |
| LED_MERAH | PC11 | Indikator visual lampu merah |
| LED_HIJAU | PA9 | Indikator visual lampu hijau |

---

### 🔴 Output (ke mesin/aktuator)
| Nama | Pin | Fungsi |
|------|-----|---------|
| TOMBOL_HOLD | PA1 | Simulasi tombol manual PAUSE |
| TOMBOL_START | PB0 | Simulasi tombol manual START |
| LIMIT_X | PA14 | Limit switch axis X |
| LIMIT_Y | PA4 | Limit switch axis Y |
| LIMIT_Z | PA5 | Limit switch axis Z |
| LIMIT_A | PC2 | Limit switch axis A |
| LIMIT_B | PC4 | Limit switch axis B |
| SPINDLE_DIR | PC1 | Arah spindle |
| ALRM_VFD | PA6 | Alarm indikator vfd spindle |
| ALRM_DRVR | PA7 | Alarm indikator driver axis |
| TOMBOL_CLAMP | PA12 | Simulasi tombol clamp fisik |
| TOMBOL_UNCLAMP | PA13 | Simulasi tombol unclamp fisik |
| PROXY_TOOL | PC0 | Sensor proximity TOOL |
| PROXY_UMB_B | PA15 | Sensor proximity UMB_B |
| PROXY_UMB_F | PB6 | Sensor proximity UMB_F |
| PROXY_CLAMP | PB7 | Sensor proximity CLAMP |
| PROXY_UNCLAMP | PA0 | Sensor proximity UNCLAMP |
| PROXY_ATC_POS | PA11 | Sensor proximity ATC_POS |
| ORIENT_OKE | PD2 | Orientasi spindle |
| OIL_LVL | PC15 | Sensor level oli (PENGETESAN OIL_LVL MENGUNAKAN TOMBOL!!)|

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
- **Board:** STM32 (Nucleo-F446RE)  
- **Compiled:** PlatformIO / STM32duino
- **Baud rate:** 115200  
- **Port serial:** USB virtual COM

---

## 🏁 Versi
**Firmware:** `v1.2.1`  
**Author:** Thoriq Akmal  
**Tanggal:** 25 Oktober 2025

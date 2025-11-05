#include <Arduino.h>
#include <HardwareTimer.h>

// --------------------- PIN INPUT --------------------
#define FLOOD PC3
#define DIR_X PB4
#define DIR_Y PB10
#define DIR_Z PA8
#define DIR_A PB2
#define DIR_B PB15
#define DIR_C PB13
#define STEP_X PA10
#define STEP_Y PB3
#define STEP_Z PB5
#define STEP_A PB1
#define STEP_B PB14
#define STEP_C PB12
#define CLEAR_ALRM_DRVR PC5
#define HYDRAULIC PC12
#define SPINDLE_MODE PC13
#define OUT_ORIENT PC6
#define OUT_UMB PC7
#define OUT_CLAMP PC8
#define OUT_ATC_CW PC9
#define OUT_ATC_CCW PC10
#define LED_MERAH PC11
#define LED_HIJAU PA9

// --------------------- PIN OUTPUT --------------------
#define TOMBOL_HOLD PA1
#define TOMBOL_START PB0
#define LIMIT_X PA14
#define LIMIT_Y PA4
#define LIMIT_Z PA5
#define LIMIT_A PC2
#define LIMIT_B PC4
#define SPINDLE_DIR PC1
#define ALRM_VFD PA6
#define ALRM_DRVR PA7
#define TOMBOL_CLAMP PA12
#define TOMBOL_UNCLAMP PA13
#define PROXY_TOOL PC0
#define PROXY_UMB_B PA15
#define PROXY_UMB_F PB6
#define PROXY_CLAMP PB7
#define PROXY_UNCLAMP PA0
#define PROXY_ATC_POS PA11
#define ORIENT_OKE PD2
#define OIL_LVL PC15

// --------------------- PIN ENCODER SIMULATION --------------------
#define ENC_A PB8
#define ENC_Z PB9

// ===================== STRUKTUR DATA =====================
struct InputPin {
  const char* name;
  uint8_t pin;
  bool lastState;
};

InputPin inputPinsLow[] = {
  {"DIR_X", DIR_X}, {"DIR_Y", DIR_Y}, {"DIR_Z", DIR_Z},
  {"DIR_A", DIR_A}, {"DIR_B", DIR_B}, {"DIR_C", DIR_C},
  {"CLEAR_ALRM_DRVR", CLEAR_ALRM_DRVR},
  {"SPINDLE_MODE", SPINDLE_MODE},
  {"OUT_ORIENT", OUT_ORIENT}, {"OUT_UMB", OUT_UMB}, {"OUT_CLAMP", OUT_CLAMP},
  {"LED_MERAH", LED_MERAH}, {"LED_HIJAU", LED_HIJAU}
};

InputPin inputPinsHigh[] = {
  {"FLOOD", FLOOD}, {"HYDRAULIC", HYDRAULIC},{"OUT_ATC_CW", OUT_ATC_CW},
  {"OUT_ATC_CCW", OUT_ATC_CCW}
};

const struct {
  const char* name;
  uint8_t pin;
} outputPins[] = {
  {"TOMBOL_HOLD", TOMBOL_HOLD}, {"TOMBOL_START", TOMBOL_START},
  {"LIMIT_X", LIMIT_X}, {"LIMIT_Y", LIMIT_Y}, {"LIMIT_Z", LIMIT_Z},
  {"LIMIT_A", LIMIT_A}, {"LIMIT_B", LIMIT_B}, {"SPINDLE_DIR", SPINDLE_DIR},
  {"ALRM_VFD", ALRM_VFD}, {"ALRM_DRVR", ALRM_DRVR}, {"TOMBOL_CLAMP", TOMBOL_CLAMP},
  {"TOMBOL_UNCLAMP", TOMBOL_UNCLAMP}, {"PROXY_TOOL", PROXY_TOOL},
  {"PROXY_UMB_B", PROXY_UMB_B}, {"PROXY_UMB_F", PROXY_UMB_F},
  {"PROXY_CLAMP", PROXY_CLAMP}, {"PROXY_UNCLAMP", PROXY_UNCLAMP},
  {"PROXY_ATC_POS", PROXY_ATC_POS},
  {"ORIENT_OKE", ORIENT_OKE}, {"OIL_LVL", OIL_LVL}
};

// ===================== PULSE COUNTER =====================
volatile long stepCountX = 0, stepCountY = 0, stepCountZ = 0;
volatile long stepCountA = 0, stepCountB = 0, stepCountC = 0;

void countStepX() { digitalRead(DIR_X) ? stepCountX++ : stepCountX--; }
void countStepY() { digitalRead(DIR_Y) ? stepCountY++ : stepCountY--; }
void countStepZ() { digitalRead(DIR_Z) ? stepCountZ++ : stepCountZ--; }
void countStepA() { digitalRead(DIR_A) ? stepCountA++ : stepCountA--; }
void countStepB() { digitalRead(DIR_B) ? stepCountB++ : stepCountB--; }
void countStepC() { digitalRead(DIR_C) ? stepCountC++ : stepCountC--; }

#define TRY_ATTACH_INTERRUPT(pin, func) \
  do { if (digitalPinToInterrupt(pin) != NOT_AN_INTERRUPT) attachInterrupt(digitalPinToInterrupt(pin), func, RISING); } while(0)

// ===================== VARIABEL GLOBAL =====================
bool showPulse = false;

bool simulateEncoder = false;
bool encoderState = false;

unsigned int encoderPPR = 100;     
unsigned int encoderMaxRPM = 12000; 
float currentRPM = 0;

// Timer hardware (gunakan timer bebas, misal TIM3 dan TIM4)
HardwareTimer *encoderTimer = new HardwareTimer(TIM3); // untuk channel A
HardwareTimer *zTimer = new HardwareTimer(TIM4);       // untuk pulse Z


// ===================== SETUP =====================
void setup() {
  Serial.begin(115200);
  delay(3000);
  Serial.println("----------------------------------------");
  Serial.println("\n=== TESTER BOARD CONTROLLER V1.2.1 ===");
  Serial.println("Perintah yang tersedia: cukup ketik nomor saja");
  Serial.println(" 1 - mulai tes -> untuk mengetes semua output bergantian secara otomatis");
  Serial.println(" 2 - cek pulse -> untuk melihat hitungan pulsa");
  Serial.println(" 3 - stop cek pulse -> untuk menghentikan tampilan pulsa");
  Serial.println(" 4 - reset pulse -> untuk mereset semua counter pulse");
  Serial.println(" 5 - mulai encoder -> untuk memulai simulasi sinyal encoder");
  Serial.println(" 6 - stop encoder -> untuk menghentikan simulasi sinyal encoder");
  Serial.println(" 7 - daftar pin -> untuk menampilkan daftar pin input dan output yang digunakan");
  Serial.println(" jika hanya ingin tes salah satu pin output, gunakan command -> Tes <NAMA_PIN>");
  Serial.println(" jika ingin mengatur simulasi encoder, gunakan command -> S<RPM>");
  Serial.println(" untuk mengatur PPR gunakan command -> PPR <nilai>");
  Serial.println("----------------------------------------");

    // Inisialisasi input LOW
  for (auto &in : inputPinsLow) {
    pinMode(in.pin, INPUT_PULLUP);
    in.lastState = digitalRead(in.pin);
  }

   for (auto &in : inputPinsHigh) {
    pinMode(in.pin, INPUT_PULLDOWN);
    in.lastState = digitalRead(in.pin);
  }

  // Inisialisasi output
  for (auto &out : outputPins) {
    pinMode(out.pin, OUTPUT);
    digitalWrite(out.pin, HIGH);
  }

  // Inisialisasi pin encoder
  pinMode(ENC_A, OUTPUT);
  pinMode(ENC_Z, OUTPUT);
  digitalWrite(ENC_A, LOW);
  digitalWrite(ENC_Z, LOW);

  // ===================== INISIALISASI HARDWARE TIMER ENCODER =====================
  encoderTimer->setMode(1, TIMER_OUTPUT_COMPARE_TOGGLE, ENC_A);  // TIM3_CH1 = PB8
  encoderTimer->setOverflow(1000000, MICROSEC_FORMAT);            // default off
  encoderTimer->pause();

  zTimer->setOverflow(1000000, MICROSEC_FORMAT);  // default off
  zTimer->attachInterrupt([](){
    digitalWrite(ENC_Z, HIGH);
    delayMicroseconds(200);   // durasi pulse Z
    digitalWrite(ENC_Z, LOW);
  });
  zTimer->pause();

  // Attach interrupt untuk STEP pin
  TRY_ATTACH_INTERRUPT(STEP_X, countStepX);
  TRY_ATTACH_INTERRUPT(STEP_Y, countStepY);
  TRY_ATTACH_INTERRUPT(STEP_Z, countStepZ);
  TRY_ATTACH_INTERRUPT(STEP_A, countStepA);
  TRY_ATTACH_INTERRUPT(STEP_B, countStepB);
  TRY_ATTACH_INTERRUPT(STEP_C, countStepC);
}

// ===================== LOOP =====================
void loop() {
  // === Deteksi perubahan input ===
  for (auto &in : inputPinsLow) {
    bool currentState = digitalRead(in.pin);
    if (in.lastState == HIGH && currentState == LOW) {
      Serial.print("PIN "); Serial.print(in.name); Serial.println(" TERDETEKSI");
    }
    in.lastState = currentState;
  }

  // === Deteksi perubahan input ===
  for (auto &in : inputPinsHigh) {
    bool currentState = digitalRead(in.pin);
    if (in.lastState == LOW && currentState == HIGH) {
      Serial.print("PIN "); Serial.print(in.name); Serial.println(" TERDETEKSI");
    }
    in.lastState = currentState;
  }

  // === Tampilkan pulse counter ===
  static unsigned long lastPrint = 0;
  if (showPulse && millis() - lastPrint >= 300) {
    Serial.printf("Pulse X:%ld | Y:%ld | Z:%ld | A:%ld | B:%ld | C:%ld\n",
                  stepCountX, stepCountY, stepCountZ, stepCountA, stepCountB, stepCountC);
    lastPrint = millis();
  }

  // === Baca perintah serial ===
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    if (cmd.equalsIgnoreCase("1")) { //mulai tes
      Serial.println("Menjalankan tes semua output...");
      for (auto &out : outputPins) {
        Serial.print("Tes "); Serial.println(out.name);
        digitalWrite(out.pin, LOW);
        delay(2000);
        digitalWrite(out.pin, HIGH);
      }
      Serial.println("Tes selesai.");
    }
    //TAMBAHAN COMMAND TESTING PER PIN OUTPUT
    else if (cmd.startsWith("Tes ")) {
      String pinName = cmd.substring(4);
      bool found = false;
      for (auto &out : outputPins) {
        if (pinName.equalsIgnoreCase(out.name)) {
          found = true;
          Serial.print("Mengetes pin "); Serial.println(out.name);
          digitalWrite(out.pin, LOW);
          delay(2000);
          digitalWrite(out.pin, HIGH);
          Serial.println("Tes selesai.");
          break;
        }
      }
      if (!found) {
        Serial.println("Nama pin output tidak dikenali.");
      }
    }
    else if (cmd.equalsIgnoreCase("2")) { //cek pulse
      showPulse = true;
      Serial.println("Menampilkan data pulse setiap 300ms...");
    }
    else if (cmd.equalsIgnoreCase("3")) {//stop cek pulse
      showPulse = false;
      Serial.println("Menutup tampilan data pulse.");
    }
    else if (cmd.equalsIgnoreCase("4")) { //reset pulse
      stepCountX = stepCountY = stepCountZ = stepCountA = stepCountB = stepCountC = 0;
      Serial.println("Semua counter pulse telah direset.");
    }
    else if (cmd.equalsIgnoreCase("5")) { // mulai encoder
      simulateEncoder = true;
      encoderTimer->resume();
      zTimer->resume();
      Serial.println("Simulasi encoder dimulai (PB8=A, PB9=Z)...");
    }
    else if (cmd.equalsIgnoreCase("6")) { // stop encoder
      simulateEncoder = false;
      encoderTimer->pause();
      zTimer->pause();
      digitalWrite(ENC_A, LOW);
      digitalWrite(ENC_Z, LOW);
      Serial.println("Simulasi encoder dihentikan.");
    }

    //tambahan untuk menampilkan pin yang digunakan
else if (cmd.equalsIgnoreCase("7")) {
  Serial.println("=== Daftar Semua Pin yang Digunakan ===");

  // ----- INPUT -----
  Serial.println("\n-- INPUT --");
  Serial.println("FLOOD = PC3");
  Serial.println("DIR_X = PB4");
  Serial.println("DIR_Y = PB10");
  Serial.println("DIR_Z = PA8");
  Serial.println("DIR_A = PB2");
  Serial.println("DIR_B = PB15");
  Serial.println("DIR_C = PB13");
  Serial.println("STEP_X = PA10");
  Serial.println("STEP_Y = PB3");
  Serial.println("STEP_Z = PB5");
  Serial.println("STEP_A = PB1");
  Serial.println("STEP_B = PB14");
  Serial.println("STEP_C = PB12");
  Serial.println("CLEAR_ALRM_DRVR = PC5");
  Serial.println("HYDRAULIC = PC12");
  Serial.println("SPINDLE_MODE = PC13");
  Serial.println("OUT_ORIENT = PC6");
  Serial.println("OUT_UMB = PC7");
  Serial.println("OUT_CLAMP = PC8");
  Serial.println("OUT_ATC_CW = PC9");
  Serial.println("OUT_ATC_CCW = PC10");
  Serial.println("LED_MERAH = PC11");
  Serial.println("LED_HIJAU = PA9");

  // ----- OUTPUT -----
  Serial.println("\n-- OUTPUT --");
  Serial.println("TOMBOL_HOLD = PA1");
  Serial.println("TOMBOL_START = PB0");
  Serial.println("LIMIT_X = PA14");
  Serial.println("LIMIT_Y = PA4");
  Serial.println("LIMIT_Z = PA5");
  Serial.println("LIMIT_A = PC2");
  Serial.println("LIMIT_B = PC4");
  Serial.println("SPINDLE_DIR = PC1");
  Serial.println("ALRM_VFD = PA6");
  Serial.println("ALRM_DRVR = PA7");
  Serial.println("TOMBOL_CLAMP = PA12");
  Serial.println("TOMBOL_UNCLAMP = PA13");
  Serial.println("PROXY_TOOL = PC0");
  Serial.println("PROXY_UMB_B = PA15");
  Serial.println("PROXY_UMB_F = PB6");
  Serial.println("PROXY_CLAMP = PB7");
  Serial.println("PROXY_UNCLAMP = PA0");
  Serial.println("PROXY_ATC_POS = PA11");
  Serial.println("ORIENT_OKE = PD2");
  Serial.println("OIL_LVL = PC15");

  // ----- ENCODER -----
  Serial.println("\n-- ENCODER SIMULATION --");
  Serial.println("ENC_A = PB8");
  Serial.println("ENC_Z = PB9");

  Serial.println("\n----------------------------------------");
}

//simulasi encoder
    else if (cmd.startsWith("PPR ")) {
  int newPPR = cmd.substring(4).toInt();
  if (newPPR > 0) {
    encoderPPR = newPPR;
    Serial.printf("Nilai PPR encoder diubah menjadi %u\n", encoderPPR);
  } else {
    Serial.println("Format salah. Gunakan contoh: PPR 200");
  }
}
else if (cmd.startsWith("MAXRPM ")) {
  int newMax = cmd.substring(7).toInt();
  if (newMax > 0) {
    encoderMaxRPM = newMax;
    Serial.printf("Nilai maksimum RPM encoder diubah menjadi %u\n", encoderMaxRPM);
  } else {
    Serial.println("Format salah. Gunakan contoh: MAXRPM 3000");
  }
}
else if (cmd.startsWith("S")) {
  int rpm = cmd.substring(1).toInt();
  if (rpm > 0 && rpm <= encoderMaxRPM) {
    currentRPM = rpm;

    // Hitung frekuensi encoder
    double freq = (rpm * encoderPPR) / 60.0;  // Hz
    double interval_us = 1e6 / (2.0 * freq);  // toggle setiap setengah siklus

    // Atur timer encoder
    encoderTimer->pause();
    encoderTimer->setOverflow(interval_us, MICROSEC_FORMAT);
    if (simulateEncoder) encoderTimer->resume();  // aktif hanya jika simulasi ON

    // Atur timer pulse Z (1 pulse per putaran)
    zTimer->pause();
    double period_us = (60e6 / rpm);  // waktu 1 putaran
    zTimer->setOverflow(period_us, MICROSEC_FORMAT);
    if (simulateEncoder) zTimer->resume();

    Serial.printf("Simulasi encoder diset ke %d RPM (PPR=%u, interval=%.2f µs)\n",
                  rpm, encoderPPR, interval_us);
  } else if (rpm > encoderMaxRPM) {
    Serial.printf("RPM terlalu tinggi! Maksimum adalah %u RPM.\n", encoderMaxRPM);
  } else {
    Serial.println("Format salah. Gunakan contoh: S1000 (untuk 1000 RPM)");
  }
}


    else if (cmd.equalsIgnoreCase("?")) {
    Serial.println("----------------------------------------");
 // Serial.println("\n=== TESTER BOARD CONTROLLER V1.2.1 ===");
  Serial.println("Perintah yang tersedia: cukup ketik nomor saja");
  Serial.println(" 1 - mulai tes -> untuk mengetes semua output bergantian secara otomatis");
  Serial.println(" 2 - cek pulse -> untuk melihat hitungan pulsa");
  Serial.println(" 3 - stop cek pulse -> untuk menghentikan tampilan pulsa");
  Serial.println(" 4 - reset pulse -> untuk mereset semua counter pulse");
  Serial.println(" 5 - mulai encoder -> untuk memulai simulasi sinyal encoder");
  Serial.println(" 6 - stop encoder -> untuk menghentikan simulasi sinyal encoder");
  Serial.println(" 7 - daftar pin -> untuk menampilkan daftar pin input dan output yang digunakan");
  Serial.println(" jika hanya ingin tes salah satu pin output, gunakan command -> Tes <NAMA_PIN>");
  Serial.println(" jika ingin mengatur simulasi encoder, gunakan command -> S<RPM>");
  Serial.println(" untuk mengatur PPR gunakan command -> PPR <nilai>");
  Serial.println("----------------------------------------");

    }
    else {
      Serial.println("Perintah tidak dikenali.");
    }
  }
}

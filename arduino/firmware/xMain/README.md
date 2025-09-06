# Biped Robot Firmware (x*)

Bu klasör: `v2/firmware/robot_v2` – Ana sketch: `xMain.ino`

Tamamen modüler, NDJSON seri protokollü biped firmware. 8 servo + 2 stepper, MPU6050 ile dengeleme, basit IK, Stand/Sit modları ve güvenlik.

## Özellikler
- 8 Servo: L/R kalça, diz, bilek + baş tilt/pan (easing ile yumuşak hareket)
- 2 Stepper (skate/paten): hız (vel) ve konum (pos) modları, Sit modunda dengeleme
- IMU: MPU6050 ile pitch/roll; komplemanter filtre opsiyonu, offset kalibrasyonu
- IK: 2D bacak ters kinematik ve mirror
- NDJSON seri API @115200: her satır bir JSON komutu → JSON cevap
- Modlar: Stand (servo dengeleme), Sit/Skate (stepper hız ile dengeleme)
- Güvenlik: heartbeat/timeout (deadman), estop, açı sınırları
- Kalıcılık: IMU offset EEPROM kaydet/yükle, boot’ta otomatik yükleme
- Tuning: PID ve servo/stepper parametrelerini çalışma anında ayarla

## Bağlantı ve Kurulum
1) Donanım pinleri (varsayılan):
	 - Servolar: L(2,3,4) R(5,6,7) Baş: Pan=8, Tilt=9
	 - Stepper sürücüler (A4988/TMC gibi): S1 STEP=10 DIR=11; S2 STEP=12 DIR=13
	 - Limit switch (opsiyonel): `PIN_LIMIT1`/`PIN_LIMIT2` (aktif LOW varsayılan)
2) Seri port seçimi: `xConfig.h` içinde `SERIAL_IO` → `Serial` (USB) veya `Serial1` (RPi UART)
3) IDE/Kütüphaneler: Arduino Mega 2560, AccelStepper, Adafruit_MPU6050, Adafruit Unified Sensor
4) Yükleme: `xMain.ino`’yu açıp 115200 8N1 ile çalıştırın.

## Çalışma Modları ve Dengeleme
- Stand: Servo dengeleme aktiftir. IMU’dan pitch/roll okunur; kalça (hip) eklemlerine sınırlı düzeltme uygulanır.
- Sit: Oturma animasyonu sonrası diz ve bilek (1,2,4,5) detach edilir; kalça ve baş aktif kalır. Dengeleme için stepper’larda hız sürüşü kullanılabilir.
- Heartbeat: 50–100 ms sıklıkla `{ "cmd":"hb" }` gönderin. Zaman aşımı (`HEARTBEAT_TIMEOUT_MS`) olursa estop devreye girer.
- Estop: `{ "cmd":"estop" }` tüm servoları detach eder, stepper hızlarını durdurur.

## Komut Referansı
Genel: Her komut tek satır JSON’dur. Cevap `{ "ok":true }` veya `{ "ok":false, "err":"..." }` şeklindedir.

- Ping: `{ "cmd":"hello" }`
- Heartbeat: `{ "cmd":"hb" }`
- Tek servo yaz: `{ "cmd":"set_servo", "index":0, "deg":90 }`
	- index: 0..7 (0-2 sol bacak; 3-5 sağ bacak; 6 tilt; 7 pan)
	- deg: 0..180; eklem sınırları otomatik kısılır
- Poz yaz: `{ "cmd":"set_pose", "pose":[..8..], "duration_ms":1000 }`
	- pose: 8 elemanlı derece dizisi
	- duration_ms (opsiyonel): easing hızı için hedef süre
- IK (bacak): `{ "cmd":"leg_ik", "x":120, "side":"L" }`
	- x: kalça-bilek mesafesi (mm). Çözüm geçerliyse o bacak servoları yazılır.
	- side: "L" veya "R"
- Stepper sür: `{ "cmd":"stepper", "id":0, "mode":"pos|vel", "value":1000 }`
	- id: 0 veya 1
	- mode: pos (hedef konum step), vel (hız steps/s)
	- value: mode’a göre hedef değer
- Stepper ayar: `{ "cmd":"stepper_cfg", "maxSpeed":2000, "accel":1000 }`
- Homing: `{ "cmd":"home" }` (limit switch varsa). Yoksa:
	- Anlık sıfırla: `{ "cmd":"zero_now" }`
	- Özel ofset: `{ "cmd":"zero_set", "p1":0, "p2":0 }`
- Modlar: `{ "cmd":"stand" }` / `{ "cmd":"sit" }`
- IMU oku: `{ "cmd":"imu_read" }` → `{ "ok":true, "pitch":..., "roll":... }`
- IMU kalibre: `{ "cmd":"imu_cal" }` (mevcut duruşu 0 kabul eder)
- PID aç/kapat: `{ "cmd":"pid", "enable":true }`
- Durum: `{ "cmd":"get_state" }` → mod, pid açık/kapalı vb.
- Telemetri: `{ "cmd":"telemetry_start", "interval_ms":50 }` / `{ "cmd":"telemetry_stop" }`
- Canlı ayar: `{ "cmd":"tune", "pid":{...}, "skate":{...}, "servoSpeed":60 }`
- EEPROM: `{ "cmd":"eeprom_save" }` / `{ "cmd":"eeprom_load" }`
- Acil dur: `{ "cmd":"estop" }`

## Servo ve Stepper Davranışları
- Servo easing: `SPEED_DEG_PER_S` ile saniyelik derece hızı belirlenir; `duration_ms` verilirse hedefe bu sürede ulaşacak şekilde hız ayarlanır.
- Eklem sınırları: Kalça/Knee/Ankle/Head için `xConfig.h` limitleri uygulanır; komutlar bu sınırların dışına çıkamaz.
- Stepper modları:
	- pos: `moveTo`/`move` ile hedef adıma gider; hız/ivme `stepper_cfg` ile sınırlıdır.
	- vel: `setSpeed` ile sabit hızda döner; `stop` veya yeni komutla durdurulur.
	- Homing yoksa konum göreli kabul edilir; `zero_now/zero_set` ile referans alın.

## İpuçları
- VS Code’ta Arduino.h uyarıları editöre özgüdür; Arduino IDE derlemesini etkilemez.
- RPi üzerinden Serial1 bağlayacaksanız seviye dönüştürücü ve ortak GND şarttır.
- Heartbeat’i kesmeyin; aksi halde estop tetiklenir.

## Lisans
Üst dizindeki LICENSE dosyasına bakın.

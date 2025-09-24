# SentryBOT Arduino Firmware

Bu dizin, biped robot firmware'ini içerir. Ana sketch `arduino/firmware/xMain/xMain.ino` altındadır. Firmware NDJSON seri protokolüyle haberleşir, 8 servo + 2 stepper, IMU tabanlı dengeleme, IK ve çeşitli çevre birimlerini destekler.

## Özellikler
- 8 Servo: L/R kalça, diz, bilek + baş tilt/pan (easing ile yumuşak)
- 2 Stepper (skate): hız ve konum modları, Sit modunda dengeleme
- IMU: MPU6050 (I2C)
- IK: 2D bacak ters kinematik + mirror
- NDJSON seri API @115200
- Modlar: Stand (servo dengeleme), Sit/Skate (stepper dengeleme)
- Güvenlik: heartbeat timeout, estop, açı sınırları
- Kalıcılık: IMU offset EEPROM kaydet/yükle
- Tuning: PID, servo/stepper parametreleri canlı ayar

## Bağlantı ve Kurulum
1) Seri port: `xConfig.h` içinde `SERIAL_IO` → `Serial` (USB) veya `Serial1` (RPi UART)
2) Gerekli kart/kütüphaneler: Arduino Mega 2560, (opsiyonel) MFRC522, LiquidCrystal_I2C
3) Yükleme: `arduino/firmware/xMain/xMain.ino`’yu açın, 115200 8N1.

## Servo Kontrolü (I2C PCA9685)
- `SERVO_USE_PCA9685` = 1 iken tüm servolar I2C üzerinden PCA9685 ile sürülür (varsayılan: 1).
- `PCA9685_ADDR` (varsayılan 0x40), `SERVO_FREQ_HZ` (50Hz), `SERVO_MIN_US`/`SERVO_MAX_US` (500..2500us) `xConfig.h`’den ayarlanır.
- Kanal eşlemesi için mevcut `PIN_*` tanımları PCA9685 kanal numarası (0..15) olarak kullanılır:
  - L HIP=0, L KNEE=1, L ANKLE=2
  - R HIP=15, R KNEE=14, R ANKLE=13
  - HEAD PAN=3, HEAD TILT=12
- Doğrudan Arduino Servo pinlerine dönmek isterseniz `SERVO_USE_PCA9685=0` yapın ve `PIN_*` değerlerini servo pinleriyle değiştirin.

## Lazer Kontrolü
- Tek lazer aç: `{ "cmd":"laser", "id":1, "on":true }` (veya id=2)
- Çift lazer aç: `{ "cmd":"laser", "both":true, "on":true }`
- Kapat: `{ "cmd":"laser", "on":false }`
- Pin ve polarite: `LASER1_PIN`, `LASER2_PIN`, `LASER_ACTIVE_HIGH` (`xConfig.h`).

## Komut Referansı (Özet)
- Ping: `{ "cmd":"hello" }`
- Heartbeat: `{ "cmd":"hb" }`
- Tek servo: `{ "cmd":"set_servo", "index":0, "deg":90 }`
- Poz: `{ "cmd":"set_pose", "pose":[..8..], "duration_ms":1000 }`
- IK: `{ "cmd":"leg_ik", "x":120, "side":"L" }`
- Stepper: `{ "cmd":"stepper", "id":0, "mode":"pos|vel", "value":1000, "drive":200 }`
- Stepper cfg: `{ "cmd":"stepper_cfg", "maxSpeed":2000, "accel":1000 }`
- Homing: `{ "cmd":"home" }` / Sıfırla: `{ "cmd":"zero_now" }` / `{ "cmd":"zero_set", "p1":0, "p2":0 }`
- Mod: `{ "cmd":"stand" }` / `{ "cmd":"sit" }`
- IMU: `{ "cmd":"imu_read" }` / `{ "cmd":"imu_cal" }`
- PID: `{ "cmd":"pid", "enable":true }`
- Durum: `{ "cmd":"get_state" }`
- Telemetri: `{ "cmd":"telemetry_start", "interval_ms":50 }` / `{ "cmd":"telemetry_stop" }`
- Tuning: `{ "cmd":"tune", "pid":{...}, "skate":{...}, "servoSpeed":60 }`
- EEPROM: `{ "cmd":"eeprom_save" }` / `{ "cmd":"eeprom_load" }`
- Estop: `{ "cmd":"estop" }`

## Çevre Birimleri
- RFID (MFRC522): `{ "cmd":"rfid_last" }` ve olay yayını
- LCD (I2C): `{ "cmd":"lcd", "msg":"HELLO" }`
- Ultrasonik: `{ "cmd":"ultra_read" }`, kaçınma `{ "cmd":"avoid", "enable":true }`

## Notlar
- VS Code’ta `Arduino.h` uyarısı IntelliSense kaynaklıdır; Arduino IDE derlemesini etkilemez.
- RPi üzerinden UART bağlarken seviye dönüştürücü ve ortak GND şart.
- Heartbeat’i (50–100ms) kesmeyin; aksi halde estop tetiklenir.

## Lisans
Üst dizindeki `LICENSE` dosyasına bakın.

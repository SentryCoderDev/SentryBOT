#ifndef ROBOT_PERIPHERALS_H
#define ROBOT_PERIPHERALS_H

#include <Arduino.h>
#include "xConfig.h"

#if RFID_ENABLED
#include <SPI.h>
#include <MFRC522.h>
#endif

#if LCD_ENABLED
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#endif

// Lightweight wrappers to keep xMain clean

class Ultrasonic {
public:
  void begin(uint8_t trigPin, uint8_t echoPin){
    _trig=trigPin; _echo=echoPin; pinMode(_trig, OUTPUT); pinMode(_echo, INPUT);
    _lastMs = 0; _lastCm = NAN;
  }
  // Measure with small timeouts to avoid blocking long
  bool measureIfDue(unsigned long intervalMs){
    unsigned long now = millis();
    if (now - _lastMs < intervalMs) return false;
    _lastMs = now;
    digitalWrite(_trig, LOW); delayMicroseconds(2);
    digitalWrite(_trig, HIGH); delayMicroseconds(10);
    digitalWrite(_trig, LOW);
    unsigned long dur = pulseIn(_echo, HIGH, 30000UL); // 30ms timeout ≈ ~5m
    if (dur==0){ _lastCm = NAN; }
    else { _lastCm = (float)dur / 58.0f; }
    return true;
  }
  float lastCm() const { return _lastCm; }
private:
  uint8_t _trig=255, _echo=255; unsigned long _lastMs=0; float _lastCm=NAN;
};

#if RFID_ENABLED
class RfidReader {
public:
  void begin(uint8_t ssPin, uint8_t rstPin){
    SPI.begin(); _mfrc = new MFRC522(ssPin, rstPin); _mfrc->PCD_Init();
    _lastUid = ""; _lastSeenMs=0;
  }
  // Returns true when a new UID is read
  bool poll(){
    if (!_mfrc) return false;
    if (!_mfrc->PICC_IsNewCardPresent() || !_mfrc->PICC_ReadCardSerial()) return false;
    String uid = uidToHex(_mfrc->uid);
    bool isNew = (uid != _lastUid);
    _lastUid = uid; _lastSeenMs = millis();
    _mfrc->PICC_HaltA(); _mfrc->PCD_StopCrypto1();
    if (isNew) { _lastEventUid = uid; return true; }
    return false;
  }
  const String &lastUid() const { return _lastUid; }
  // If a new UID was just read in poll(), returns it once; else empty
  String takeLastEvent(){ String t=_lastEventUid; _lastEventUid=""; return t; }
private:
  static String uidToHex(const MFRC522::Uid &u){
    String s; for (byte i=0;i<u.size;i++){ if (i) s+=""; if (u.uidByte[i]<16) s += "0"; s += String(u.uidByte[i], HEX); }
    s.toUpperCase(); return s;
  }
  MFRC522 *_mfrc{nullptr}; String _lastUid; String _lastEventUid; unsigned long _lastSeenMs{0};
};
#endif

#if LCD_ENABLED
class LcdDisplay {
public:
  void begin(){
    int hwRows = (LCD_ROWS==1?2:LCD_ROWS); // bazı 16x1 modüller 8x2 adresleme kullanır
    _lcd = new LiquidCrystal_I2C(LCD_I2C_ADDR, LCD_COLS, hwRows);
    _lcd->init(); _lcd->backlight(); clear();
  }
  void clear(){
    if (!_lcd) return;
    _lcd->clear();
    // 16x1 büyük font (8x2 adresleme) için iki yarıyı boşlukla temizle
    if (LCD_ROWS==1){
      _lcd->setCursor(0,0); _lcd->print("        ");
      if (LCD_16X1_SPLIT_ROW){
        _lcd->setCursor(0,1);
      } else {
        _lcd->setCursor(8,0);
      }
      _lcd->print("        ");
      _lcd->setCursor(0,0);
    } else {
      _lcd->setCursor(0,0);
    }
  }
  void printLine(const String &msg){
    if (!_lcd) return;
    String m = msg;
    if ((int)m.length()>LCD_COLS) m = m.substring(0, LCD_COLS);
    if (LCD_ROWS==1){
      // 16x1 büyük font: ilk 8 karakter satır 0'a, sonraki 8 satır 1'e yazılır
      String s0 = m.substring(0, min(8, (int)m.length()));
      while ((int)s0.length() < 8) s0 += ' ';
      String s1 = (m.length()>8)? m.substring(8) : String("");
      while ((int)s1.length() < 8) s1 += ' ';
      _lcd->setCursor(0,0); _lcd->print(s0);
      if (LCD_16X1_SPLIT_ROW){
        _lcd->setCursor(0,1);
      } else {
        _lcd->setCursor(8,0);
      }
      _lcd->print(s1);
      _lcd->setCursor(0,0);
    } else {
      // Klasik 16x2 vb.
      // Satırı temizle ve yaz
      _lcd->setCursor(0,0);
      for (int i=0;i<LCD_COLS;i++) _lcd->print(' ');
      _lcd->setCursor(0,0); _lcd->print(m);
    }
  }
private:
  LiquidCrystal_I2C *_lcd{nullptr};
};
#endif

#if LASER_ENABLED
class LaserPair {
public:
  void begin(uint8_t pin1, uint8_t pin2){
    _p1 = pin1; _p2 = pin2; pinMode(_p1, OUTPUT); pinMode(_p2, OUTPUT); off();
  }
  void oneOn(uint8_t idx){
    if (idx==1) digitalWrite(_p1, LASER_ACTIVE_HIGH?HIGH:LOW);
    else if (idx==2) digitalWrite(_p2, LASER_ACTIVE_HIGH?HIGH:LOW);
  }
  void bothOn(){
    digitalWrite(_p1, LASER_ACTIVE_HIGH?HIGH:LOW);
    digitalWrite(_p2, LASER_ACTIVE_HIGH?HIGH:LOW);
  }
  void off(){
    digitalWrite(_p1, LASER_ACTIVE_HIGH?LOW:HIGH);
    digitalWrite(_p2, LASER_ACTIVE_HIGH?LOW:HIGH);
  }
private:
  uint8_t _p1{255}, _p2{255};
};
#endif

#endif // ROBOT_PERIPHERALS_H
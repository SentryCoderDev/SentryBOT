#ifndef ROBOT_STEPPER_PAIR_H
#define ROBOT_STEPPER_PAIR_H

#include <Arduino.h>
#include <AccelStepper.h>
#include "../xConfig.h"

class StepperPair {
public:
  void begin(){
    s1 = AccelStepper(AccelStepper::DRIVER, PIN_STEPPER1_STEP, PIN_STEPPER1_DIR);
    s2 = AccelStepper(AccelStepper::DRIVER, PIN_STEPPER2_STEP, PIN_STEPPER2_DIR);
    s1.setMaxSpeed(2000); s1.setAcceleration(1000);
    s2.setMaxSpeed(2000); s2.setAcceleration(1000);
    mode1 = MODE_POS; mode2 = MODE_POS;
    // Configure limit pins if available
    if (PIN_LIMIT1>=0){ pinMode(PIN_LIMIT1, LIMIT_ACTIVE_LOW?INPUT_PULLUP:INPUT); }
    if (PIN_LIMIT2>=0){ pinMode(PIN_LIMIT2, LIMIT_ACTIVE_LOW?INPUT_PULLUP:INPUT); }
  }
  void setMaxSpeed(float v){ s1.setMaxSpeed(v); s2.setMaxSpeed(v); }
  void setAcceleration(float a){ s1.setAcceleration(a); s2.setAcceleration(a); }

  void moveTo(long p1, long p2){ s1.moveTo(p1); s2.moveTo(p2); }
  void moveBy(long d1, long d2){ s1.move(d1); s2.move(d2); }
  void setSpeed(float v1, float v2){ mode1=MODE_VEL; mode2=MODE_VEL; s1.setSpeed(v1); s2.setSpeed(v2); }

  // Single-stepper helpers (renamed to avoid overload ambiguity)
  void setModeOne(uint8_t id, bool vel){ if(id==0) mode1 = vel?MODE_VEL:MODE_POS; else mode2 = vel?MODE_VEL:MODE_POS; }
  void setSpeedOne(uint8_t id, float v){ if(id==0){ mode1=MODE_VEL; s1.setSpeed(v);} else { mode2=MODE_VEL; s2.setSpeed(v);} }
  void moveToOne(uint8_t id, long p){ if(id==0){ mode1=MODE_POS; s1.moveTo(p);} else { mode2=MODE_POS; s2.moveTo(p);} }
  void moveByOne(uint8_t id, long d){ if(id==0){ mode1=MODE_POS; s1.move(d);} else { mode2=MODE_POS; s2.move(d);} }

  void update(){
    if (mode1==MODE_VEL) s1.runSpeed(); else s1.run();
    if (mode2==MODE_VEL) s2.runSpeed(); else s2.run();
  }

  long pos1() const { return s1.currentPosition(); }
  long pos2() const { return s2.currentPosition(); }

  void stop(){
    // Immediate velocity stop; keep modes in velocity for safety
    mode1 = MODE_VEL; mode2 = MODE_VEL;
    s1.setSpeed(0); s2.setSpeed(0);
  }

  void zeroNow(){ s1.setCurrentPosition(0); s2.setCurrentPosition(0); }
  void zeroSet(long p1, long p2){ s1.setCurrentPosition(p1); s2.setCurrentPosition(p2); }

  // Blocking simple homing towards negative direction until limit switch is hit
  void homeBoth(long speed = -400){
    if (PIN_LIMIT1<0 && PIN_LIMIT2<0) return;
    s1.setSpeed(speed); s2.setSpeed(speed);
    mode1 = MODE_VEL; mode2 = MODE_VEL;
    while (true){
      if (PIN_LIMIT1>=0){ if (digitalRead(PIN_LIMIT1)==(LIMIT_ACTIVE_LOW?LOW:HIGH)) { s1.setSpeed(0); s1.setCurrentPosition(0); } }
      if (PIN_LIMIT2>=0){ if (digitalRead(PIN_LIMIT2)==(LIMIT_ACTIVE_LOW?LOW:HIGH)) { s2.setSpeed(0); s2.setCurrentPosition(0); } }
      if ((PIN_LIMIT1<0 || s1.speed()==0) && (PIN_LIMIT2<0 || s2.speed()==0)) break;
      s1.runSpeed(); s2.runSpeed();
      delay(2);
    }
  }

private:
  AccelStepper s1{AccelStepper::DRIVER, PIN_STEPPER1_STEP, PIN_STEPPER1_DIR};
  AccelStepper s2{AccelStepper::DRIVER, PIN_STEPPER2_STEP, PIN_STEPPER2_DIR};
  enum Mode { MODE_POS, MODE_VEL };
  Mode mode1{MODE_POS}, mode2{MODE_POS};
};

#endif // ROBOT_STEPPER_PAIR_H
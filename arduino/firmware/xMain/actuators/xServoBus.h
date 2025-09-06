#ifndef ROBOT_SERVO_BUS_H
#define ROBOT_SERVO_BUS_H

#include <Arduino.h>
#include <Servo.h>
#include <math.h>
#include "../xConfig.h"

class ServoBus {
public:
  void attachAll(const uint8_t pins[SERVO_COUNT_TOTAL], const uint8_t initDeg[SERVO_COUNT_TOTAL]){
    for (int i=0;i<SERVO_COUNT_TOTAL;i++){ pinMap[i]=pins[i]; servos[i].attach(pinMap[i]); servos[i].write(initDeg[i]); targets[i]=initDeg[i]; currents[i]=initDeg[i]; }
    lastUpdate=millis();
  }

  void setSpeed(float degPerSec){ speed=degPerSec; }

  void write(int index, float deg){ if (index<0||index>=SERVO_COUNT_TOTAL) return; targets[index]=constrain(deg,0,180); }

  void writePose(const uint8_t pose[SERVO_COUNT_TOTAL]){ for(int i=0;i<SERVO_COUNT_TOTAL;i++) targets[i]=pose[i]; }

  void update(){
    unsigned long now=millis();
    float dt = (now - lastUpdate) / 1000.0f;
    if (dt<=0) return; lastUpdate=now;
    float step = speed * dt; // deg to move this frame
    for (int i=0;i<SERVO_COUNT_TOTAL;i++){
      float cur = currents[i];
      float tgt = targets[i];
      float diff = tgt - cur;
      if (fabs(diff) < 0.5f) { currents[i]=tgt; servos[i].write((int)tgt); continue; }
      float delta = constrain(diff, -step, step);
      cur += delta; currents[i]=cur; servos[i].write((int)cur);
    }
  }

  float get(int index) const { return currents[index]; }
  bool attached(int index) const { if(index<0||index>=SERVO_COUNT_TOTAL) return false; return servos[index].attached(); }

  void detachAll(){ for(int i=0;i<SERVO_COUNT_TOTAL;i++){ if(servos[i].attached()) servos[i].detach(); } }
  void reattachAll(){ for(int i=0;i<SERVO_COUNT_TOTAL;i++){ if(!servos[i].attached()) servos[i].attach(pinMap[i]); servos[i].write((int)currents[i]); } }

  void detachOne(int index){ if(index<0||index>=SERVO_COUNT_TOTAL) return; if(servos[index].attached()) servos[index].detach(); }
  void reattachOne(int index){ if(index<0||index>=SERVO_COUNT_TOTAL) return; if(!servos[index].attached()){ servos[index].attach(pinMap[index]); servos[index].write((int)currents[index]); } }

  bool isSettled(float eps=0.7f) const {
    for (int i=0;i<SERVO_COUNT_TOTAL;i++){
      if (fabs(targets[i]-currents[i])>eps) return false;
    }
    return true;
  }

private:
  Servo servos[SERVO_COUNT_TOTAL];
  float currents[SERVO_COUNT_TOTAL] = {0};
  float targets[SERVO_COUNT_TOTAL]  = {0};
  float speed = SPEED_DEG_PER_S; // deg/s
  unsigned long lastUpdate=0;
  uint8_t pinMap[SERVO_COUNT_TOTAL] = {0};
};

#endif // ROBOT_SERVO_BUS_H
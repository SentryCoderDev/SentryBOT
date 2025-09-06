#include <Arduino.h>
#include "xConfig.h"
#include "xProtocol.h"
#include "xRobot.h"
#include <EEPROM.h>

Robot robot;
static unsigned long lastHeartbeatMs = 0;
static bool telemetryOn = false; static unsigned long telemetryInterval = 100; static unsigned long lastTelemetryMs = 0;

void setup(){
  SERIAL_IO.begin(ROBOT_SERIAL_BAUD);
  robot.begin();
  // Auto-load IMU offsets if present
  if (EEPROM.read(EEPROM_ADDR_MAGIC)==EEPROM_MAGIC){ float p,r; EEPROM.get(EEPROM_ADDR_IMU_OFF,p); EEPROM.get(EEPROM_ADDR_IMU_OFF+sizeof(float),r); robot.imu.setOffsets(p,r); }
  Protocol::sendOk("ready");
#if BOOT_CALIBRATION_PROMPT
  unsigned long t0 = millis();
  SERIAL_IO.println(F("{\"info\":\"press 'c' + Enter in 2s to calibrate\"}"));
  while (millis() - t0 < 2000) {
    if (SERIAL_IO.available()){
      String ln = SERIAL_IO.readStringUntil('\n'); ln.trim();
      if (ln.equalsIgnoreCase("c") || ln.indexOf("\"cmd\":\"calibrate\"")>=0){
        robot.calibrateNeutral();
        Protocol::sendOk("boot_calibrated");
      }
      break;
    }
    delay(5);
  }
#endif
}

static void handleJson(const String &line){
  // Very small manual parse for known keys to avoid heavy JSON libs on AVR
  if (line.indexOf("\"cmd\":\"hello\"")>=0){ Protocol::sendOk("hello"); return; }
  if (line.indexOf("\"cmd\":\"hb\"")>=0){ lastHeartbeatMs = millis(); Protocol::sendOk("hb"); return; }
  if (line.indexOf("\"cmd\":\"set_servo\"")>=0){
    int idx=-1; float deg=90;
    int p=line.indexOf("\"index\":"); if(p>=0){ idx=line.substring(p+8).toInt(); }
    p=line.indexOf("\"deg\":"); if(p>=0){ deg=line.substring(p+6).toFloat(); }
    if (idx>=0 && idx<SERVO_COUNT_TOTAL){ robot.writeServoLimited(idx,deg); Protocol::sendOk(); }
    else Protocol::sendErr("bad_index");
    return;
  }
  if (line.indexOf("\"cmd\":\"set_pose\"")>=0){
    // Expect pose as 8 ints [..]; optional duration_ms for time-based easing
    int lb=line.indexOf('['); int rb=line.indexOf(']');
    if (lb>0 && rb>lb){
      uint8_t pose[SERVO_COUNT_TOTAL]; int i=0; String nums=line.substring(lb+1,rb);
      nums += ','; int s=0; for (int k=0;k<nums.length() && i<SERVO_COUNT_TOTAL;k++) if(nums[k]==','){ pose[i++]= (uint8_t) constrain(nums.substring(s,k).toInt(),0,180); s=k+1; }
      // duration_ms
      long durMs = 0; int p=line.indexOf("\"duration_ms\":"); if(p>=0) durMs = line.substring(p+14).toInt();
      if (i==SERVO_COUNT_TOTAL){
        if (durMs>0){
          // Compute max delta and set speed so that worst joint arrives in durMs
          float maxDelta = 0.0f;
          for (int j=0;j<SERVO_COUNT_TOTAL;j++){
            float d = fabs((float)pose[j] - robot.servos.get(j)); if (d>maxDelta) maxDelta=d;
          }
          if (maxDelta>0){ float speed = (maxDelta / (durMs/1000.0f)); robot.servos.setSpeed(speed); }
        }
        robot.writePoseLimited(pose); Protocol::sendOk(); return;
      }
    }
    Protocol::sendErr("bad_pose"); return;
  }
  if (line.indexOf("\"cmd\":\"leg_ik\"")>=0){
    float x=120; Side side=LEFT;
    int p=line.indexOf("\"x\":"); if(p>=0){ x=line.substring(p+4).toFloat(); }
    p=line.indexOf("\"side\":\"R\""); if(p>=0){ side=RIGHT; }
    if (robot.setLegByIK(side,x)) Protocol::sendOk(); else Protocol::sendErr("ik_fail");
    return;
  }
  if (line.indexOf("\"cmd\":\"stepper\"")>=0){
    // modes: pos, vel
    bool vel = line.indexOf("\"mode\":\"vel\"")>=0;
    int id = (line.indexOf("\"id\":1")>=0)?1:0;
    long val=0; int p=line.indexOf("\"value\":"); if(p>=0){ val=line.substring(p+8).toInt(); }
    if (vel){ robot.steppers.setSpeedOne(id, (float)val); }
    else { robot.steppers.moveToOne(id, val); }
    Protocol::sendOk(); return;
  }
  if (line.indexOf("\"cmd\":\"home\"")>=0){
    robot.steppers.homeBoth(); Protocol::sendOk("homed"); return;
  }
  if (line.indexOf("\"cmd\":\"zero_now\"")>=0){ robot.steppers.zeroNow(); Protocol::sendOk("zeroed_now"); return; }
  if (line.indexOf("\"cmd\":\"zero_set\"")>=0){
    long p1=0,p2=0; int p=line.indexOf("\"p1\":"); if(p>=0) p1=line.substring(p+5).toInt(); p=line.indexOf("\"p2\":"); if(p>=0) p2=line.substring(p+5).toInt();
    robot.steppers.zeroSet(p1,p2); Protocol::sendOk("zero_set"); return;
  }
  if (line.indexOf("\"cmd\":\"stepper_cfg\"")>=0){
    int p=line.indexOf("\"maxSpeed\":"); if(p>=0){ float v=line.substring(p+11).toFloat(); robot.steppers.setMaxSpeed(v); }
    p=line.indexOf("\"accel\":"); if(p>=0){ float a=line.substring(p+8).toFloat(); robot.steppers.setAcceleration(a); }
    Protocol::sendOk("stepper_cfg"); return;
  }
  if (line.indexOf("\"cmd\":\"pid\"")>=0){
    bool enable = (line.indexOf("\"enable\":true")>=0);
    robot.setBalance(enable);
    Protocol::sendOk(enable?"pid_on":"pid_off");
    return;
  }
  if (line.indexOf("\"cmd\":\"calibrate\"")>=0){
    robot.calibrateNeutral();
    Protocol::sendOk("calibrated");
    return;
  }
  if (line.indexOf("\"cmd\":\"stand\"")>=0){ robot.setModeStand(); Protocol::sendOk("stand"); return; }
  if (line.indexOf("\"cmd\":\"sit\"")>=0){ robot.setModeSit(); Protocol::sendOk("sit"); return; }
  if (line.indexOf("\"cmd\":\"imu_read\"")>=0){ robot.imu.read();
    String msg = String("pitch=")+robot.imu.getPitch()+",roll="+robot.imu.getRoll(); Protocol::sendOk(msg); return; }
  if (line.indexOf("\"cmd\":\"imu_cal\"")>=0){ robot.imu.calibrateLevel(); Protocol::sendOk("imu_calibrated"); return; }
  if (line.indexOf("\"cmd\":\"eeprom_save\"")>=0){
    float p,r; robot.imu.getOffsets(p,r);
    EEPROM.update(EEPROM_ADDR_MAGIC, EEPROM_MAGIC);
    EEPROM.put(EEPROM_ADDR_IMU_OFF, p); EEPROM.put(EEPROM_ADDR_IMU_OFF+sizeof(float), r);
    Protocol::sendOk("saved"); return;
  }
  if (line.indexOf("\"cmd\":\"eeprom_load\"")>=0){
    if (EEPROM.read(EEPROM_ADDR_MAGIC)==EEPROM_MAGIC){ float p,r; EEPROM.get(EEPROM_ADDR_IMU_OFF,p); EEPROM.get(EEPROM_ADDR_IMU_OFF+sizeof(float),r); robot.imu.setOffsets(p,r); Protocol::sendOk("loaded"); }
    else Protocol::sendErr("no_data");
    return;
  }
  if (line.equalsIgnoreCase("cal")) { robot.calibrateNeutral(); Protocol::sendOk("calibrated"); return; }
  if (line.indexOf("\"cmd\":\"policy\"")>=0){
    // MuJoCo/policy hook: optional pose[8] and steppers[2] arrays
    int lb=line.indexOf("[", line.indexOf("\"pose\""));
    int rb=line.indexOf("]", lb+1);
    if (lb>0 && rb>lb){ uint8_t pose[SERVO_COUNT_TOTAL]; int i=0; String nums=line.substring(lb+1,rb); nums+=','; int s=0; for(int k=0;k<nums.length() && i<SERVO_COUNT_TOTAL;k++) if(nums[k]==','){ pose[i++]=(uint8_t)constrain(nums.substring(s,k).toInt(),0,180); s=k+1; }
      if (i==SERVO_COUNT_TOTAL) robot.servos.writePose(pose);
    }
    int ls=line.indexOf("[", line.indexOf("\"steppers\""));
    int rs=line.indexOf("]", ls+1);
    if (ls>0 && rs>ls){ String nums=line.substring(ls+1,rs); nums+=','; long v[2]={0,0}; int i=0; int s=0; for(int k=0;k<nums.length() && i<2;k++) if(nums[k]==','){ v[i++]=nums.substring(s,k).toInt(); s=k+1; }
      if (i>=1) robot.steppers.setSpeedOne(0, (float)v[0]);
      if (i>=2) robot.steppers.setSpeedOne(1, (float)v[1]);
    }
    Protocol::sendOk("policy_applied"); return;
  }
  if (line.indexOf("\"cmd\":\"track\"")>=0){
    // Tracking skeleton: client (OpenCV) sends desired head angles and optional steer velocity
    // {"cmd":"track","head_tilt":x,"head_pan":y,"drive":v}
    float tilt=90, pan=90; long drive=0;
    int p=line.indexOf("\"head_tilt\":"); if(p>=0) tilt=line.substring(p+12).toFloat();
    p=line.indexOf("\"head_pan\":"); if(p>=0) pan=line.substring(p+11).toFloat();
    p=line.indexOf("\"drive\":"); if(p>=0) drive=line.substring(p+9).toInt();
    robot.head(tilt, pan);
    // In sit/skate, use drive as forward velocity
    if (drive!=0) { robot.steppers.setSpeedOne(0, drive); robot.steppers.setSpeedOne(1, drive); }
    Protocol::sendOk("track_ack"); return;
  }
  if (line.indexOf("\"cmd\":\"get_state\"")>=0){
    robot.imu.read();
    String out = "{""ok"":true,""mode"":"; out += (robot.getMode()==MODE_STAND?"\"stand\"":"\"sit\"");
    out += ",""pid"":"; out += (robot.isBalanceEnabled()?"true":"false");
    out += ",""pitch"":"; out += robot.imu.getPitch();
    out += ",""roll"":"; out += robot.imu.getRoll();
    out += ",""pose"": [";
    for (int i=0;i<SERVO_COUNT_TOTAL;i++){ if(i) out += ","; out += (int)robot.servos.get(i); }
    out += "],""stepper_pos"": ["; out += robot.steppers.pos1(); out += ","; out += robot.steppers.pos2(); out += "]}";
    SERIAL_IO.println(out); return;
  }
  if (line.indexOf("\"cmd\":\"estop\"")>=0){ robot.estop(); Protocol::sendOk("estopped"); return; }
  if (line.indexOf("\"cmd\":\"telemetry_start\"")>=0){
    int p=line.indexOf("\"interval_ms\":"); if(p>=0){ unsigned long v=line.substring(p+15).toInt(); telemetryInterval = max((unsigned long)TELEMETRY_MIN_INTERVAL_MS, v); }
    telemetryOn = true; lastTelemetryMs = millis(); Protocol::sendOk("telemetry_on"); return;
  }
  if (line.indexOf("\"cmd\":\"telemetry_stop\"")>=0){ telemetryOn=false; Protocol::sendOk("telemetry_off"); return; }
  if (line.indexOf("\"cmd\":\"tune\"")>=0){
    int p=line.indexOf("\"servo_speed\":"); if(p>=0){ float v=line.substring(p+14).toFloat(); robot.setServoSpeed(v); }
    float kpP,kiP,kdP,kpR,kiR,kdR; robot.getPidGains(kpP,kiP,kdP,kpR,kiR,kdR);
    p=line.indexOf("\"kpP\":"); if(p>=0) kpP=line.substring(p+6).toFloat();
    p=line.indexOf("\"kiP\":"); if(p>=0) kiP=line.substring(p+6).toFloat();
    p=line.indexOf("\"kdP\":"); if(p>=0) kdP=line.substring(p+6).toFloat();
    p=line.indexOf("\"kpR\":"); if(p>=0) kpR=line.substring(p+6).toFloat();
    p=line.indexOf("\"kiR\":"); if(p>=0) kiR=line.substring(p+6).toFloat();
    p=line.indexOf("\"kdR\":"); if(p>=0) kdR=line.substring(p+6).toFloat();
    robot.setPidGains(kpP,kiP,kdP,kpR,kiR,kdR);
    float skp, ski, skd; robot.getSkateGains(skp,ski,skd); float smax=robot.getSkateSpeedLimit();
    int sp=line.indexOf("\"skate\""); if(sp>=0){
      int q=line.indexOf("\"kp\":", sp); if(q>0) skp=line.substring(q+5).toFloat();
      q=line.indexOf("\"ki\":", sp); if(q>0) ski=line.substring(q+5).toFloat();
      q=line.indexOf("\"kd\":", sp); if(q>0) skd=line.substring(q+5).toFloat();
      q=line.indexOf("\"max\":", sp); if(q>0) smax=line.substring(q+6).toFloat();
    }
    robot.setSkateGains(skp,ski,skd); robot.setSkateSpeedLimit(smax);
    Protocol::sendOk("tuned"); return;
  }
  Protocol::sendErr("unknown_cmd");
}

void loop(){
  String line; if (Protocol::readLine(SERIAL_IO, line)) handleJson(line);
  robot.update();
  // Heartbeat timeout safety
  if (HEARTBEAT_TIMEOUT_MS>0 && (millis() - lastHeartbeatMs > HEARTBEAT_TIMEOUT_MS)){
    robot.estop();
  }
  // Telemetry periodic output
  if (telemetryOn && millis() - lastTelemetryMs >= telemetryInterval){
    lastTelemetryMs = millis(); robot.imu.read();
    String out = "{""ok"":true,""telemetry"":true,""pitch"":"; out += robot.imu.getPitch();
    out += ",""roll"":"; out += robot.imu.getRoll();
    out += ",""pose"": ["; for (int i=0;i<SERVO_COUNT_TOTAL;i++){ if(i) out += ","; out += (int)robot.servos.get(i);} out += "],""stepper_pos"": ["; out += robot.steppers.pos1(); out += ","; out += robot.steppers.pos2(); out += "]}";
    SERIAL_IO.println(out);
  }
}

#include "xNemaController.h"
#include "../xConfig.h"
#include <math.h>

// Use global Robot instance for steppers and the global radio instance
#include "../xRobot.h"
extern Robot robot;
extern EbyteRadio g_ebyteRadio;

NemaController g_nema(&robot.steppers, &g_ebyteRadio);

NemaController::NemaController(StepperPair* pair, EbyteRadio* radio): steppers(pair), radio(radio) {}

void NemaController::begin(){
  if (steppers) steppers->begin();
}

// Map joystick X,Y (-127..127) to left/right stepper speeds (steps/s)
void NemaController::applyJoystick(int8_t x, int8_t y){
  // Normalize to -1..1
  float nx = (float)x / 127.0f;
  float ny = (float)y / 127.0f;
  // Tank mixing: left = forward + turn, right = forward - turn
  float left = ny + nx;
  float right = ny - nx;
  // Clamp
  left = constrain(left, -1.0f, 1.0f);
  right = constrain(right, -1.0f, 1.0f);
  float lspd = left * maxSpeedStepsPerSec;
  float rspd = right * maxSpeedStepsPerSec;
  // Apply toggles
  if (!leftEnabled) lspd = 0.0f;
  if (!rightEnabled) rspd = 0.0f;
  steppers->setSpeed(lspd, rspd);
}

void NemaController::update(){
  // If new packet arrived, process it
  if (radio && radio->newPacket){
    // Copy and consume
    MasterPacket1 pkt = radio->lastPkt;
    radio->newPacket = false;

    // Determine toggles based on button bits (BTN_MOTOR1 = right, BTN_MOTOR2 = left)
    // Button bits documented in xMaster/src/SharedVariables.h
    const uint16_t BTN_MOTOR1 = (1<<9);
    const uint16_t BTN_MOTOR2 = (1<<10);
    rightEnabled = (pkt.Buttons & BTN_MOTOR1);
    leftEnabled  = (pkt.Buttons & BTN_MOTOR2);

    // Apply joystick mapping
    applyJoystick(pkt.Rstick_X, pkt.Rstick_Y);

    // Display incoming info
    String s = String("SRC:") + radio->lastSource + " ";
    s += String("X=") + pkt.Rstick_X + ",Y=" + pkt.Rstick_Y;
    SERIAL_IO.println(s);
  }

  // Always run stepper update so AccelStepper processes ramping
  if (steppers) steppers->update();
}

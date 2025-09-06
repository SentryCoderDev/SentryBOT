#ifndef ROBOT_KINEMATICS_H
#define ROBOT_KINEMATICS_H

#include <Arduino.h>
#include "xConfig.h"

struct IkSolution { float hip, knee, ankle; bool valid; };

class LegIK2D {
public:
  void setup(float thigh, float shin) { A=thigh; B=shin; }
  void setOffsets(float hipDeg, float kneeDeg, float ankleDeg){ offHip=hipDeg; offKnee=kneeDeg; offAnkle=ankleDeg; }

  IkSolution solve(float x /*hip-ankle distance*/, float /*y not used*/=0) {
    IkSolution s{}; s.valid=false;
    float a=A, b=B, c=x;
    if (c <= 1) c = 1; // avoid div by zero
    float cosHip = (sq(a)+sq(c)-sq(b)) / (2*a*c);
    if (cosHip < -1 || cosHip > 1) return s;
    float hipRad = acos(cosHip);
    float kneeRad = PI - (hipRad*2);
    float ankleRad = PI - kneeRad - hipRad;
    if (isnan(hipRad)||isnan(kneeRad)||isnan(ankleRad)) return s;

    float hip = deg(PI - (hipRad + rad(offHip)));
    float knee = deg(kneeRad - rad(offKnee));
    float ankle = deg(ankleRad + rad(offAnkle));

    if (!within(hip, HIP_MIN, HIP_MAX) || !within(knee, KNEE_MIN, KNEE_MAX) || !within(ankle, ANKLE_MIN, ANKLE_MAX)) return s;
    s.hip=hip; s.knee=knee; s.ankle=ankle; s.valid=true; return s;
  }

  static void mirror(const IkSolution &L, IkSolution &R){ R.hip=180-L.hip; R.knee=180-L.knee; R.ankle=180-L.ankle; R.valid=L.valid; }

private:
  static inline float rad(float d){ return d*PI/180.0f; }
  static inline float deg(float r){ return r*180.0f/PI; }
  static inline bool within(float v, float mn, float mx){ return v>=mn && v<=mx; }
  float A=THIGH_LEN, B=SHIN_LEN; 
  float offHip=OFFS_HIP, offKnee=OFFS_KNEE, offAnkle=OFFS_ANKLE;
};

#endif // ROBOT_KINEMATICS_H
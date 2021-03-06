#include <Arduino.h>
#include <Servo.h>
#include <Metro.h>

/* constants (pins, etc) */
#define STEERING_AUTOPILOT  6
#define THROTTLE_AUTOPILOT  5
#define RCVR_CH3            4
#define RCVR_CH2            3
#define RCVR_CH1            2
#define STEERING_OUTPUT     8
#define THROTTLE_OUTPUT     7
#define STEERING_IN_MIN     1000
#define STEERING_OUT_MIN    1000
#define STEERING_IN_MAX     2000
#define STERRING_OUT_MAX    2000
#define STEERING_DEFAULT    1500
#define THROTTLE_IN_MIN     1490
#define THROTTLE_IN_MAX     1990
#define THROTTLE_OUT_MIN    1000
#define THROTTLE_OUT_MAX    2000
#define THROTTLE_DEFAULT    900
#define PWM_TIMEOUT         100000
#define T_CAP               2000
#define DISABLE_CALIB

/* servos for writing PWM to actuators */
Servo steeringOutput;
Servo throttleOutput;

/* variables for blinking LED (debug) */
Metro blinkMetro = Metro(333);
bool ledState = 0;

enum state {
  autopilot,
  manual
} boat_state;

unsigned long sprev;
unsigned long tprev;
unsigned long sout;
unsigned long tout;
unsigned long autoStrIn;
unsigned long autoThrIn;

unsigned long rreadThr;
void risingThr();
void fallingThr()
{
  autoThrIn = micros() - rreadThr;
  attachInterrupt(digitalPinToInterrupt(THROTTLE_AUTOPILOT), risingThr, RISING);
}
void risingThr()
{
  rreadThr = micros();
  attachInterrupt(digitalPinToInterrupt(THROTTLE_AUTOPILOT), fallingThr, FALLING);
}

unsigned long rreadStr;
void risingStr();
void fallingStr()
{
  autoStrIn = micros() - rreadStr;
  attachInterrupt(digitalPinToInterrupt(STEERING_AUTOPILOT), risingStr, RISING);
}
void risingStr()
{
  rreadStr = micros();
  attachInterrupt(digitalPinToInterrupt(STEERING_AUTOPILOT), fallingStr, FALLING);
}

bool acceptable(unsigned long pwm) 
{
  return pwm >= 900 & pwm <= 2000;
}

void setup() {
  /* setup input pins */
  
  pinMode(RCVR_CH1, INPUT);
  pinMode(RCVR_CH2, INPUT);
  pinMode(RCVR_CH3, INPUT);
  pinMode(STEERING_AUTOPILOT, INPUT);
  pinMode(THROTTLE_AUTOPILOT, INPUT); 
  attachInterrupt(digitalPinToInterrupt(THROTTLE_AUTOPILOT), risingThr, RISING);
  attachInterrupt(digitalPinToInterrupt(STEERING_AUTOPILOT), risingStr, RISING);

  /* attach output servos */
  steeringOutput.attach(STEERING_OUTPUT);
  throttleOutput.attach(THROTTLE_OUTPUT);
  steeringOutput.writeMicroseconds(STEERING_DEFAULT);
  throttleOutput.writeMicroseconds(THROTTLE_DEFAULT);

  /* configure LED for debug */
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, ledState);

  boat_state = manual;
  sout = STEERING_DEFAULT;
  tout = THROTTLE_DEFAULT;
  sprev = sout;
  tprev = tout;
  steeringOutput.writeMicroseconds((int)sout);
  throttleOutput.writeMicroseconds((int)tout);

  Serial.begin(9600);
  #ifdef DISABLE_CALIB 
    delay(3000);
  #endif
}



void loop() {

  unsigned long ch3in = pulseIn(RCVR_CH3, HIGH, PWM_TIMEOUT);
  unsigned long ch1in = pulseIn(RCVR_CH1, HIGH, PWM_TIMEOUT);
  unsigned long ch2in = pulseIn(RCVR_CH2, HIGH, PWM_TIMEOUT);

  if (ch3in > 900 && ch3in < 1100)     /* ch3 switch low */
    boat_state = manual;
  else if (ch3in > 1900 && ch3in < 2100)  /* ch3 switch high */
    boat_state = autopilot;

  if (boat_state == manual) {
    sout = acceptable(ch1in) ? ch1in : sprev;
    tout = acceptable(ch2in) ? ch2in : tprev;
  } else if (boat_state == autopilot) {
    sout = acceptable(autoStrIn) ? autoStrIn : sprev;
    tout = acceptable(autoThrIn) ? autoThrIn : tprev;
  }

  tout = tout > T_CAP ? T_CAP : tout; /* limit throttle to some arbitrary number */
  tprev = tout;
  sprev = sout;

  steeringOutput.writeMicroseconds((int)sout);
  throttleOutput.writeMicroseconds((int)tout);

  Serial.print(sout); Serial.print(" "); Serial.print(tout); Serial.print(" | ");
  Serial.print(autoStrIn); Serial.print(" "); Serial.print(autoThrIn); Serial.print(" | ");
  Serial.print(ch1in); Serial.print(" "); Serial.print(ch2in); Serial.print(" "); Serial.print(ch3in); 
  Serial.print(" | "); Serial.print(acceptable(ch1in)); Serial.print(" "); Serial.print(acceptable(ch2in));
  
  Serial.println();

  if (blinkMetro.check()) {   /* LED blink to check loop still running */ 
    ledState = !ledState;
    digitalWrite(LED_BUILTIN, ledState);
  }
}
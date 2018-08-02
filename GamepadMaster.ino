#include <Joystick.h>
/* Master: 8
 *  Dpad: 4 (not buttons)
 *  buttons 0-3
 *  select, 8
 *  start, 9
 *  J1, 10
 *  J2, 11
 */
#define MASTER_BUTTON_COUNT 8
/* Slave: 8
 *  Shoulders 4-7
 *  4 extra non-descript buttons 12, 13, 14, 15
 */
#define SLAVE_BUTTON_COUNT 8
/* Number of HAT switches */
#define HAT_COUNT 1
/* Number of dpad buttons, 4 for each switch */
#define DPAD_COUNT (4 * HAT_COUNT)

Joystick_ Joystick(JOYSTICK_DEFAULT_REPORT_ID, JOYSTICK_TYPE_GAMEPAD,
  (MASTER_BUTTON_COUNT + SLAVE_BUTTON_COUNT), HAT_COUNT,
  true, true, false,     // X and Y, but no Z Axis
  true, true, false,     // Rx and Ry, but no Rz
  false, false,          // No rudder or throttle
  false, false, false);  // No accelerator, brake, or steering

/* Digital button pin numbers to read from, not including HAT. */
int _pins[] = {2, 3, 4, 5, 6, 7, 8, 9};
/* Digital HAT switch pins */
int _hat_pins[] = {10, 14, 15, 16};
/* Master buttons: 0, 1, 2, 3, select, start, J1, J2 */
int _master_buttons[] = {0, 1, 2, 3, 8, 9, 10, 11};
/* Slave buttons: L1, R1, L2, R2, +4 extras */
int _slave_buttons[] = {4, 5, 6, 7, 12, 13, 14, 15};

/* Current pin state, always same array length as _pins. */
int _master_states[MASTER_BUTTON_COUNT];
int _hat_states[DPAD_COUNT];
int _slave_states[SLAVE_BUTTON_COUNT];
/* axis state: x, y, rx, ry */
int _xAxis = 0;
int _yAxis = 0;
int _rxAxis = 0;
int _ryAxis = 0;

/* Analog joystick input pins */
#define X_PIN A0
#define Y_PIN A1
#define RX_PIN A2
#define RY_PIN A3

/* Adjust analog for parasitic resistance */
#define AXIS_MIN 42
#define AXIS_MAX 1019

void setup()
{
  size_t i;
  // Initialize Button Pins
  for (i = 0; i < MASTER_BUTTON_COUNT; i++) {
    pinMode(_pins[i], INPUT);
  }
  for (i = 0; i < DPAD_COUNT; i++) {
    pinMode(_hat_pins[i], INPUT);
  }

  // Initialize Joystick Library
  Joystick.begin();
  Joystick.setXAxisRange(AXIS_MIN, AXIS_MAX);
  Joystick.setYAxisRange(AXIS_MIN, AXIS_MAX);
  Joystick.setRxAxisRange(AXIS_MIN, AXIS_MAX);
  Joystick.setRyAxisRange(AXIS_MIN, AXIS_MAX);
}

int _slaveReport;
void loop()
{
  size_t i;
  int curBit = 1;
  int state;
  bool hatChanged = false;
  /* Read serial from slave */
  if (Serial.available() > 0) {
    Serial.readBytes((char *)&_slaveReport, sizeof(_slaveReport));
    for (i = 0; i < SLAVE_BUTTON_COUNT; i++) {
      if ((_slaveReport & curBit)) {
        if (!_slave_states[i]) {
          _slave_states[i] = 1;
          Joystick.setButton(_slave_buttons[i], 1);
        }
      } else if (_slave_states[i]) {
        _slave_states[i] = 0;
        Joystick.setButton(_slave_buttons[i], 0);
      }
      curBit <<= 1;
    }
  }

  /* Joysticks */
  state = analogRead(X_PIN);
  if (state != _xAxis) {
    _xAxis = state;
    Joystick.setXAxis(state);
  }
  state = analogRead(Y_PIN);
  if (state != _yAxis) {
    _yAxis = state;
    Joystick.setYAxis(state);
  }
  state = analogRead(RX_PIN);
  if (state != _rxAxis) {
    _rxAxis = state;
    Joystick.setRxAxis(state);
  }
  state = analogRead(RY_PIN);
  if (state != _ryAxis) {
    _ryAxis = state;
    Joystick.setRyAxis(state);
  }

  /* HAT */
  for (i = 0; i < DPAD_COUNT; i++) {
    state = digitalRead(_hat_pins[i]);
    if (state != _hat_states[i]) {
      if (!hatChanged)
        hatChanged = true;
      _hat_states[i] = state;
    }
  }
  /* HAT: -1 is nothing pressed, otherwise set in degrees from 0-360 */
  /* TODO: Algorithm */
  if (hatChanged) {
    if (_hat_states[0]) {
      if (_hat_states[3]) {
        Joystick.setHatSwitch(0, 315);
      } else if (_hat_states[1]) {
        Joystick.setHatSwitch(0, 45);
      } else {
        Joystick.setHatSwitch(0, 0);
      }
    } else if (_hat_states[2]) {
      if (_hat_states[3]) {
        Joystick.setHatSwitch(0, 225);
      } else if (_hat_states[1]) {
        Joystick.setHatSwitch(0, 135);
      } else {
        Joystick.setHatSwitch(0, 180);
      }
    } else if (_hat_states[3]) {
      Joystick.setHatSwitch(0, 270);
    } else if (_hat_states[1]) {
      Joystick.setHatSwitch(0, 90);
    } else {
      Joystick.setHatSwitch(0, -1);
    }
  }

  /* Finally, regular buttons */
  for (i = 0; i < MASTER_BUTTON_COUNT; i++) {
    state = digitalRead(_pins[i]);
    if (state != _master_states[i]) {
      _master_states[i] = state;
      Joystick.setButton(_master_buttons[i], state);
    }
  }

  delay(10);
}


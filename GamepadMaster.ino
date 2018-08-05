#include <Joystick.h>
/* Y axis Joysticks have reversed analog signal directions */
//#define Y_AXIS_FLIPPED

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
#ifdef Y_AXIS_FLIPPED
int _mid_y_axis;
int _mid_ry_axis;
#endif

void setup()
{
  size_t i;
  /* Read slave state from Serial1. */
  Serial1.begin(9600);
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
#ifdef Y_AXIS_FLIPPED
  /* Remember center point for adjustments */
  _mid_y_axis = analogRead(Y_PIN);
  _mid_ry_axis = analogRead(RY_PIN);
#endif
}

void loop()
{
  size_t i;
  int curBit = 1;
  int state = 0, slaveState = 0;
  bool hatChanged = false;
  /* Read serial from slave */
  if (Serial1.available() > 0) {
    Serial1.readBytes((char *)&slaveState, sizeof(slaveState));
    for (i = 0; i < SLAVE_BUTTON_COUNT; i++) {
      if ((slaveState & curBit)) {
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
    /* Reset to reuse variables. */
    curBit = 1;
    slaveState = 0;
  }

  /* Joysticks */
  state = analogRead(X_PIN);
  if (state != _xAxis) {
    _xAxis = state;
    Joystick.setXAxis(state);
  }
  state = analogRead(Y_PIN);
  /* Y axis joystick may have backwards resistance. */
#ifdef Y_AXIS_FLIPPED
  if (state != _mid_y_axis)
    state = state > _mid_y_axis ? _mid_y_axis - (state - _mid_y_axis) : _mid_y_axis + (_mid_y_axis - state);
#endif
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
  /* Y axis joystick may have backwards resistance. */
#ifdef Y_AXIS_FLIPPED
  if (state != _mid_ry_axis)
    state = state > _mid_ry_axis ? _mid_ry_axis - (state - _mid_ry_axis) : _mid_ry_axis + (_mid_ry_axis - state);
#endif
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
    /* Reuse slave state as HAT position */
    if (state)
      slaveState |= curBit;
    curBit <<= 1;
  }
  /* HAT: -1 is nothing pressed, otherwise set in degrees from 0-360 */
  if (hatChanged) {
    switch (slaveState) {
      /* DPAD 1: 0 degrees */
      case 0b0001:
        Joystick.setHatSwitch(0, 0);
        break;
      /* DPAD 1+2: 45 degrees */
      case 0b0011:
        Joystick.setHatSwitch(0, 45);
        break;
      /* DPAD 2: 90 degrees */
      case 0b0010:
        Joystick.setHatSwitch(0, 90);
        break;
      /* DPAD 2+3: 135 degrees */
      case 0b0110:
        Joystick.setHatSwitch(0, 135);
        break;
      /* DPAD 3: 180 degrees */
      case 0b0100:
        Joystick.setHatSwitch(0, 180);
        break;
      /* DPAD 3+4: 225 degrees */
      case 0b1100:
        Joystick.setHatSwitch(0, 225);
        break;
      /* DPAD 4: 270 degrees */
      case 0b1000:
        Joystick.setHatSwitch(0, 270);
        break;
      /* DPAD 4+1: 315 degrees */
      case 0b1001:
        Joystick.setHatSwitch(0, 315);
        break;
      /* No HATS pressed, or invalid combination */
      default:
        Joystick.setHatSwitch(0, -1);
        break;
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


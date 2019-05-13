/**
 * @file SlowServoPWMExpander.h
 * 
 * @brief An Arduino library for SlowServo behind PWM expander (PCA9685). 
 * 
 * @author Etienne Hamelin (etienne.hamelin@gmail.com ; www.github.com/etiennehamelin)
 * 
 * @licence 
 * You may freely use, compile, link, copy, adapt, distribute, this software,
 * as long as you keep this author & license header.
 * Happy hacking!
 * 
 */


#include <Wire.h>                     // I2C communication with PCA9685 I2C hardware PWM controller
#include <Adafruit_PWMServoDriver.h>  // PCA9685 I2C hardware PWM controller driver

// maximum allowed speed (degrees per second)
#define MAX_DPS_DEFAULT 600

#ifdef SERVO_EXTENDED_RANGE
// Extended servo range: input values from -120° to 120°, pulse lengths from 600us to 2400us
#define SERVO_MIN (-120)
#define SERVO_MAX (120)
#define MIN_PULSE (122)
#define MAX_PULSE (491)
#else
// Standard servo range: input values from -90° to 90°, pulse lengths from 700us to 2300us
#define SERVO_MIN (-90)
#define SERVO_MAX (90)
#define MIN_PULSE (143)
#define MAX_PULSE (471)
#endif
#define MED_PULSE (MIN_PULSE + MAX_PULSE)/2

class SlowServo {
private:
  int16_t _curr_pulse, _last_pulse, _start_pulse, _end_pulse;
  int16_t _min_pulse, _max_pulse;  
  long int _start_ms, _end_ms;
  int _num;
  int16_t _max_dps;
  int8_t _inverted;
  
  static bool _pwm_started;
  static Adafruit_PWMServoDriver _pwm;

public:
  /**
   * \brief Constructor
   * 
   * \param num   The servo identifier
   * \param max_dps   maximum allowed speed, in degree per second (dps)
   * \param min_deg   soft limit for min position (in degrees) 
   * \param max_deg   soft limit for max position (in degrees)
   * \param inverted  whether input position shall be interpreted negative
   */
  SlowServo(int num = 0, int16_t max_dps = MAX_DPS_DEFAULT, int16_t min_deg = -90, int16_t max_deg = 90, int8_t inverted = false) :
    _num(num), 
    _max_dps(max_dps),
    _min_pulse(deg2pulse(min_deg)),
    _max_pulse(deg2pulse(max_deg)),
    _last_pulse(-1),
    _curr_pulse(MED_PULSE), 
    _start_pulse(MED_PULSE),
    _end_pulse(MED_PULSE),
    _inverted(inverted)
  {
    // if necessary, initialize PWM expander IC
    if (!_pwm_started) {
      _pwm.begin();
      _pwm.setPWMFreq(50);
      _pwm_started = true;
    }
  }

  /** 
   * Utilities
   */
  int16_t deg2pulse(int16_t deg) {
    return map(deg, -SERVO_MIN, SERVO_MAX, MIN_PULSE, MAX_PULSE);
  }

  int16_t pulse2deg(int16_t pulse) {
    return map(pulse, MIN_PULSE, MAX_PULSE, -SERVO_MIN, SERVO_MAX);
  }

  int16_t get_min_deg() {
    return pulse2deg(_min_pulse);
  }

  int16_t get_max_deg() {
    return pulse2deg(_max_pulse);
  }


  /**
   * Main API
   */

  /**
   * @brief Set servo to move towards given position, at maximum allowed speed
   * @param degree: servo set position
   */
  void write(int16_t degree) {
    write_speed(degree, _max_dps);
  }

 /**
   * @brief Set servo to move towards given position in given delay
   * @param degree: servo set position
   * @param delay_ms: how much time the servo should take to arrive at set pos
   */
  void write_delay(int16_t degree, long int delay_ms = 0) {
    delay_ms = constrain(delay_ms, 0, 60000); // make sure delay is between 0 ms and < 1 minute
    write_pulse(deg2pulse(degree), delay_ms);
  }

 /**
   * @brief Set servo to move towards given position at given speed 
   * @param degree: servo set position
   * @param speed_dps: angular velocity in degree per second
   */
  void write_speed(int16_t degree, int16_t speed_dps = MAX_DPS_DEFAULT) {
    degree = constrain(degree, -SERVO_MIN, SERVO_MAX);
    speed_dps = constrain(speed_dps, 1, MAX_DPS_DEFAULT); /* arbitrary max 360°/s */
    int16_t pulse = deg2pulse(degree);
    // delay_ms = degrees / deg_per_ms = 1000 * deg/deg_per_s;
    long int delay_ms = abs(degree - read()) * 1000 / speed_dps;
    write_pulse(pulse, delay_ms);
  }

  void write_pulse(int16_t pulse, long int delay_ms) {
    _start_pulse = _curr_pulse;
    _end_pulse = constrain(pulse, MIN_PULSE, MAX_PULSE);
    _start_ms = millis();
    _end_ms = _start_ms + delay_ms;
  }

  int16_t read_pulse() {
    return _curr_pulse;
  }
  
  int16_t read() {
    return map(read_pulse(), 123, 492, -90, 90);
  }

  // Make sure the update() function is called frequently, e.g. every few milliseconds
  void update() {
    long int now = millis();
    if ((signed)(now - _end_ms) >= 0) { // controlled move is finished
      _curr_pulse = _end_pulse;
      _end_ms = now - 1; // keep updating so that comparison is negative 
    } else { // still moving
      _curr_pulse = map(now, _start_ms, _end_ms, _start_pulse, _end_pulse); // linear interpolation during movement
      _curr_pulse = constrain(_curr_pulse, _min_pulse, _max_pulse); // enforce soft limits
    }
    
    // Send update to PWM Extender only when necessary
    if (_curr_pulse != _last_pulse) {
      int16_t offset = 0; //_num << 8; // each servo on PWM extender has a specific offset, so that high pulses are equally distributed in 20ms period
      _pwm.setPWM(_num, offset, 0x0fff & (offset + _curr_pulse));
      _last_pulse = _curr_pulse;
    }
  }
};

// Initialization of static members
bool SlowServo::_pwm_started = false;
Adafruit_PWMServoDriver SlowServo::_pwm = Adafruit_PWMServoDriver();

#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

// called this way, it uses the default address 0x40
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

class SlowServo {
public:
  int16_t _curr_pulse, _last_pulse, _start_pulse, _end_pulse;
  int16_t _min_pulse, _max_pulse;  
  long int _start_ms, _end_ms;
  int _num;
  int16_t _max_dps; // maximum speed in degrees per second
  static bool _pwm_started;

  SlowServo() {
    SlowServo(0, 600);
  }
  
  SlowServo(int num) {
    SlowServo(num, 600);
  }

  SlowServo(int num, int16_t max_dps) {
    if (!_pwm_started) {
      pwm.begin();
      pwm.setPWMFreq(50);      
      _pwm_started = true;
    }
    setup(num, max_dps);
  }

  void setup(int num, int16_t max_dps) {
    _num = num;
    _last_pulse = -1; // force send in first call to update()
    _curr_pulse = _start_pulse = _end_pulse = 308;
    _min_pulse = deg2pulse(-90); // default limit values
    _max_pulse = deg2pulse(90);
    _max_dps = max_dps;    
  }

  int16_t deg2pulse(int16_t deg) {
//    return map(deg, -90, 90, 123, 492); // 600 .. 2400 us pulses
    return map(deg, -90, 90, 143, 471); // 700 .. 2300 pulses
  }

  void setLimits(int16_t min_deg, int16_t max_deg) {
    _min_pulse = deg2pulse(min_deg);
    _max_pulse = deg2pulse(max_deg);
  }
  
  void write_pulse(int16_t pulse, long int delay_ms) {
    _start_pulse = _curr_pulse;
    _end_pulse = pulse;
    _start_ms = millis();
    _end_ms = _start_ms + delay_ms;
  }

  void write(int16_t degree, long int delay_ms) {
    write_pulse(deg2pulse(degree), delay_ms);
  }

  void write(int16_t degree) {
    write_speed(degree, _max_dps);
  }

  void write_speed(int16_t degree, int16_t speed_dps) {
    degree = constrain(degree, -90, 90);
    speed_dps = constrain(speed_dps, 1, 100); /* arbitrary max 100°/s */
    int16_t pulse = deg2pulse(degree);
    // delay_ms = degrees / deg_per_ms = 1000 * deg/deg_per_s;
    long int delay_ms = abs(degree - read()) * 100 / speed_dps;
    write_pulse(pulse, delay_ms);
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
      pwm.setPWM(_num, offset, 0x0fff & (offset + _curr_pulse));
      _last_pulse = _curr_pulse;
    }
  }
};

// Initialization of static member
bool SlowServo::_pwm_started = false;

/****************************************************************
 * Library for interfacing with the DRV 10970 motor driver.
 *
 * @author: Garrett Wells
 * @date: 01/18/22
 *
 * Provides a configurable interface for working with the DRV 10970 motor driver. Initially written for University of Idaho senior capstone 2022 for use by the
 * Attitude adjustment team working for NASA.
 ****************************************************************/
enum MotorDirection {FWD, REV};

class DRV10970 {
    private:
        int FG, FR, BRKMOD, PWM, RD; // interface pins
    public:
        void DRV10970(int fg, int fr, int brkmod, int pwm, int rd);
        void run(MotorDirection dir, int dc); // drive motor in direction at dutycycle dc
        void stop(); // stop motor driver and put in low power state
        int read(); // returns the rpm of motor spindle
        bool spindleFree(); // returns true if motor spindle is free to spin
 };

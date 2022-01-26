#include "comm.h"
#include "supportFunctions.h"
#include "commandFunctions.h"
#include "ICM_20948.h"
#include "DRV_10970.h"
#include "INA209.h"
#include <FreeRTOS_SAMD51.h>
#include <stdint.h>

/*
 * ADCS system test rig. Run to see test output on serial output in a loop.
 * @author  Garrett Wells
 * @since   01/23/2022
 */

// Initialization functions, each should print to serial debug on success/failure
void init(void);       // init ADCS systems and sensors
void initIMU(void);    // init IMU
void initDRV(void);    // init motor driver
void initINA(void);    // init current monitor

// Test functions, each should get input or send value to sensor or subsystem and then read/get feedback and print success/failure
void testIMU(void);     // test IMU readings to make sure values are plausible
void testDRV(void);     // spin up in increments, and test ability to read wheel rpm
void testINA(void);     // measure the draw of the system, should be non-zero

/*
 * Init for the tests
 */
void setup(){
    // start serial debug
    Serial.begin(115200);

    init(); // init all ADCS systems

}

/*
 * Run tests here to infinity
 */
void loop(){
    Serial.println("------ START TESTING SUBSYSTEMS ------");
    // TODO: test IMU, values back should be close to zero if not moving
    Serial.println("------ END TESTING SUBSYSTEMS ------\n");
}

/*
 * Init all systems here...
 */
void init(void){
    Serial.println("------ STARTING SYSTEM INIT ------");
    initIMU();
    initDRV();
    initINA();
    Serial.println("------ FINISHING SYSTEM INIT ------");
}

/*
 * Init the IMU connection over the I2C interface
 */
void initIMU(void){
    /**
     * Initialize I2C connection to IMU
     * Clock: 400 kHz
     * IMU address: 0x69
     */
    long int t0 = millis();
    SERCOM_I2C.begin();
    SERCOM_I2C.setClock(400000);
    IMU.begin(SERCOM_I2C, AD0_VAL);
    while (IMU.status != ICM_20948_Stat_Ok);  // wait for initialization to
    long int cT = millis();
    Serial.print("INIT IMU [SUCCESS] in ");
    Serial.print(cT - t0);
    Serial.println(" ms");
}

/*
 * Init the motor driver for the flywheel
 */
void initDRV(void){
    if(DRV_FG == 0 || DRV_FR == 0 || DRV_BRKMOD == 0 || DRV_PWM == 0 || DRV_RD == 0){
        Serial.println("INIT DRV10970 [FAILED]\n\t invalid pinout");

    }else {
        long int t0 = millis();
        DRV = new DRV10970(DRV_FG, DRV_FR, DRV_BRKMOD, DRV_PWM, DRV_RD);
        long int cT = millis();
        Serial.print("INIT DRV10970 [SUCCESS] in ");
        Serial.print(cT - t0);
        Serial.println(" ms");

    }
 }

/*
 * Init the INA209 current sensor for monitoring system draw
 */
void initINA(void){
    // TODO init INA209 with real values, defaults are for 32V system
    long int t0 = millis();
    ina209 = new INA209(0x40);
    ina209.writeCfgReg(14751); // default
    ina209.writeCal(4096);
    long int cT = millis();
    Serial.print("INIT INA209 [SUCCESS] in ");
    Serial.print(cT - t0);
    Serial.println(" ms");
}

/*
 * Test the IMU, values should be close to zero but probably not zero if the unit is stationary
 */
void testIMU(void){
    Serial.println("\tIMU TEST");
    IMU.readScaledAGMT();
    Serial.println("");
}

/*
 * Test the DRV10979 motor driver
 */
void testDRV(void){
    Serial.println("\tDRV10970 TEST");

    Serial.println("PWM ~ 10%");
    DRV.run(FWD, 0.1*255); // start at 10%
    const int duration = 10000; // 10s
    volatile long int t0 = millis();
    while(millis() - t0 < duration){/*do nothing*/}

    Serial.println("PWM ~ 20%");
    t0 = millis();
    DRV.run(FWD, 0.2*255);
    while(millis() - t0 < duration){/*do nothing*/}

    // TODO: read the spindle rpm

    Serial.println("\tstopping DRV10970");
    DRV.stop();
    Serial.println("\tDRV10970 stopped");

}

/*
 * Test the INA209's ability to monitor the power draw of the system
 */
void testINA(void){
    Serial.println("\tINA209 TEST");
    Serial.print("\t\tINA209 current (");
    Serial.print(ina209.current())
    Serial.println(") Amps");
}

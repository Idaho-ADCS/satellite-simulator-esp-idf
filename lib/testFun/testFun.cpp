#include "testFun.h"

/*
 * Run tests here 
 */
void TEST(){
    Serial.println("------ START TESTING SUBSYSTEMS ------");
    // Test IMU, values back should be close to zero if not moving
    testIMU();
    testINA();
    Serial.println("------ END TESTING SUBSYSTEMS ------\n");
}


/*
 * Test the IMU, values should be close to zero but probably not zero if the unit is stationary
 */
void testIMU(void){
    Serial.println("\tIMU TEST");
    //printScaledAGMT(&ICM_20948_INT_STATUS_1_t); //This gives an error lol
    Serial.println("");
}

/*
 * Test the INA209's ability to monitor the power draw of the system
 */
void testINA(void){
    Serial.println("\tINA209 TEST");
    Serial.print("\t\tINA209 current (");
    Serial.print(ina209->current());
    Serial.println(") Amps");
}

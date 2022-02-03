//Allows us to be able to test sensors and their data for errors.
#include "comm.h"
#include "supportFunctions.h"
#include "ICM_20948.h"
#include "INA209.h"
#include <stdint.h>

//The main function that runs everything in testFun
void TEST();

// Test functions, each should get input or send value to sensor or subsystem and then read/get feedback and print success/failure
void testIMU(void);     // test IMU readings to make sure values are plausible
void testINA(void);     // measure the draw of the system, should be non-zero

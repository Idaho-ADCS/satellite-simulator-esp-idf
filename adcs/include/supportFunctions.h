/* These are functions that support the main command functions in one way
 * or another they are called upon in the commandFunctions.cpp file */
/* I tried to make these as self-explanatory as possible by name
 * but I also added little descriptions anyways -Kristie */

#include <FreeRTOS_SAMD51.h>
#include "ICM_20948.h"
#include "comm.h"

#define SERCOM_USB   Serial
#define SERCOM_UART  Serial1

#define MODE_STANDBY  0
#define MODE_TEST     1

/**
 * @brief
 * Stores the current mode of the ADCS.
 * Possible values:
 *   MODE_STANDBY
 *   MODE_TEST
 * 
 * TODO: Add concurrency protections
 */
uint8_t mode;

/**
 * @brief
 * IMU object, attached to IMU. Used to read data from IMU.
 */
ICM_20948_I2C IMU;

/** @brief
 * Stores a command currently being received from the satellite.
 */
TEScommand cmd_packet;

/**
 * @brief
 * Stores sensor data to be sent to the satellite.
 */
ADCSdata data_packet;

//Reads UART and sets it to cmd array if something is there
void readUART(char* cmd);
//Copied from demo (ADD DESCRIPTION)
void writeUART(void *pvParameters);
//Parses cmd and calls appropriate function
void doCmd(char* cmd);
//Reads output of sensors and compiles it into the array
void getData(int* data);
//Sends array data to main satellite system
void sendToSystem(int* data);
//Runs motors for a certain number of rotations
void startRotation(int rotations);

// "helper" functions from IMU demo code that print sensor data to serial
// monitor over USB
void printPaddedInt16b(int16_t val);
void printRawAGMT(ICM_20948_AGMT_t agmt);
void printFormattedFloat(float val, uint8_t leading, uint8_t decimals);
void printScaledAGMT(ICM_20948_I2C *sensor);

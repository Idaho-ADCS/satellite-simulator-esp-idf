#ifndef __SENSORS_H__
#define __SENSORS_H__

#include "ICM_20948.h"
#include "INA209.h"

#define NUM_IMUS 1
#define INA 1

/* DATA TYPES =============================================================== */

typedef struct
{
	float magX;
	float magY;
	float magZ;
	float gyrX;
	float gyrY;
	float gyrZ;
} IMUdata;

typedef struct
{
	float voltage;
	int current;
} INAdata;

/* HARDWARE INIT FUNCTIONS ================================================== */

void initIMU(void);
void initINA(void);

/* SENSOR READING FUNCTIONS ================================================= */

IMUdata readIMU(void);
INAdata readINA(void);

/* PRINTING FUNCTIONS ======================================================= */

// "helper" functions from IMU demo code that print sensor data to serial
// monitor over USB
void printPaddedInt16b(int16_t val);
void printRawAGMT(ICM_20948_AGMT_t agmt);
void printFormattedFloat(float val, uint8_t leading, uint8_t decimals);
void printScaledAGMT(ICM_20948_I2C *sensor);

#endif
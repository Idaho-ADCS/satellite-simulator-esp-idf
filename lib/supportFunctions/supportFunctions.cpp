/* These are functions that support the main command functions in one way
 * or another they are called upon in the commandFunctions.cpp file */

#include "supportFunctions.h"
#include "commandFunctions.h"
#include <FreeRTOS_SAMD51.h>
#include "comm.h"
#include "ICM_20948.h"
#include <stdint.h>

//Parses cmd and calls appropriate function
void doCmd(char *cmd)
{
	if (cmd[0] == 0xa0) //Binary: 10100000
	{
		testFun();
	}
	else if (cmd[0] == 0xc0) //Binary: 11000000
	{
		standby();
	}
	//For now, the direction doesn't matter, but it's set up for later
	else if (cmd[0] == 0xe0) //Binary: 11100000
	{
		orient("X+");
	}
	else if (cmd[0] == 0xe1) //Binary: 11100001
	{
		orient("Y+");
	}
	else if (cmd[0] == 0xe2) //Binary: 11100010
	{
		orient("X-");
	}
	else if (cmd[0] == 0xe3) //Binary: 11100011
	{
		orient("Y-");
	}
	return;
}

//Reads all sensor output and compiles everything into the data array
/* WIP: I am unsure what data type data should be (prob int or float?)..
 * I know it should be an array -Kristie */
void getData(int *data)
{
	return;
}

/* WIP: This will use the same data from function above, sends out to main
 * satellite system */
void sendToSystem(int *data)
{
	return;
}

//Runs motors for a certain number of rotations
/* WIP: This function will need to take a rotation value (assuming int type)
 * from orient and then rotate the motors that amount */
void startRotation(int rotations)
{
	return;
}

void printPaddedInt16b(int16_t val)
{
	if (val > 0)
	{
		SERCOM_USB.print(" ");
		if (val < 10000)
		{
			SERCOM_USB.print("0");
		}
		if (val < 1000)
		{
			SERCOM_USB.print("0");
		}
		if (val < 100)
		{
			SERCOM_USB.print("0");
		}
		if (val < 10)
		{
			SERCOM_USB.print("0");
		}
	}
	else
	{
		SERCOM_USB.print("-");
		if (abs(val) < 10000)
		{
			SERCOM_USB.print("0");
		}
		if (abs(val) < 1000)
		{
			SERCOM_USB.print("0");
		}
		if (abs(val) < 100)
		{
			SERCOM_USB.print("0");
		}
		if (abs(val) < 10)
		{
			SERCOM_USB.print("0");
		}
	}
	SERCOM_USB.print(abs(val));
}

/**
 * @brief
 * This function has something to do with printing IMU data over USB to the
 * serial monitor. Other than that, I have no idea what it does. It came as part
 * of the IMU demo code, and printScaledAGMT, which is used to validate data
 * transmissions, relies on this function.
 * 
 * @param[in] agmt  An instance of the IMU data object
 * 
 * @return None
 */
void printRawAGMT(ICM_20948_AGMT_t agmt)
{
	SERCOM_USB.print("RAW. Acc [ ");
	printPaddedInt16b(agmt.acc.axes.x);
	SERCOM_USB.print(", ");
	printPaddedInt16b(agmt.acc.axes.y);
	SERCOM_USB.print(", ");
	printPaddedInt16b(agmt.acc.axes.z);
	SERCOM_USB.print(" ], Gyr [ ");
	printPaddedInt16b(agmt.gyr.axes.x);
	SERCOM_USB.print(", ");
	printPaddedInt16b(agmt.gyr.axes.y);
	SERCOM_USB.print(", ");
	printPaddedInt16b(agmt.gyr.axes.z);
	SERCOM_USB.print(" ], Mag [ ");
	printPaddedInt16b(agmt.mag.axes.x);
	SERCOM_USB.print(", ");
	printPaddedInt16b(agmt.mag.axes.y);
	SERCOM_USB.print(", ");
	printPaddedInt16b(agmt.mag.axes.z);
	SERCOM_USB.print(" ], Tmp [ ");
	printPaddedInt16b(agmt.tmp.val);
	SERCOM_USB.print(" ]");
	SERCOM_USB.println();
}

/**
 * @brief
 * This function has something to do with printing IMU data over USB to the
 * serial monitor. Other than that, I have no idea what it does. It came as part
 * of the IMU demo code, and printScaledAGMT, which is used to validate data
 * transmissions, relies on this function.
 * 
 * @param[in] val       Value to print
 * @param[in] leading   Number of digits left of the decimal
 * @param[in] decimals  Number of digits right of the decimal
 * 
 * @return None
 */
void printFormattedFloat(float val, uint8_t leading, uint8_t decimals)
{
	float aval = abs(val);
	if (val < 0)
	{
		SERCOM_USB.print("-");
	}
	else
	{
		SERCOM_USB.print(" ");
	}
	for (uint8_t indi = 0; indi < leading; indi++)
	{
		uint32_t tenpow = 0;
		if (indi < (leading - 1))
		{
			tenpow = 1;
		}
		for (uint8_t c = 0; c < (leading - 1 - indi); c++)
		{
			tenpow *= 10;
		}
		if (aval < tenpow)
		{
			SERCOM_USB.print("0");
		}
		else
		{
			break;
		}
	}
	if (val < 0)
	{
		SERCOM_USB.print(-val, decimals);
	}
	else
	{
		SERCOM_USB.print(val, decimals);
	}
}

/**
 * @brief
 * Prints IMU data over USB to the serial monitor. Converts raw data to a form
 * that is readable by humans. Came as part of the IMU demo code.
 * 
 * @param[in] sensor  Pointer to IMU object
 * 
 * @return None
 */
void printScaledAGMT(ICM_20948_I2C *sensor)
{
	SERCOM_USB.print("Scaled. Acc (mg) [ ");
	printFormattedFloat(sensor->accX(), 5, 2);
	SERCOM_USB.print(", ");
	printFormattedFloat(sensor->accY(), 5, 2);
	SERCOM_USB.print(", ");
	printFormattedFloat(sensor->accZ(), 5, 2);
	SERCOM_USB.print(" ], Gyr (DPS) [ ");
	printFormattedFloat(sensor->gyrX(), 5, 2);
	SERCOM_USB.print(", ");
	printFormattedFloat(sensor->gyrY(), 5, 2);
	SERCOM_USB.print(", ");
	printFormattedFloat(sensor->gyrZ(), 5, 2);
	SERCOM_USB.print(" ], Mag (uT) [ ");
	printFormattedFloat(sensor->magX(), 5, 2);
	SERCOM_USB.print(", ");
	printFormattedFloat(sensor->magY(), 5, 2);
	SERCOM_USB.print(", ");
	printFormattedFloat(sensor->magZ(), 5, 2);
	SERCOM_USB.print(" ], Tmp (C) [ ");
	printFormattedFloat(sensor->temp(), 5, 2);
	SERCOM_USB.print(" ]");
	SERCOM_USB.println();
}
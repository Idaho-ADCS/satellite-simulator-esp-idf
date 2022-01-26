/* These are functions that support the main command functions in one way
 * or another they are called upon in the commandFunctions.cpp file */

#include "supportFunctions.h"
#include "commandFunctions.h"
#include <FreeRTOS_SAMD51.h>
#include "comm.h"
#include "ICM_20948.h"
#include <stdint.h>

//WIP: Reads UART and sets it to cmd array input if something is there
void readUART(char *message)
{
	//Below is from the demo that Parker created
	/* The only difference between the demo piece and this is that the while loop was 
   * removed from the function and moved to the function it was called by */
	uint8_t bytes_received = 0; // number of consecutive bytes received from
								// satellite - used as index for cmd packet
								// char array
#ifdef DEBUG
	char cmd_str[8]; // used to print command value to serial monitor
#endif
	if (SERCOM_UART.available()) // at least one byte is in the UART
	{							 // receive buffer

		// copy one byte out of UART receive buffer
		cmd_packet.data[bytes_received] = SERCOM_UART.read();
		bytes_received++;

		if (bytes_received >= COMMAND_LEN) // full command packet received
		{
			// TODO: verify CRC

			if (cmd_packet.command == COMMAND_TEST)
			{
				mode = MODE_TEST; // enter test mode
			}

			if (cmd_packet.command == COMMAND_STANDBY)
			{
				mode = MODE_STANDBY; // enter standby mode
			}

#ifdef DEBUG
			// convert int to string for USB monitoring
			sprintf(cmd_str, "0x%02x", cmd_packet.command);

			// print command value to USB
			SERCOM_USB.print("Command received: ");
			SERCOM_USB.print(cmd_str);
			SERCOM_USB.print("\n");

			if (cmd_packet.command == COMMAND_TEST)
			{
				SERCOM_USB.print("Entering test mode\n");
			}

			if (cmd_packet.command == COMMAND_STANDBY)
			{
				SERCOM_USB.print("Entering standby mode\n");
			}
#endif

			// reset index counter to zero for next command
			bytes_received = 0;
		}
	}
	//---End of demo piece------
	else //If there is nothing in UART, make input something else unused
	{
		message[0] = 0x01;
	}

	return;
}

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

/**
 * @brief
 * Reads magnetometer and gyroscope values from IMU and writes them to UART
 * every 0.5 seconds while ADCS is in test mode.
 * 
 * @param[in] pvParameters  Unused but required by FreeRTOS. Program will not
 * compile without this parameter. When a task is instantiated from this
 * function, a set of initialization arguments or NULL is passed in as
 * pvParameters, so pvParameters must be declared even if it is not used.
 * 
 * @return None
 */
static void writeUART(void *pvParameters)
{
	ICM_20948_I2C *sensor_ptr = &IMU; // IMU data can only be accessed through
									  // a pointer

	data_packet.status = STATUS_OK;

	// use static dummy values for voltage, current, and motor speed until we
	// have a device that can monitor them
	data_packet.voltage = 0x01;
	data_packet.current = 0x80;
	data_packet.speed = 0x00;

	while (1)
	{
		if (mode == MODE_TEST)
		{
			if (IMU.dataReady())
			{
				IMU.getAGMT(); // acquires data from sensor

				// extract data from IMU object
				data_packet.magX = (int8_t)sensor_ptr->magX();
				data_packet.magY = (int8_t)sensor_ptr->magY();
				data_packet.magZ = (int8_t)sensor_ptr->magZ();

				data_packet.gyroX = (fixed5_3_t)sensor_ptr->gyrX();
				data_packet.gyroY = (fixed5_3_t)sensor_ptr->gyrY();
				data_packet.gyroZ = (fixed5_3_t)sensor_ptr->gyrZ();

				// TODO: compute CRC

				SERCOM_UART.write(data_packet.data, PACKET_LEN); // send to TES
#ifdef DEBUG
				SERCOM_USB.write("Wrote to UART\n");
				printScaledAGMT(&IMU);
#endif
			}
		}

		vTaskDelay(500 / portTICK_PERIOD_MS);
	}
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
/****************************************************************
 * Example1_Basics.ino
 * ICM 20948 Arduino Library Demo
 * Use the default configuration to stream 9-axis IMU data
 * Owen Lyke @ SparkFun Electronics
 * Original Creation Date: April 17 2019
 *
 * Please see License.md for the license information.
 *
 * Distributed as-is; no warranty is given.
 ***************************************************************/
#include <FreeRTOS_SAMD51.h>
#include "ICM_20948.h" // Click here to get the library: http://librarymanager/All#SparkFun_ICM_20948_IMU

typedef union
{
	uint8_t data[11];

	struct
	{
		uint8_t status;
		uint8_t voltage;
		uint8_t current;
		uint8_t speed;
		uint8_t mag[3];
		uint8_t gyro[3];
	};
} TESdata;

#define COMMAND_LEN  2
#define PACKET_LEN  11

#define SERCOM_USB   Serial
#define SERCOM_UART  Serial1
#define SERCOM_I2C   Wire // Your desired Wire port.      Used when "USE_SPI" is not defined
#define AD0_VAL 1		// The value of the last bit of the I2C address.                \
                       // On the SparkFun 9DoF IMU breakout the default is 1, and when \
                       // the ADR jumper is closed the value becomes 0

ICM_20948_I2C myICM; // Otherwise create an ICM_20948_I2C object

uint8_t mode;

static void readUART(void* pvParameters);
static void readSensors(void* pvParameters);

void printScaledAGMT(ICM_20948_I2C *sensor);

void setup()
{
	mode = 0;

	SERCOM_USB.begin(115200);
	while (!SERCOM_USB);

	SERCOM_UART.begin(115200);
	while (!SERCOM_UART);

	SERCOM_I2C.begin();
	SERCOM_I2C.setClock(400000);

	// myICM.enableDebugging(); // Uncomment this line to enable helpful debug messages on Serial

	bool initialized = false;
	while (!initialized)
	{
		myICM.begin(SERCOM_I2C, AD0_VAL);

		SERCOM_USB.print(F("Initialization of the sensor returned: "));
		SERCOM_USB.println(myICM.statusString());
		if (myICM.status != ICM_20948_Stat_Ok)
		{
			SERCOM_USB.println("Trying again...");
			delay(500);
		}
		else
		{
			initialized = true;
		}
	}

	// pinMode(LED_BUILTIN, OUTPUT);
	// digitalWrite(LED_BUILTIN, HIGH);

	xTaskCreate(readUART, "Read UART", 2048, NULL, 1, NULL);
	xTaskCreate(readSensors, "Read Sensors", 2048, NULL, 1, NULL);
	SERCOM_USB.write("Tasks created\n");

	vTaskStartScheduler();
}

static void readUART(void* pvParameters)
{
	uint8_t bytes_received = 0;
	char message[COMMAND_LEN + 1];
	
	char cmd_str[8];
	char crc_str[8];

	for (int i = 0; i < COMMAND_LEN+1; i++)
	{
		message[i] = 0;
	}

	while (1)
	{
		if (SERCOM_UART.available())
		{
			message[bytes_received] = SERCOM_UART.read();
			bytes_received++;

			if (bytes_received >= COMMAND_LEN)
			{
				if (message[0] == 0xa0)
				{
					mode = 1;
				}

				if (message[0] == 0xc0)
				{
					mode = 0;
				}

				// SERCOM_UART.write(message);

				sprintf(cmd_str, "0x%02x", message[0]);
				sprintf(crc_str, "%02x", message[1]);

				SERCOM_USB.print("Command received: ");
				SERCOM_USB.print(cmd_str);
				// SERCOM_USB.println(crc_str);
				SERCOM_USB.print("\n");

				if (message[0] == 0xa0)
				{
					SERCOM_USB.println("Entering test mode");
				}

				if (message[0] == 0xc0)
				{
					SERCOM_USB.println("Entering standby mode");
				}

				bytes_received = 0;
			}
		}
	}
}

static void readSensors(void* pvParameters)
{
	char data[PACKET_LEN + 1];
	ICM_20948_I2C* sensor_ptr = &myICM;

	for (int i = 0; i < PACKET_LEN+1; i++)
	{
		data[i] = 0;
	}

	data[0] = 0xaa;

	data[1] = 3;
	data[2] = 50;
	data[3] = 1;

	while (1)
	{
		if (mode == 1)
		{
			if (myICM.dataReady())
			{
				myICM.getAGMT();
				printScaledAGMT(&myICM);

				data[4] = (int8_t) sensor_ptr->magX();
				data[5] = (int8_t) sensor_ptr->magY();
				data[6] = (int8_t) sensor_ptr->magZ();

				data[7] = (int8_t) sensor_ptr->gyrX();
				data[8] = (int8_t) sensor_ptr->gyrY();
				data[9] = (int8_t) sensor_ptr->gyrZ();

				data[10] = 0x01;

				SERCOM_UART.write(data);
			}
		}

		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
}

void loop()
{
	// if (myICM.dataReady())
	// {
	// 	myICM.getAGMT();		 // The values are only updated when you call 'getAGMT'
	// 							 //    printRawAGMT( myICM.agmt );     // Uncomment this to see the raw values, taken directly from the agmt structure
	// 	printScaledAGMT(&myICM); // This function takes into account the scale settings from when the measurement was made to calculate the values with units
	// 	delay(30);
	// }
	// else
	// {
	// 	SERCOM_USB.println("Waiting for data");
	// 	delay(500);
	// }

	// if (SERCOM_UART.available())
	// {
	// 	SERCOM_UART.write(SERCOM_UART.read());
	// }
}

// Below here are some helper functions to print the data nicely!

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

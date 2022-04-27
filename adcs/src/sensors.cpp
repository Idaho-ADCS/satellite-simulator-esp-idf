#include "flags.h"
#include "sensors.h"
#include "ADCSPhotodiodeArray.h"
#include "comm.h"

ICM_20948_I2C IMU1;
ICM_20948_I2C IMU2;

INA209 ina209;

ADCSPhotodiodeArray sunSensors(A0, 13, 12, 11);

/* HARDWARE INIT FUNCTIONS ================================================== */

void initIMU(void)
{
	/**
	 * Initialize first IMU
	 * Address: 0x69 or 0x68
	 */
    IMU1.begin(SERCOM_I2C, AD0_VAL);
    while (IMU1.status != ICM_20948_Stat_Ok);  // wait for initialization to
                                               // complete
#if DEBUG
    SERCOM_USB.print("[system init]\tIMU1 initialized\r\n");
#endif

#if NUM_IMUS >= 2
	/**
	 * Initialize second IMU
	 * Address: 0x68 or 0x69
	 */
    IMU2.begin(SERCOM_I2C, AD0_VAL^1);  // initialize other IMU with opposite
										// value for bit 0
    while (IMU2.status != ICM_20948_Stat_Ok);  // wait for initialization to
                                               // complete
	#if DEBUG
    SERCOM_USB.print("[system init]\tIMU2 initialized\r\n");
	#endif
#endif
}

void initINA(void)
{
	/**
	 * Write default settings to INA209
	 * Reset: no
	 * Bus voltage range: 32V
	 * PGA gain: /8
	 * PGA range: +-320mV
	 * ADC resolution: 12 bits
	 * ADC conversion time: 532us
	 * Mode: shunt and bus, continuous
	 */
    ina209.writeCfgReg(0x399f);

	/**
	 * Calibrate INA209
	 * Current LSB: 100uA
	 * 
	 * Can also use 0x6aaa to prevent overflow
	 * 7fff seems to be more accurate though
	 */
    ina209.writeCal(0x7fff);
    
#if DEBUG
    SERCOM_USB.print("[system init]\tINA209 initialized\r\n");
#endif
}

void initSunSensors(void)
{
	sunSensors.init();
#if DEBUG
	SERCOM_USB.print("[system init]\tSun sensors initialized\r\n");
#endif
}

/* SENSOR READING FUNCTIONS ================================================= */

IMUdata readIMU(void)
{
	IMUdata data;

	ICM_20948_I2C *sensor_ptr1 = &IMU1; // IMU data can only be accessed through
                                        // a pointer
#if NUM_IMUS >= 2
	ICM_20948_I2C *sensor_ptr2 = &IMU2; // IMU data can only be accessed through
                                        // a pointer
#endif

	if (IMU1.dataReady())
		IMU1.getAGMT();  // acquires data from sensor

#if NUM_IMUS >= 2
	if (IMU2.dataReady())
		IMU2.getAGMT();

	// extract data from IMU object
	data.magX = (sensor_ptr1->magY() + sensor_ptr2->magY()) / 2;
	data.magY = (sensor_ptr1->magX() + sensor_ptr2->magX()) / 2;
	data.magZ = (sensor_ptr1->magZ() + sensor_ptr2->magZ()) / -2;

	data.gyrX = (sensor_ptr1->gyrY() + sensor_ptr2->gyrY()) / 2;
	data.gyrY = (sensor_ptr1->gyrX() + sensor_ptr2->gyrX()) / 2;
	data.gyrZ = (sensor_ptr1->gyrZ() + sensor_ptr2->gyrZ()) / -2;
#else
	data.magX = sensor_ptr1->magY();
	data.magY = sensor_ptr1->magX();
	data.magZ = sensor_ptr1->magZ() * -1;

	data.gyrX = sensor_ptr1->gyrY();
	data.gyrY = sensor_ptr1->gyrX();
	data.gyrZ = (sensor_ptr1->gyrZ() * -1) - 2.0;
#endif

#if DEBUG
	SERCOM_USB.print("[readIMU]\tMag:  [");
	SERCOM_USB.print(data.magX);
	SERCOM_USB.print(", ");
	SERCOM_USB.print(data.magY);
	SERCOM_USB.print(", ");
	SERCOM_USB.print(data.magZ);
	SERCOM_USB.print("]\r\n");

	SERCOM_USB.print("[readIMU]\tGyro: [");
	SERCOM_USB.print(data.gyrX);
	SERCOM_USB.print(", ");
	SERCOM_USB.print(data.gyrY);
	SERCOM_USB.print(", ");
	SERCOM_USB.print(data.gyrZ);
	SERCOM_USB.print("]\r\n");
#endif

	return data;
}

INAdata readINA(void)
{
	INAdata data;

	int v_raw;
	int i_raw;

	v_raw = ina209.busVol();
	i_raw = ina209.current();

	data.voltage = v_raw / 1000.0f;
	data.current = i_raw / 10.0f;

#if DEBUG
	SERCOM_USB.print("[readINA]\tBus voltage: ");
	SERCOM_USB.print(data.voltage);
	SERCOM_USB.print("V\r\n");

	SERCOM_USB.print("[readINA]\tCurrent: ");
	SERCOM_USB.print(data.current);
	SERCOM_USB.print("mA\r\n");
#endif

	return data;
}

PDdata readPD(void)
{
	PDdata data;
	uint8_t channel;

	for (channel = 0; channel < 6; channel++)
	{
		data.data[channel] = sunSensors.read(channel);
	}

// #if DEBUG
// 	SERCOM_USB.print("[readPD]\tSun sensors: [ [");
// 	SERCOM_USB.print(data.x_pos);
// 	SERCOM_USB.print(", ");
// 	SERCOM_USB.print(data.x_neg);
// 	SERCOM_USB.print("], [");
// 	SERCOM_USB.print(data.y_pos);
// 	SERCOM_USB.print(", ");
// 	SERCOM_USB.print(data.y_neg);
// 	SERCOM_USB.print("], [");
// 	SERCOM_USB.print(data.z_pos);
// 	SERCOM_USB.print(", ");
// 	SERCOM_USB.print(data.z_neg);
// 	SERCOM_USB.print("] ]\r\n");
// #endif

	return data;
}

/* PRINTING FUNCTIONS ======================================================= */

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
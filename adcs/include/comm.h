#ifndef __COMM_H__
#define __COMM_H__

#include "CRC16.h"
#include <stdint.h>

// #define COMM_DEBUG

// create more descriptive names for serial interfaces
#define SERCOM_USB  Serial
#define SERCOM_UART Serial1
#define SERCOM_I2C  Wire
#define AD0_VAL     1

// packet sizes in bytes
#define COMMAND_LEN  3
#define PACKET_LEN   12

// command values (all unsigned ints)
#define COMMAND_STANDBY  0xc0
#define COMMAND_TEST     0xa0

// data packet status codes (all unsigned ints)
#define STATUS_OK     		0xaa
#define STATUS_ERROR  		0xf0
#define STATUS_HELLO  		0xaf
#define STATUS_OVERFLOW(x)  (0x00 + x)

/**
 * @brief
 * Defines a fixed-point data type that is one byte wide. Five bits are reserved
 * for the integer part and 3 bits are reserved for the fraction part. While in
 * fixed-point form, the data cannot be read - it must be converted back to
 * floating-point first. Functions that convert float to fixed and back are
 * declared below.
 */
typedef int8_t fixed5_3_t;

class TEScommand
{
private:
	union
	{
		uint8_t data[COMMAND_LEN];

		struct
		{
			uint8_t  command;
			uint16_t crc;
		};
	};

	uint8_t bytes_received;
	bool full;

public:
	TEScommand();
	void addByte(uint8_t b);
	bool isFull();
	uint8_t getCommand();
	bool checkCRC();
	void clear();
};

class ADCSdata
{
private:
	union
	{
		uint8_t data[PACKET_LEN];

		struct
		{
			uint8_t    status;
			fixed5_3_t voltage;
			int8_t     current;
			uint8_t    speed;
			int8_t     magX;
			int8_t	   magY;
			int8_t	   magZ;
			fixed5_3_t gyroX;
			fixed5_3_t gyroY;
			fixed5_3_t gyroZ;
			uint16_t   crc;
		};
	};

public:
	ADCSdata();
	void setStatus(uint8_t s);
	void setINAdata(float v, float i);
	void setSpeed(float s);
	void setIMUdata(float mx, float my, float mz, float gx, float gy, float gz);
	char *getData();
	void computeCRC();
	bool checkCRC();
	void clear();
	void send();
};

// fixed/float conversions
fixed5_3_t floatToFixed(float f);
float fixedToFloat(fixed5_3_t fix);

#endif
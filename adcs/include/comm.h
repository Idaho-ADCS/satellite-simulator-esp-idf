#ifndef __COMM_H__
#define __COMM_H__

#include "CRC16.h"
#include <stdint.h>

// create more descriptive names for serial interfaces
#define SERCOM_USB  Serial
#define SERCOM_UART Serial1
#define SERCOM_I2C  Wire
#define AD0_VAL     1

// packet sizes in bytes
#define COMMAND_LEN  4
#define PACKET_LEN   12

// command values
enum Command : uint8_t
{
	STANDBY = 0xc0,
	TEST    = 0xa0
};

// data packet status codes
enum Status : uint8_t
{
	OK         = 0xaa,
	HELLO      = 0xaf,
	ADCS_ERROR = 0xf0,
	COMM_ERROR = 0x99
};

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
		uint8_t _data[COMMAND_LEN];

		struct
		{
			uint16_t _command;
			uint16_t _crc;
		};
	};

	uint8_t _bytes_received;
	bool    _full;

public:
	TEScommand();

	void    addByte(uint8_t b);
	bool    isFull();
	uint8_t getCommand();
	bool    checkCRC();
	void    clear();
};

class ADCSdata
{
private:
	union
	{
		uint8_t _data[PACKET_LEN];

		struct
		{
			uint8_t    _status;
			fixed5_3_t _voltage;
			int8_t     _current;
			uint8_t    _speed;
			int8_t     _magX;
			int8_t	   _magY;
			int8_t	   _magZ;
			fixed5_3_t _gyroX;
			fixed5_3_t _gyroY;
			fixed5_3_t _gyroZ;
			uint16_t   _crc;
		};
	};

public:
	ADCSdata();
	void setStatus(uint8_t s);
	void setINAdata(float v, float i);
	void setSpeed(float s);
	void setIMUdata(float mx, float my, float mz, float gx, float gy, float gz);
	void computeCRC();
	void clear();
	void send();
};

// fixed/float conversions
fixed5_3_t floatToFixed(float f);
float fixedToFloat(fixed5_3_t fix);

#endif
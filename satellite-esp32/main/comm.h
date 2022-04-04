#include "driver/gpio.h"

// packet sizes in bytes
#define COMMAND_LEN 4
#define PACKET_LEN  13

// command values
enum Command
{
	CMD_STANDBY = 0xc0,
	CMD_TEST    = 0xa0
};

// data packet status codes
enum Status
{
	STATUS_OK         = 0xaa,  // "Heartbeat"
	STATUS_HELLO      = 0xaf,  // Sent upon system init
	STATUS_ADCS_ERROR = 0xf0,  // Sent upon runtime error
	STATUS_COMM_ERROR = 0x99   // Sent upon invalid communication
};

typedef int8_t fixed5_3_t;

typedef union
{
	uint8_t _data[COMMAND_LEN];

	struct
	{
		uint16_t _command;
		uint16_t _crc;
	};
} TEScommand;

typedef struct
{
	int _seq;

	union
	{
		// Data can be accessed as a single array - used to send via UART
		uint8_t _data[PACKET_LEN];

		struct
		{
			// Data can be accessed as fields - used to build packet
			uint8_t    _status;
			fixed5_3_t _voltage;
			int16_t    _current;
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
} ADCSdata;

#define TXD_PIN (GPIO_NUM_1)
#define RXD_PIN (GPIO_NUM_2)

void init_uart(void);
int send_command(uint8_t cmd);

// fixed/float conversions
fixed5_3_t floatToFixed(float f);
float fixedToFloat(fixed5_3_t fix);
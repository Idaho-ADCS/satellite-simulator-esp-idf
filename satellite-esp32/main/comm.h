#include "driver/gpio.h"

// packet sizes in bytes
#define COMMAND_LEN 4
#define PACKET_LEN  14

// command values
enum Command
{
	CMD_DESATURATE  = 0x00,     // bring everything to a stop, maybe turn off?
	CMD_STANDBY     = 0xc0,
	CMD_HEARTBEAT   = 0xa0,     // transmit heartbeat signal regularly
	CMD_TST_BASIC_MOTION = 0xa1, // test how much force needed to rotate
	CMD_TST_BASIC_AD = 0xa2,    // test attitude determination
	CMD_TST_BASIC_AC = 0xa3,    // test attitude control
	CMD_TST_SIMPLE_DETUMBLE = 0xa4, // test simplistic detumble 
	CMD_TST_SIMPLE_ORIENT   = 0xa5, // test simplistic orientation
	CMD_ORIENT_DEFAULT = 0x80,  // should be orienting to something like X+
	CMD_ORIENT_X_POS= 0xe0,
	CMD_ORIENT_Y_POS= 0xe1,
	CMD_ORIENT_X_NEG= 0xe2,
	CMD_ORIENT_Y_NEG= 0xe3
};

// data packet status codes
enum Status
{
	STATUS_OK         = 0xaa,  // "Heartbeat"
	STATUS_HELLO      = 0xaf,  // Sent upon system init
	STATUS_ADCS_ERROR = 0xf0,  // Sent upon runtime error
	STATUS_COMM_ERROR = 0x99,  // Sent upon invalid communication
	STATUS_FUDGED     = 0x00,  // Data is not real, just test output
	STATUS_TEST_START = 0xb0,  // starting test
	STATUS_TEST_END   = 0xb1  // test finished
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
			uint16_t   _status;
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
void disable_uart(void);
int send_command(uint8_t cmd);

void rx_task(void *arg);

// fixed/float conversions
fixed5_3_t floatToFixed(float f);
float fixedToFloat(fixed5_3_t fix);
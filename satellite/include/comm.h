#pragma once  // include guard

#include <stdint.h>

#define COMMAND_LEN  2
#define PACKET_LEN   11

// command values (all unsigned ints)
#define COMMAND_STANDBY  0xc0u
#define COMMAND_TEST     0xa0u

// data packet status codes (all unsigned ints)
#define STATUS_OK     		0xaau
#define STATUS_ERROR  		0xf0u
#define STATUS_HELLO  		0xafu
#define STATUS_OVERFLOW(x)  (0x00u + x)

/**
 * @brief
 * Defines a fixed-point data type that is one byte wide. Five bits are reserved
 * for the integer part and 3 bits are reserved for the fraction part. While in
 * fixed-point form, the data cannot be read - it must be converted back to
 * floating-point first. Functions that convert float to fixed and back are
 * declared below.
 */
typedef int8_t fixed5_3_t;

/**
 * @brief
 * Data type that stores commands sent from the satellite to the ADCS. The
 * struct fields are unioned with a char array to make it easier to write to
 * objects of this type.
 */
typedef union
{
    char data[COMMAND_LEN];

    struct
    {
        uint8_t command;
        uint8_t crc;
    };
} TEScommand;

/**
 * @brief
 * Data type that stores data collected from the ADCS sensors. Struct fields are
 * unioned with a char array so the data can be written to the UART, as the UART
 * module will only write char arrays.
 */
typedef union
{
    char data[PACKET_LEN];

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
        uint8_t    crc;
    };
} ADCSdata;

// zero out command/data packets
void clearTEScommand(TEScommand *tes);
void clearADCSdata(ADCSdata *adcs);

// fixed/float conversions
fixed5_3_t floatToFixed(float f);
float fixedToFloat(fixed5_3_t fix);
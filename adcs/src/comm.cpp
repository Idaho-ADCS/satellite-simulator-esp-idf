#include "comm.h"

ADCSdata::ADCSdata()
{
	clear();
}

void ADCSdata::setStatus(uint8_t s)
{
	status = s;
}

void ADCSdata::setINAdata(float v, float i)
{
	voltage = floatToFixed(v);
	current = (int8_t)(i / 10);
}

void ADCSdata::setIMUdata(float mx, float my, float mz, float gx, float gy, float gz)
{
	magX = (int8_t)mx;
	magY = (int8_t)my;
	magZ = (int8_t)mz;

	gyroX = floatToFixed(gx);
	gyroY = floatToFixed(gy);
	gyroZ = floatToFixed(gz);
}

char *ADCSdata::getData()
{
	return (char*)data;
}

void ADCSdata::computeCRC()
{
	CRC16 crcGen;
	char crc_str[8];

	// for (int i = 0; i < PACKET_LEN-2; i++)
	// 	crcGen.add(data[i]);

	crcGen.add(data, PACKET_LEN-2);

	crc = crcGen.getCRC();

	sprintf(crc_str, "0x%02x", crcGen.getCRC());
	SERCOM_USB.write("CRC: ");
	SERCOM_USB.write(crc_str);
	SERCOM_USB.write("\r\n");
}

uint16_t ADCSdata::validateCRC()
{
	CRC16 crcGen;

	for (int i = 0; i < PACKET_LEN; i++)
		crcGen.add(data[i]);

	return crcGen.getCRC();
}

void ADCSdata::clear()
{
	for (int i = 0; i < PACKET_LEN; i++)
		data[i] = 0;
}

/**
 * @brief
 * Zeroes out a packet that will receive a command from the satellite. Used to
 * ensure the program doesn't attempt to read uninitialized data.
 * 
 * @param[out] tes  Command packet - passed by reference
 */
void clearTEScommand(TEScommand *tes)
{
    int i;
    for (i = 0; i < COMMAND_LEN; i++)
    {
        tes->data[i] = 0;
    }
}

/**
 * @brief
 * Zeroes out a packet that will be filled with data collected by the ADCS. Used
 * to ensure the program doesn't attempt to read uninitialized data.
 * 
 * @param[out] adcs  Data packet - passed by reference
 */
// void clearADCSdata(ADCSdata *adcs)
// {
//     int i;
//     for (i = 0; i < PACKET_LEN; i++)
//     {
//         adcs->data[i] = 0;
//     }
// }

/**
 * @brief
 * Converts a floating-point number to a fixed-point number with 5 bits for the
 * integer part and 3 bits for the fraction part. Resulting data cannot be
 * properly interpreted until converted back into a float.
 * 
 * @param[in] f  Float to convert
 * 
 * @return Fixed-point conversion
 */
fixed5_3_t floatToFixed(float f)
{
    fixed5_3_t fix;
    fix = (uint8_t)(f * (1 << 3));
    return fix;
}

/**
 * @brief
 * Converts a fixed-point number with 5 integer bits and 3 fraction bits to a
 * floating-point number. Resulting data can be properly interpreted with no
 * extra conversions.
 * 
 * @param[in] fix  Fixed-point number to convert
 * 
 * @return Float conversion
 */
float fixedToFloat(fixed5_3_t fix)
{
    float f;
    f = ((float)fix) / (1 << 3);
    return f;
}
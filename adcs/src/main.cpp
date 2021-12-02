#include "comm.h"
#include "ICM_20948.h"
#include <FreeRTOS_SAMD51.h>
#include <stdint.h>

// if defined, enables debug print statements over USB to the serial monitor
#define DEBUG

// create more descriptive names for serial interfaces
#define SERCOM_USB   Serial
#define SERCOM_UART  Serial1
#define SERCOM_I2C   Wire
#define AD0_VAL      1  // last bit of I2C address

// descriptive names for mode values
#define MODE_STANDBY  0
#define MODE_TEST     1

/* GLOBAL VARIABLES ========================================================= */

/**
 * @brief
 * IMU object, attached to IMU. Used to read data from IMU.
 */
ICM_20948_I2C IMU;

/**
 * @brief
 * Stores the current mode of the ADCS.
 * Possible values:
 *   MODE_STANDBY
 *   MODE_TEST
 * 
 * TODO: Add concurrency protections
 */
uint8_t mode;

/**
 * @brief
 * Stores a command currently being received from the satellite.
 */
TEScommand cmd_packet;

/**
 * @brief
 * Stores sensor data to be sent to the satellite.
 */
ADCSdata data_packet;

/* FUNCTION DECLARATIONS ==================================================== */

// task function declarations
static void readUART(void *pvParameters);
static void writeUART(void *pvParameters);

// "helper" functions from IMU demo code that print sensor data to serial
// monitor over USB
void printPaddedInt16b(int16_t val);
void printRawAGMT(ICM_20948_AGMT_t agmt);
void printFormattedFloat(float val, uint8_t leading, uint8_t decimals);
void printScaledAGMT(ICM_20948_I2C *sensor);

/* FUNCTION DEFINITIONS ===================================================== */

/**
 * @brief
 * Since main is already defined by the Arduino framework, we will use the
 * setup function as if it were main. Since setup runs only once, it
 * essentially behaves the same as main.
 */
void setup()
{
    mode = MODE_STANDBY;  // boot into standby mode

    // initialize command and data packets to zeros
    clearTEScommand(&cmd_packet);
    clearADCSdata(&data_packet);

#ifdef DEBUG
    /**
     * Initialize UART connection to satellite
     * Baud rate: 115200
     * Data bits: 8
     * Parity: none
     */
    SERCOM_USB.begin(115200);
    while (!SERCOM_USB);  // wait for initialization to complete
    SERCOM_USB.write("USB interface initialized\r\n");
#endif

    /**
     * Initialize UART connection to satellite
     * Baud rate: 115200
     * Data bits: 8
     * Parity: odd (1 bit)
     */
    SERCOM_UART.begin(115200, SERIAL_8O1);
    while (!SERCOM_UART);  // wait for initialization to complete
#ifdef DEBUG
    SERCOM_USB.write("UART interface initialized\r\n");
#endif

    /**
     * Initialize I2C connection to IMU
     * Clock: 400 kHz
     * IMU address: 0x69
     */
    SERCOM_I2C.begin();
    SERCOM_I2C.setClock(400000);
    IMU.begin(SERCOM_I2C, AD0_VAL);
    while (IMU.status != ICM_20948_Stat_Ok);  // wait for initialization to
                                              // complete
#ifdef DEBUG
    SERCOM_USB.write("IMU initialized\r\n");
#endif

    // initialization completed, notify satellite
    data_packet.status = STATUS_HELLO;
    // TODO: compute CRC
    SERCOM_UART.write(data_packet.data, PACKET_LEN);

    // instantiate tasks and start scheduler
    xTaskCreate(readUART, "Read UART", 2048, NULL, 1, NULL);
    xTaskCreate(writeUART, "Write UART", 2048, NULL, 1, NULL);
#ifdef DEBUG
    SERCOM_USB.write("Tasks created\r\n");
#endif

    vTaskStartScheduler();

    // should never be reached if everything goes right
    while (1);
}

/**
 * @brief
 * Polls the UART module for data. Processes data one byte at a time if the
 * module reports that data is ready to be received.
 * 
 * @param[in] pvParameters  Unused but required by FreeRTOS. Program will not
 * compile without this parameter. When a task is instantiated from this
 * function, a set of initialization arguments or NULL is passed in as
 * pvParameters, so pvParameters must be declared even if it is not used.
 * 
 * @return None
 * 
 * TODO: Remove polling and invoke this task using an interrupt instead.
 */
static void readUART(void *pvParameters)
{
    uint8_t bytes_received = 0;  // number of consecutive bytes received from
                                 // satellite - used as index for cmd packet
                                 // char array
#ifdef DEBUG
    char cmd_str[8];  // used to print command value to serial monitor
#endif

    while (1)
    {
        if (SERCOM_UART.available())  // at least one byte is in the UART
        {							  // receive buffer

            // copy one byte out of UART receive buffer
            cmd_packet.data[bytes_received] = SERCOM_UART.read();
            bytes_received++;

            if (bytes_received >= COMMAND_LEN)  // full command packet received
            {
                // TODO: verify CRC

                if (cmd_packet.command == COMMAND_TEST)
                {
                    mode = MODE_TEST;  // enter test mode
                }

                if (cmd_packet.command == COMMAND_STANDBY)
                {
                    mode = MODE_STANDBY;  // enter standby mode
                }

#ifdef DEBUG
                // convert int to string for USB monitoring
                sprintf(cmd_str, "0x%02x", cmd_packet.command);

                // print command value to USB
                SERCOM_USB.print("Command received: ");
                SERCOM_USB.print(cmd_str);
                SERCOM_USB.print("\r\n");

                if (cmd_packet.command == COMMAND_TEST)
                {
                    SERCOM_USB.print("Entering test mode\r\n");
                }

                if (cmd_packet.command == COMMAND_STANDBY)
                {
                    SERCOM_USB.print("Entering standby mode\r\n");
                }
#endif

                // reset index counter to zero for next command
                bytes_received = 0;
            }
        }
    }
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
    ICM_20948_I2C *sensor_ptr = &IMU;  // IMU data can only be accessed through
                                       // a pointer

    data_packet.status = STATUS_OK;

    // use static dummy values for voltage, current, and motor speed until we
    // have a device that can monitor them
    data_packet.voltage = 6;
    data_packet.current = 500 / 10;
    data_packet.speed = 1;

    while (1)
    {
        if (mode == MODE_TEST)
        {
            if (IMU.dataReady())
            {
                IMU.getAGMT();  // acquires data from sensor

                // extract data from IMU object
                data_packet.magX = (int8_t)sensor_ptr->magX();
                data_packet.magY = (int8_t)sensor_ptr->magY();
                data_packet.magZ = (int8_t)sensor_ptr->magZ();

                data_packet.gyroX = (fixed5_3_t)sensor_ptr->gyrX();
                data_packet.gyroY = (fixed5_3_t)sensor_ptr->gyrY();
                data_packet.gyroZ = (fixed5_3_t)sensor_ptr->gyrZ();

                // TODO: compute CRC

                SERCOM_UART.write(data_packet.data, PACKET_LEN);  // send to TES
#ifdef DEBUG
                SERCOM_USB.write("Wrote to UART\r\n");
                printScaledAGMT(&IMU);
#endif
            }
        }

        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}

/**
 * @brief
 * Does nothing. Since we are using FreeRTOS, we will not use Arduino's loop
 * function. However, the project will fail to compile if loop is not defined.
 * Therefore, we define loop to do nothing.
 * 
 * TODO: Eliminate this function entirely? Even though it does nothing, it will
 * still likely be called and consume clock cycles.
 */
void loop()
{
    // do nothing
}

/* IMU DEMO FUNCTIONS ======================================================= */

/**
 * @brief
 * This function has something to do with printing IMU data over USB to the
 * serial monitor. Other than that, I have no idea what it does. It came as part
 * of the IMU demo code, and printScaledAGMT, which is used to validate data
 * transmissions, relies on this function.
 * 
 * @param[in] val  Signed value to print
 * 
 * @return None
 */
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

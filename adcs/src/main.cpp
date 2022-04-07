// Our own headers
#include "flags.h"
#include "comm.h"
#include "sensors.h"
#include "supportFunctions.h"
#include "commandFunctions.h"
#include "DRV_10970.h"
// only include this if test functions are desired
//#include <test.h>

// Arduino library headers
#include "INA209.h"
#include "ICM_20948.h"
#include "FreeRTOS_SAMD51.h"

// Standard C/C++ library headers
#include <stdint.h>

/* NON-RTOS GLOBAL VARIABLES ================================================ */

/* RTOS GLOBAL VARIABLES ==================================================== */

/**
 * @brief
 * FreeRTOS queue that stores the current mode of the ADCS.
 * Possible values:
 *   MODE_STANDBY (0)
 *   MODE_TEST    (1)
 */
QueueHandle_t modeQ;

/* RTOS TASK DECLARATIONS =================================================== */

static void receiveCommand(void *pvParameters);
static void writeUART(void *pvParameters);

/* "MAIN" =================================================================== */

/**
 * @brief
 * Since main is already defined by the Arduino framework, we will use the
 * setup function as if it were main. Since setup runs only once, it
 * essentially behaves the same as main.
 */
void setup()
{
	// Create a counting semaphore with a maximum value of 1 and an initial
	// value of 0. Starts ADCS in standby mode.
	modeQ = xQueueCreate(1, sizeof(uint8_t));
	uint8_t mode = MODE_STANDBY;
	xQueueSend(modeQ, (void*)&mode, (TickType_t)0);

	// enable LED
	pinMode(LED_BUILTIN, OUTPUT);
	digitalWrite(LED_BUILTIN, HIGH);

	// pinMode(9, OUTPUT);
	// digitalWrite(9, HIGH);

	// pinMode(10, OUTPUT);
	// analogWrite(10, 127);

#ifdef DEBUG
    initUSB();
#endif

    initUART();

#if NUM_IMUS > 0
	initI2C();
	initIMU();
#endif

#ifdef INA
	initINA();
#endif

    // initialization completed, notify satellite
	ADCSdata data_packet;
    data_packet.setStatus(STATUS_HELLO);
    data_packet.computeCRC();
    data_packet.send();

    // instantiate tasks and start scheduler
    xTaskCreate(receiveCommand, "Read UART", 2048, NULL, 1, NULL);
    xTaskCreate(writeUART, "Write UART", 2048, NULL, 1, NULL);

#ifdef DEBUG
    SERCOM_USB.write("Tasks created\r\n");
#endif

    vTaskStartScheduler();

    // should never be reached if everything goes right
    while (1)
	{
		data_packet.setStatus(STATUS_ADCS_ERROR);
		data_packet.computeCRC();
		data_packet.send();
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
}

/* RTOS TASK DEFINITIONS ==================================================== */

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
static void receiveCommand(void *pvParameters)
{
	TEScommand cmd_packet;
	ADCSdata response;
	uint8_t mode;

#ifdef DEBUG
    char cmd_str[8];  // used to print command value to serial monitor
#endif

    while (1)
    {
        if (SERCOM_UART.available())  // at least one byte is in the UART
        {							  // receive buffer

            // copy one byte out of UART receive buffer
			cmd_packet.addByte((uint8_t)SERCOM_UART.read());

			if (cmd_packet.isFull())  // full command packet received
            {
#ifdef DEBUG
                // convert int to string for USB monitoring
                sprintf(cmd_str, "0x%02x", cmd_packet.getCommand());

                // print command value to USB
                SERCOM_USB.print("Command received: ");
                SERCOM_USB.print(cmd_str);
                SERCOM_USB.print("\r\n");
#endif

				// if (cmd_packet.checkCRC())
				// {
					// process command if CRC is valid
					if (cmd_packet.getCommand() == CMD_TEST)
					{
						mode = MODE_TEST;
#ifdef DEBUG
						SERCOM_USB.write("Entering test mode\n");
#endif
					}

					if (cmd_packet.getCommand() == CMD_STANDBY)
					{
						mode = MODE_STANDBY;
#ifdef DEBUG
						SERCOM_USB.write("Entering standby mode\n");
#endif
					}
// 				}
// 				else
//                 {
// 					// send error message if CRC is not valid
// 					response.setStatus(STATUS_COMM_ERROR);
// 					response.computeCRC();
// 					response.send();
// #ifdef DEBUG
// 					SERCOM_USB.write("CRC check failed\n");
// #endif
// 				}

				xQueueOverwrite(modeQ, (void*)&mode);  // enter specified mode
            }
        }

		// vTaskDelay(1000 / portTICK_PERIOD_MS);
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
	uint8_t mode;
	ADCSdata data_packet;
	INAdata ina;
	IMUdata imu;

#ifdef DEBUG
	char mode_str[8];
#endif

    while (1)
    {
		xQueuePeek(modeQ, (void*)&mode, (TickType_t)0);

        if (mode == MODE_TEST)
        {
			data_packet.setStatus(STATUS_OK);

#if NUM_IMUS > 0
			imu = readIMU();
			data_packet.setIMUdata(imu);
#endif

#ifdef INA
			ina = readINA();
			data_packet.setINAdata(ina);
#endif

			data_packet.computeCRC();
			data_packet.send();  // send to TES

#ifdef DEBUG
			SERCOM_USB.write("Wrote to UART\r\n");
			// printScaledAGMT(&IMU1);
#endif

			data_packet.clear();
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
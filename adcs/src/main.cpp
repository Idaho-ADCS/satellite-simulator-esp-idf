// Our own headers
#include "flags.h"
#include "rtos/rtos_tasks.h"
#include "rtos/rtos_helpers.h"
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

// DRV10970 object
DRV10970 flywhl(6, 9, 0, 10, 5);  // pin 0 needs to be something else

/* RTOS GLOBAL VARIABLES ==================================================== */

/**
 * @brief
 * FreeRTOS queue that stores the current mode of the ADCS.
 * Possible values:
 *   MODE_STANDBY (0)
 *   MODE_HEARTBEAT    (1)
 */
QueueHandle_t modeQ;

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
	uint8_t mode = CMD_STANDBY;
	xQueueSend(modeQ, (void *)&mode, (TickType_t)0);

	// enable LED
	pinMode(LED_BUILTIN, OUTPUT);
	digitalWrite(LED_BUILTIN, HIGH);

	// pinMode(9, OUTPUT);
	// digitalWrite(9, HIGH);

	// pinMode(10, OUTPUT);
	// analogWrite(10, 127);

#if DEBUG
	initUSB();
#endif

	initUART();

#if NUM_IMUS > 0
	initI2C();
	initIMU();
#endif

#if INA
	initINA();
#endif

	pinMode(9, OUTPUT);
	digitalWrite(9, HIGH); // set the direction pin HIGH??

	pinMode(10, OUTPUT);
	analogWrite(10, 0); // set the PWM pin to 0%

	// initialization completed, notify satellite
	ADCSdata data_packet;
	data_packet.setStatus(STATUS_HELLO);
	data_packet.computeCRC();
	data_packet.send();

	// instantiate tasks and start scheduler
	xTaskCreate(receiveCommand, "Read UART", 256, NULL, 2, NULL);
#if DEBUG
	SERCOM_USB.print("[rtos]\t\tTask receiveCommand created\r\n");
#endif

	xTaskCreate(heartbeat, "Write UART", 256, NULL, 1, NULL);
#if DEBUG
	SERCOM_USB.print("[rtos]\t\tTask heartbeat created\r\n");
#endif

#if DEBUG
	SERCOM_USB.print("[rtos]\t\tStarting task scheduler\r\n");
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

void _general_exception_handler(unsigned long ulCause, unsigned long ulStatus)
{
	/* This overrides the definition provided by the kernel.  Other exceptions
	should be handled here. */

	ADCSdata error_msg;
	error_msg.setStatus(STATUS_ADCS_ERROR);

	while (1)
	{
		error_msg.send();
		vNopDelayMS(5000);
	}
}
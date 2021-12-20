

#ifndef FREE_RTOS_SAMD21_H
#define FREE_RTOS_SAMD21_H

	// #include <Arduino.h> //required to prevent a compiling error when a cpp header file tries to include this library

	// Rtos core library
	#include <FreeRTOS/FreeRTOS.h>
	#include <FreeRTOS/timers.h>
	#include <FreeRTOS/task.h>
	#include <FreeRTOS/stack_macros.h>
	#include <FreeRTOS/semphr.h>
	#include <FreeRTOS/queue.h>
	#include <FreeRTOS/projdefs.h>
	#include <FreeRTOS/mpu_wrappers.h>
	#include <FreeRTOS/list.h>
	#include <FreeRTOS/FreeRTOSConfig.h>
	#include <FreeRTOS/event_groups.h>
	#include <FreeRTOS/deprecated_definitions.h>
	#include <FreeRTOS/croutine.h>
	#include <FreeRTOS/message_buffer.h>
	#include <FreeRTOS/stream_buffer.h>

	// hardware specific port files
	#include <FreeRTOS/portmacro.h>
	#include <FreeRTOS/portable.h>
	
	// added helper filed for Arduino support
	#include <FreeRTOS/error_hooks.h>
	#include <FreeRTOS/runTimeStats_hooks.h>


#endif

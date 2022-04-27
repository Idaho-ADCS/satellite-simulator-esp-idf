#ifndef __RTOS_HELPERS_H__
#define __RTOS_HELPERS_H__

#include <stdint.h>
#include "sensors.h"
#include <DRV_10970.h>
#include <ADCSPhotodiodeArray.h>

void state_machine_transition(uint8_t cmd);
void create_test_tasks(void);

MotorDirection getDirection(PDdata); // get direction the adcs should turn to align X+ with the light source

#endif
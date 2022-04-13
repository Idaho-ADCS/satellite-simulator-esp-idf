#ifndef __RTOS_HELPERS_H__
#define __RTOS_HELPERS_H__

#include <stdint.h>

void state_machine_transition(uint8_t cmd);
void create_test_tasks(void);

#endif
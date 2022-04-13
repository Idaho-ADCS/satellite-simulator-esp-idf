#ifndef __RTOS_TASKS_H__
#define __RTOS_TASKS_H__

void receiveCommand(void *pvParameters);
void heartbeat(void *pvParameters);

void basic_motion(void* pvParameters);
void basic_attitude_determination(void *pvParameters);
void basic_attitude_control(void *pvParameters);
void simple_detumble(void *pvParameters);
void simple_orient(void *pvParameters);

#endif
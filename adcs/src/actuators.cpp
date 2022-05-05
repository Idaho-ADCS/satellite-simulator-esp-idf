#include "actuators.h"

// DRV10970 motor driver object
DRV10970 flywhl(A1, 6, 9, 0, 10, 5);  // pin 0 needs to be something else

/**
 * @brief      Initialize pins for flywheel motor driver, DRV10970
 */
void initFlyWhl(void)
{
	flywhl.init();
	#if DEBUG
		SERCOM_USB.print("[system init]\tFlywheel initialized\r\n");
	#endif
}
#pragma once

/* These are the main functionalies and/or processes, they rely on the support
 * functions (located in supportFunctions.cpp) to properally execute */

//This tests the sensors and makes sure they are reading correctly
void testFun();
//This function has the satellite do nothing until something is in UART
void standby();
//This corrdinates the satellite's rotation
void orient(const char *direction);

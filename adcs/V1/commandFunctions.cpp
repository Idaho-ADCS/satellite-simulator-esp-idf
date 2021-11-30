/* These are the main functionalies and/or processes, they rely on the support
 * functions (located in supportFunctions.cpp) to properally execute */

#include "commandFunctions.h"
#include "supportFunctions.h"

//This tests the sensors and makes sure they are reading correctly
void testFun()
{
  return;
}

//This will loop indefinately until there is something in UART
void standby()
{
  int cmd[8];
  do{
    readUART(cmd); //returns 0 in 4th bit if there was no command?
    if(cmd[3] == 0) //If the no command bit *hasn't* been set
    {
     doCmd(cmd);
     return; //This will break the loop
    }
    //Should possibly put a delay in here?
  }while(1);
}

//Starts rotation and checks that rotation actually occured
//This function will be changed later to support direction
void orient()
{
  return;
}

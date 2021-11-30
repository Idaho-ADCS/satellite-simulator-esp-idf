/* These are functions that support the main command functions in one way
 * or another they are called upon in the commandFunctions.cpp file */

#include "supportFunctions.h"
#include "commandFunctions.h"

//WIP: Reads UART and sets it to cmd array input if something is there
void readUART(int* cmd)
{
  //Below is just for testing at the momment
  cmd[3] = 1; //Set the no command bit
  return;
}

//Parses cmd and calls appropriate function
void doCmd(int* cmd)
{
  //cmd is an array length 8
  if(cmd[0] == 0) //Starts with 0
  {
    //shutdown()
    return; //shutdown does not exist as of yet
  }
  else
  {
      if(cmd[1] == 0) //Command: 10......
      {
        testFun();
      }
      else
      {
        if(cmd[2] == 0) //Command: 110.....
        {
          standby();
        }
        else //Command 111.....
        {
          //This function call is temporary
          orient();
          //It will eventually be converted into something like this:
          //orient(cmd);
        }
      }
  }
  return;
}

//Reads all sensor output and compiles everything into the data array
/* WIP: I am unsure what data type data should be (prob int or float?)..
 * I know it should be an array -Kristie */
void getData(int* data)
{
  return;
}

/* WIP: This will use the same data from function above, sends out to main
 * satellite system */
void sendToSystem(int* data)
{
  return;
}

//Runs motors for a certain number of rotations
/* WIP: This function will need to take a rotation value (assuming int type)
 * from orient and then rotate the motors that amount */
void startRotation(int rotations)
{
  return;
}

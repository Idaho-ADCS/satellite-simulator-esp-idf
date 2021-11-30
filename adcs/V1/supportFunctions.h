/* These are functions that support the main command functions in one way
 * or another they are called upon in the commandFunctions.cpp file */
/* I tried to make these as self-explanatory as possible by name
 * but I also added little descriptions anyways -Kristie */

//Reads UART and sets it to cmd array if something is there
void readUART(int* cmd);
//Parses cmd and calls appropriate function
void doCmd(int* cmd);
//Reads output of sensors and compiles it into the array
void getData(int* data);
//Sends array data to main satellite system
void sendToSystem(int* data);

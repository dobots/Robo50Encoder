/*

TODO

*check proportional wheel driver function 
*check on wheel encoder overflow
*seperate motor incident parameters per motor
*make pump work! (write a seperate driver function for it? perhaps use timer?)
*get bumpers on interrupts (may require a uc with more interrupts ;-)

*/


/*********************************
*******      pinout      *********
**********************************

*****************************************/
#include "Arduino.h"
#include "aJSON.h"
#include "log.h"
//#include "digitalWriteFast.h"  //could be used to improve performance


// message constants
#define HEADER 0xA5

#define SENSOR_DATA 0
#define DRIVE_COMMAND 1
#define MOTOR_COMMAND 2
#define CONTROL_COMMAND 3
#define DISCONNECT 4
#define SENSOR_REQUEST 5
#define ENCODER_DATA 6
#define START_STREAMING 7
#define STOP_STREAMING 8

#define INT_T 0
#define DOUBLE_T 1
#define STRING_T 2
#define BOOL_T 3

// right encoder
 #define c_RightEncoderInterrupt 0 //(interrupt 0 is on pin 2)
 #define c_RightEncoderPinA 2
 #define c_RightEncoderPinB A6
 //#define RightEncoderIsReversed   //example, just to let you know this is checked in the cpp file
 volatile bool _RightEncoderBSet; //required for working quadrature encoder

// left encoder
 #define c_LeftEncoderInterrupt 1   //(interrupt 1 is on pin 3)
 #define c_LeftEncoderPinA 3
 #define c_LeftEncoderPinB A7
 #define LeftEncoderIsReversed
 volatile bool _LeftEncoderBSet;

////////////////////////////////
void setup();
void loop();

//communication
void receiveCommands();
void handleDisconnect(aJsonObject* json);
void handleSensorRequest(aJsonObject* json);
void sendData();
void handleInput(int incoming);
int getID(aJsonObject* json);

//sensors
void HandleLeftMotorInterruptA();
void HandleRightMotorInterruptA();

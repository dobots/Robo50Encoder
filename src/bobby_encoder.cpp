#include "debug.h"
#include "bobby_encoder.h"


// JSON message is of the format:
// {"compass":{"heading":119.00000},"accelero":{"x":0.04712,"y":0.00049,"z":0.97757},"gyro":{"x":-0.39674,"y":-1.95318,"z":-1.65563}}

aJsonStream serial_stream(&Serial);

boolean isStreaming = true;

volatile int _LeftEncoderTicks = 0;   //value can be changed in interrupt
volatile int _RightEncoderTicks = 0;

// --------------------------------------------------------------------
void setup() {

	//setup pulse counter callbacks   //interrupts cause problem for drive function
	// Quadrature encoders
	// Left encoder
	pinMode(c_LeftEncoderPinA, INPUT);      // sets pin A as input
	pinMode(c_LeftEncoderPinB, INPUT);      // sets pin B as input
	attachInterrupt(c_LeftEncoderInterrupt, HandleLeftMotorInterruptA, RISING);
	// Right encoder
	pinMode(c_RightEncoderPinA, INPUT);      // sets pin A as input
	pinMode(c_RightEncoderPinB, INPUT);      // sets pin B as input
	attachInterrupt(c_RightEncoderInterrupt, HandleRightMotorInterruptA, RISING);


#ifdef DEBUG
	Serial.begin(115200);
	initLogging(&Serial);
#else
	Serial.begin(115200);
#endif

	LOGd(1, "ready");
}

void loop() {

	if (isStreaming) {
		sendData();
	}

	receiveCommands();							//talky talky

	delay(30); // delay 30ms to get a rate of approx 30Hz
}

//**********************************************************************************
// communication functions

void handleInput(int incoming) {
	switch(incoming) {
		case 'o': sendData(); LOGd(1, ""); break;
		case 's': isStreaming = !isStreaming; LOGd(0, "streaming: %s", isStreaming ? "ON" : "OFF"); break;
		case 'l': log_level = (log_level+1) % 4; LOGd(0, "loglevel: %d", log_level); break;

		default: LOGd(1, "incoming: %c (%d)", incoming, incoming); break;
	}
}

void receiveCommands() {

#ifdef DEBUG
	if (Serial.available()) {
		int incoming = Serial.read();
		handleInput(incoming);
	}
	return;
#endif

	aJsonObject* item;
	if (serial_stream.available()) {
		item = aJson.parse(&serial_stream);

		if (item == NULL) {
			LOGd(0, "not a json object!");
			// while(Serial.available()) {
			// 	Serial3.print(Serial.read());
			// }
			// Serial3.println("");
			serial_stream.flush();
			return;
		}
	} else {
		return;
	}

	switch(getID(item)) {
		case DISCONNECT:
			handleDisconnect(item);
			break;
		case SENSOR_REQUEST:
			handleSensorRequest(item);
			break;
		case START_STREAMING:
			isStreaming = true;
			break;
		case STOP_STREAMING:
			isStreaming = false;
			break;
		default:
			break;
	}
	aJson.deleteItem(item);

}

void handleDisconnect(aJsonObject* json) {
	isStreaming = false;
}

void handleSensorRequest(aJsonObject* json) {
	LOGd(3, "handleSensorRequest");
	sendData();
}

void sendData() {

	aJsonObject *json, *header, *data, *group, *sub, *item;

	json = aJson.createObject();

	aJson.addNumberToObject(json, "id", ENCODER_DATA);
	data = aJson.createObject();

	// ENCODER
	group = aJson.createObject();
    //Ticks can be changed in the interrupts and its a 2-byte value (most likely not atomic)
    //Therefore disable the interrupts while sending the data here!
    uint8_t SaveSREG = SREG;                                            // save interrupt flag
    cli();                                                              // disable interrupts
    aJson.addNumberToObject(group, "rightEncoder", _RightEncoderTicks); // access the shared data
	aJson.addNumberToObject(group, "leftEncoder", _LeftEncoderTicks);   //
    SREG = SaveSREG;                                                    // restore the interrupt flag
	aJson.addItemToObject(data, "odom", group);
	aJson.addItemToObject(json, "data", data);

	aJson.print(json, &serial_stream);
	Serial.println("");
	aJson.deleteItem(json);
}

// Interrupt service routines for the left motor's quadrature encoder
void HandleLeftMotorInterruptA() {

     //for managing with 1 channel encoder (really doesnt work at all)
     //#ifdef LeftEncoderIsReversed
     //    _LeftEncoderTicks -= lastDirectionLeft;
     //	#else
 	//    _LeftEncoderTicks += lastDirectionLeft;
 	//#endif

     //for working 2channel encoder
 	// Test transition; since the interrupt will only fire on 'rising' we don't need to read pin A
 	_LeftEncoderBSet = analogRead(c_LeftEncoderPinB) < 750 ? false : true;   // read the input pin

 	// and adjust counter + if A leads B
 	#ifdef LeftEncoderIsReversed
 	  _LeftEncoderTicks -= _LeftEncoderBSet ? -1 : +1;
 	#else
 	  _LeftEncoderTicks += _LeftEncoderBSet ? -1 : +1;
 	#endif
 }

// Interrupt service routines for the right motor's quadrature encoder
 void HandleRightMotorInterruptA() {

     //for managing with 1 channel encoder (really doesnt work at all)
     //#ifdef RightEncoderIsReversed
     //    _RightEncoderTicks -= lastDirectionRight;
 	//#else
 	//    _RightEncoderTicks += lastDirectionRight;
 	//#endif

     //for working 2channel encoder
 	// Test transition; since the interrupt will only fire on 'rising' we don't need to read pin A
     	_RightEncoderBSet = analogRead(c_RightEncoderPinB) < 750 ? false : true;   // read the input pin  //digitalReadFast

 	// and adjust counter + if A leads B
 	#ifdef RightEncoderIsReversed
 	  _RightEncoderTicks -= _RightEncoderBSet ? -1 : +1;
 	#else
 	  _RightEncoderTicks += _RightEncoderBSet ? -1 : +1;
 	#endif
 }

int getID(aJsonObject* json) {
	aJsonObject* id;
	id = aJson.getObjectItem(json, "id");
	if (id == NULL) {
		LOGd(1, "wrong json message");
		return -1;
	}
	return id->valueint;
}

#include <IRremote.h>

/*
Siehe: http://knx-user-forum.de/forum/%C3%B6ffentlicher-bereich/knx-eib-forum/diy-do-it-yourself/37790-hob2hood-ir-protokoll-arduino/page2
Lüftung Stufe1 0xE3C01BE2
Lüftung Stufe2 0xD051C301
Lüftung Stufe3 0xC22FFFD7
Lüftung Stufe4 0xB9121B29
Lüftung Aus 0x55303A3
Licht An 0xE208293C
Licht Aus 0x24ACF947
*/

// IR commands from AEG hob2hood device
const long IRCMD_VENT_1 = 0xE3C01BE2;
const long IRCMD_VENT_2 = 0xD051C301;
const long IRCMD_VENT_3 = 0xC22FFFD7;
const long IRCMD_VENT_4 = 0xB9121B29;
const long IRCMD_VENT_OFF = 0x55303A3;
const long IRCMD_LIGHT_ON = 0xE208293C;
const long IRCMD_LIGHT_OFF = 0x24ACF947;

// Pins for input switches/controls for manual control of the hood
const int PIN_IN_VENT_1 = A0;
const int PIN_IN_VENT_2 = A1;
const int PIN_IN_VENT_3 = A2;
const int PIN_IN_VENT_4 = A3;
const int PIN_IN_LIGHT = A4;

// Pins for the output which controls relais which finally control the hood
const int PIN_OUT_VENT_1 = 2;
const int PIN_OUT_VENT_2 = 3;
const int PIN_OUT_VENT_3 = 4; // war: 4
const int PIN_OUT_VENT_4 = 5;
const int PIN_OUT_LIGHT = 6; // war: 6
const int PIN_OUT_EXT1 = 7;
const int PIN_OUT_EXT2 = 8;
const int PIN_OUT_EXT3 = 9;

// IR Receiver PIN
const int PIN_IR_RECEIVER = 11;

const int MODE_HOB2HOOD = 0;
const int MODE_MANUAL = 1;

// ventilation, light and mode status
int ventilation = 0;
int last_ventilation = 0;
int light = 0;
int last_light = 0;
int mode = 0; // 0 = hob2hood control, 1 = manual control

IRrecv irrecv(PIN_IR_RECEIVER); // create instance of 'irrecv'
decode_results results;

#define OFF HIGH
#define ON LOW

void setup() {
  // Setup relay-outputs
  pinMode(PIN_OUT_VENT_1, OUTPUT);
  pinMode(PIN_OUT_VENT_2, OUTPUT);
  pinMode(PIN_OUT_VENT_3, OUTPUT);
  pinMode(PIN_OUT_VENT_4, OUTPUT);
  digitalWrite(PIN_OUT_VENT_1, OFF);
  digitalWrite(PIN_OUT_VENT_2, OFF);
  digitalWrite(PIN_OUT_VENT_3, OFF);
  digitalWrite(PIN_OUT_VENT_4, OFF);


  pinMode(PIN_OUT_LIGHT, OUTPUT);
  pinMode(PIN_OUT_EXT1, OUTPUT);
  pinMode(PIN_OUT_EXT2, OUTPUT);
  pinMode(PIN_OUT_EXT3, OUTPUT);

  // define startup-state

  digitalWrite(PIN_OUT_LIGHT, OFF);
  digitalWrite(PIN_OUT_EXT1, OFF);
  digitalWrite(PIN_OUT_EXT2, OFF);
  digitalWrite(PIN_OUT_EXT3, OFF);

  Serial.begin(9600); // for serial monitor output
  Serial.println("Hob2Hood Starting ...");

  Serial.println(" ... Setup IR receiver");
  irrecv.enableIRIn(); // Start the IR receiver
  Serial.println("Hob2Hood ready ...");
}

void loop() {

  // read manual control inputs
  int inLight = analogRead(PIN_IN_LIGHT);
  int inVentilation1 = analogRead(PIN_IN_VENT_1);
  int inVentilation2 = analogRead(PIN_IN_VENT_2);
  int inVentilation3 = analogRead(PIN_IN_VENT_3);
  int inVentilation4 = analogRead(PIN_IN_VENT_4);


  // if any of the manual control inputs is IN USE (analog value >512) --> manual mode
  if (inLight >= 512 ||
      inVentilation1 >= 512 ||
      inVentilation2 >= 512 ||
      inVentilation3 >= 512 ||
      inVentilation4 >= 512 ) {

    if (mode == MODE_HOB2HOOD) {
      Serial.println("Switching to manual mode");
    }

    mode = MODE_MANUAL;

    if (inLight > 512) {
      // Beleuchtung einschalten:
      last_light = light;
      light = 1;
    } else {
      // Beleuchtung ausschalten:
      last_light = light;
      light = 0;
    }

    if (inVentilation1 > 512) {
      // set ventilation speed 1
      last_ventilation = ventilation;
      ventilation = 1;
    } else if (inVentilation2 > 512) {
      // set ventilation speed 2
      last_ventilation = ventilation;
      ventilation = 2;
    } else if (inVentilation3 > 512) {
      // set ventilation speed 3
      last_ventilation = ventilation;
      ventilation = 3;
    } else if (inVentilation4 > 512) {
      // set ventilation speed 4
      last_ventilation = ventilation;
      ventilation = 4;
    } else {
      // set ventilation off
      last_ventilation = ventilation;
      ventilation = 0;
    }

    controlHood();

  } else {

    // now we are in HOB2HOOD-mode, because no manual control is in use

    // check for previous mode
    if (mode == MODE_MANUAL) {
      Serial.println("Switching to Hob2Hood mode");
      // set to initial state
      ventilation = 0;
      light = 0;
      controlHood();

      // and switch to hob2hood mode
      mode = MODE_HOB2HOOD;
      irrecv.resume();
    }
    receiveIRCommand();
  }

}

// Receive and decode IR commands and control hood upon received command
void receiveIRCommand() {

  // have we received an IR signal?
  if (irrecv.decode(&results)) {

    Serial.println("Received IR command: ");
    Serial.println(results.value, HEX); // display it on serial monitor in hexadecimal

    switch (results.value) {

      case IRCMD_LIGHT_ON:
        light = 1;
        break;

      case IRCMD_LIGHT_OFF:
        light = 0;
        break;

      case IRCMD_VENT_1:
        ventilation = 1;
        break;

      case IRCMD_VENT_2:
        ventilation = 2;
        break;

      case IRCMD_VENT_3:
        ventilation = 3;
        break;

      case IRCMD_VENT_4:
        ventilation = 4;
        break;

      case IRCMD_VENT_OFF:
        ventilation = 0;
        break;

      default:
        break;
    }

    controlHood();
    irrecv.resume(); // receive the next value
  }
}

// control hood based on 'light' and 'ventilation' variables
void controlHood() {

  bool logLight = light!=last_light;
  bool logVent = ventilation!=last_ventilation;
  

  // control light
  switch (light) {
    // Light OFF
    case 0:
      if (logLight) Serial.println("Light: OFF");
      digitalWrite(PIN_OUT_LIGHT, OFF);
      delay(10);
      break;
    // Light ON
    case 1:
      if (logLight) Serial.println("Light: ON");
      digitalWrite(PIN_OUT_LIGHT, ON);
      delay(10);
      break;
    default:
      break;
  }

  // control ventilation
  switch (ventilation) {

    // Ventilation OFF
    case 0:
      if (logVent) Serial.println("Ventilation: OFF");
      digitalWrite(PIN_OUT_VENT_1, OFF);
      digitalWrite(PIN_OUT_VENT_2, OFF);
      digitalWrite(PIN_OUT_VENT_3, OFF);
      digitalWrite(PIN_OUT_VENT_4, OFF);
      delay(10);
      break;

    // Ventilation Speed 1
    case 1:
      if (logVent) Serial.println("Ventilation: 1");
      digitalWrite(PIN_OUT_VENT_2, OFF);
      digitalWrite(PIN_OUT_VENT_3, OFF);
      digitalWrite(PIN_OUT_VENT_4, OFF);
      delay(100);
      digitalWrite(PIN_OUT_VENT_1, ON);
      delay(10);

      delay(10);
      break;

    // Ventilation Speed 2
    case 2:
      if (logVent) Serial.println("Ventilation: 2");
      digitalWrite(PIN_OUT_VENT_1, OFF);
      digitalWrite(PIN_OUT_VENT_3, OFF);
      digitalWrite(PIN_OUT_VENT_4, OFF);
      delay(100);
      digitalWrite(PIN_OUT_VENT_2, ON);
      delay(10);
      break;

    // Ventilation Speed 3
    case 3:
      if (logVent) Serial.println("Ventilation: 3");
      digitalWrite(PIN_OUT_VENT_1, OFF);
      digitalWrite(PIN_OUT_VENT_2, OFF);
      digitalWrite(PIN_OUT_VENT_4, OFF);
      delay(100);
      digitalWrite(PIN_OUT_VENT_3, ON);
      delay(10);
      break;

    // Ventilation Speed 4
    case 4:
      if (logVent) Serial.println("Ventilation: 4");
      digitalWrite(PIN_OUT_VENT_1, OFF);
      digitalWrite(PIN_OUT_VENT_2, OFF);
      digitalWrite(PIN_OUT_VENT_3, OFF);
      delay(100);
      digitalWrite(PIN_OUT_VENT_4, ON);
      delay(10);
      break;

    default:
      break;

  }
  
  last_light = light;
  last_ventilation = ventilation;
  
}


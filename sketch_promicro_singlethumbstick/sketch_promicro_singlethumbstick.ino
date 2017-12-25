/* Elliott's Extra Analog Thumbstick on a Warthog Throttle
 * Using a SparkFun Arduino Pro Micro 5V / 16MHz
 * 
 * This code on Github at:
 *   https://github.com/ehzastrow/ThrottleJoy.git
 * 
 * Windows INF device driver files for this Arduino at:
 *   https://github.com/sparkfun/Arduino_Boards/archive/master.zip
 * 
 * To get the Arduino Pro Micro working with AVR IDE, follow these instructions:
 *   https://github.com/sparkfun/Arduino_Boards
 * 
 * In Arduino IDE select: 
 *   Tools --> Board: "Arduino AVR Boards --> SparkFun Pro Micro"
 *   Tools --> Processor: "ATMega 32U4 (5V, 16MHz)"
 *
 * Requires MHeironimus's ArduinoJoystickLibrary from:
 *   https://github.com/MHeironimus/ArduinoJoystickLibrary
*/

#include <Joystick.h>

const bool DEBUG = false;  // set to true to debug the raw values
const int ADC_MIN = 0;
const int ADC_MAX = 1023;

// subtract 1/AXIS_LEARNING_RATIO of range from axis maximum/minimum when initially teaching the stick what it's full range is
// try values 2 to 8
const int AXIS_LEARNING_RATIO = 4;

// Probably best keep this small, mostly can be done in joystick control software
const int DEAD_ZONE = 2;

// Set to true to test "Auto Send" mode or false to test "Manual Send" mode.
const bool AUTO_SEND_MODE = true;
//const bool AUTO_SEND_MODE = false;

// Create the Joystick
Joystick_ joySt(JOYSTICK_DEFAULT_REPORT_ID, 
  JOYSTICK_TYPE_JOYSTICK, 1, 0,
  true, true, false, false, false, false,
  false, false, false, false, false);

// Thumbstick
int thumbStick_xPin = A0;
int thumbStick_yPin = A1;
int thumbStick_bPin = A10;
int thumbStick_xMin, thumbStick_yMin;
int thumbStick_xMax, thumbStick_yMax;
int thumbStick_xZero, thumbStick_yZero;
int thumbStick_xValue, thumbStick_yValue;

byte thumbStick_bState = HIGH;
byte thumbStick_bLastState = HIGH;
long thumbStick_bLastDebounceTime = 0;
long thumbStick_bDebounceDelay = 10;

void setup() {
  // put your setup code here, to run once:
  pinMode(thumbStick_xPin, INPUT);
  pinMode(thumbStick_yPin, INPUT);
  pinMode(thumbStick_bPin, INPUT_PULLUP);

  if(DEBUG) {
    Serial.begin(9600);
  }

  // Calculate neutral position as an average of 16 readings
  thumbStick_xZero = 0;
  thumbStick_yZero = 0;

  for (int i = 0; i < 16; i++) {
    thumbStick_xZero += analogRead(thumbStick_xPin);
    thumbStick_yZero += analogRead(thumbStick_yPin);
    delay(10);
  }
  thumbStick_xZero = thumbStick_xZero / 16;
  thumbStick_yZero = thumbStick_yZero / 16;

  if(DEBUG) {
    Serial.print("Thumbstick X Zero: ");
    Serial.println(thumbStick_xZero);
    Serial.print("Thumbstick Y Zero: ");
    Serial.println(thumbStick_yZero);
  }

  // start out assuming our min and max values aren't actually the full range possible... will update these values as we see otherwise
  int initial_minimum = ADC_MIN + ((ADC_MIN + ADC_MAX) / AXIS_LEARNING_RATIO);
  int initial_maximum = ADC_MAX - ((ADC_MIN + ADC_MAX) / AXIS_LEARNING_RATIO);

  thumbStick_xMin = initial_minimum;
  thumbStick_xMax = initial_maximum;
  thumbStick_yMin = initial_minimum;
  thumbStick_yMax = initial_maximum;
  
  joySt.begin(AUTO_SEND_MODE);
}

void loop() {
  // put your main code here, to run repeatedly:
  
  int thumbStick_xReading = analogRead(thumbStick_xPin);
  int thumbStick_yReading = analogRead(thumbStick_yPin);

  // update our expected minimums and maximums
  thumbStick_xMin = min(thumbStick_xMin, thumbStick_xReading);
  thumbStick_yMin = min(thumbStick_yMin, thumbStick_yReading);
  thumbStick_xMax = max(thumbStick_xMax, thumbStick_xReading);
  thumbStick_yMax = max(thumbStick_yMax, thumbStick_yReading);

  thumbStick_xValue = thumbStick_xReading - thumbStick_xZero;
  thumbStick_yValue = thumbStick_yReading - thumbStick_yZero;

  //circular dead zone
  if ( (pow((float)thumbStick_xValue, 2) + pow((float)thumbStick_yValue, 2)) < pow((float)DEAD_ZONE, 2) ) {
    thumbStick_xValue = 0;
    thumbStick_yValue = 0; 
  }

  // read button value
  byte button_0_reading = digitalRead(thumbStick_bPin);

  // button debounce
  if (button_0_reading != thumbStick_bLastState) {
    thumbStick_bLastDebounceTime = millis(); 
  }

  if ((millis() - thumbStick_bLastDebounceTime) > thumbStick_bDebounceDelay) {
    if (button_0_reading != thumbStick_bState) {
      thumbStick_bState = button_0_reading; 
    }
  }

  thumbStick_bLastState = button_0_reading;
  if (thumbStick_bState == HIGH) {
    joySt.releaseButton(0);
  } else {
    joySt.pressButton(0); 
  }

  joySt.setXAxis(map(thumbStick_xValue, (thumbStick_xMin - thumbStick_xZero), (thumbStick_xMax - thumbStick_xZero), 0, 1023));
  joySt.setYAxis(map(thumbStick_yValue, (thumbStick_yMin - thumbStick_yZero), (thumbStick_yMax - thumbStick_yZero), 0, 1023));

  if(DEBUG) {
    Serial.print("X: ");
    Serial.println(thumbStick_xValue);
    Serial.print("Y: ");
    Serial.println(thumbStick_yValue);
  }

  // Update state of Joystick (not necessary unless Joystick is constructed with AutoSendState = false)
  if (!AUTO_SEND_MODE) {
    joySt.sendState();
  }
}

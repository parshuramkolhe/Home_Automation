/*
  ============================================================
             ESP32 SMART HOME AUTOMATION
  ============================================================

  CONTROL:
  1. Bluetooth - BT Home App
  2. VC02 Voice
  3. IR Remote
  4. Manual Push Buttons

  OUTPUT:
  4 Lights + 1 Fan

  SENSOR:
  DHT11

  IMPORTANT:
  Bluetooth ON/OFF commands are ABSOLUTE.
  They do NOT use toggle logic.

  ============================================================
  BLUETOOTH COMMANDS
  ============================================================

  LIGHT 1:
    81 00 01 -> ON
    81 00 00 -> OFF

  LIGHT 2:
    82 00 01 -> ON
    82 00 00 -> OFF

  LIGHT 3:
    83 00 01 -> ON
    83 00 00 -> OFF

  LIGHT 4:
    84 00 01 -> ON
    84 00 00 -> OFF

  FAN:
    85 00 00 -> OFF
    85 00 01 -> ON / FULL SPEED
    85 00 02 -> SPEED 1
    85 00 03 -> SPEED 2
    85 00 04 -> SPEED 3
    85 00 05 -> SPEED 4

  ============================================================
*/

#include <BluetoothSerial.h>
#include <IRremote.hpp>
#include <DHT.h>


// ============================================================
// BLUETOOTH
// ============================================================

BluetoothSerial SerialBT;


// ============================================================
// VC02
// ============================================================

HardwareSerial VC02(2);

#define VC02_RX 16
#define VC02_TX 17


// ============================================================
// IR
// ============================================================

#define IR_PIN 12


// ============================================================
// LIGHT OUTPUTS
// ============================================================

#define LIGHT1_PIN 25
#define LIGHT2_PIN 26
#define LIGHT3_PIN 27
#define LIGHT4_PIN 14


// ============================================================
// FAN
// ============================================================

#define FAN_DIMMER_PIN 33


// ============================================================
// MANUAL BUTTONS
// Button -> GPIO -> GND
// ============================================================

#define BUTTON_LIGHT1 18
#define BUTTON_LIGHT2 19
#define BUTTON_LIGHT3 21
#define BUTTON_LIGHT4 22
#define BUTTON_FAN    23


// ============================================================
// DHT11
// ============================================================

#define DHT_PIN 13
#define DHT_TYPE DHT11

DHT dht(DHT_PIN, DHT_TYPE);

float temperature = 0;
float humidity = 0;

unsigned long lastDHTRead = 0;

#define DHT_INTERVAL 2000


// ============================================================
// DEVICE STATES
// ============================================================

bool light1State = false;
bool light2State = false;
bool light3State = false;
bool light4State = false;


// Fan:
// 0 = OFF
// 1 = Speed 1
// 2 = Speed 2
// 3 = Speed 3
// 4 = Speed 4 / FULL SPEED

int fanSpeed = 0;


// ============================================================
// BLUETOOTH PACKET BUFFER
// ============================================================

uint8_t packet[3];
uint8_t packetIndex = 0;


// ============================================================
// VC02 COMMANDS
//
// These values are from your current code.
// If your VC02 training gives different values,
// replace them with the actual values.
// ============================================================

#define VC02_L1_ON       0xA0
#define VC02_L1_OFF      0xB0

#define VC02_L2_ON       0xB1
#define VC02_L2_OFF      0xC0

#define VC02_L3_ON       0xC1
#define VC02_L3_OFF      0xD0

#define VC02_L4_ON       0xD1
#define VC02_L4_OFF      0xFA

#define VC02_ALL_ON      0xB2
#define VC02_ALL_OFF     0xF9

#define VC02_FAN_OFF     0xF1
#define VC02_FAN_ON      0xF0

#define VC02_FAN_S1      0xF2
#define VC02_FAN_S2      0xF3
#define VC02_FAN_S3      0xFF
#define VC02_FAN_S4      0xA2


// ============================================================
// IR COMMANDS
//
// These are the values currently present in your code.
// Replace ONLY with the actual Command values from
// your IR remote testing.
// ============================================================

#define IR_L1_TOGGLE     0x0A
#define IR_L2_TOGGLE     0x1B
#define IR_L3_TOGGLE     0x1F
#define IR_L4_TOGGLE     0x0C

#define IR_ALL_ON        0x01
#define IR_ALL_OFF       0x12

#define IR_FAN_OFF       0x03
#define IR_FAN_ON        0x02

#define IR_FAN_S1        0x0D
#define IR_FAN_S2        0x0E
#define IR_FAN_S3        0x0F
#define IR_FAN_S4        0x19


// ============================================================
// BLUETOOTH STATE SYNC
// ============================================================

// Send light state to BT Home app
// Used ONLY when another control source changes the state.

void sendLightState(uint8_t command, bool state)
{
  if (!SerialBT.hasClient())
    return;

  uint8_t data[3];

  data[0] = command;
  data[1] = 0x00;
  data[2] = state ? 0x01 : 0x00;

  SerialBT.write(data, 3);
}


// ============================================================
// SEND FAN STATE
// ============================================================
//
// reportFullAsOn:
// true  -> send 85 00 01
// false -> send 85 00 05 for speed 4
//
// This keeps FAN ON and SPEED 4 separate in the app.
// ============================================================

void sendFanState(int speed, bool reportFullAsOn)
{
  if (!SerialBT.hasClient())
    return;

  uint8_t data[3];

  data[0] = 0x85;
  data[1] = 0x00;

  if (speed == 0)
  {
    data[2] = 0x00;
  }
  else if (speed == 4 && reportFullAsOn)
  {
    data[2] = 0x01;
  }
  else if (speed == 4)
  {
    data[2] = 0x05;
  }
  else
  {
    data[2] = speed + 1;
  }

  SerialBT.write(data, 3);
}


// ============================================================
// LIGHT 1
// ============================================================

void light1On(bool syncBluetooth)
{
  digitalWrite(LIGHT1_PIN, HIGH);
  light1State = true;

  if (syncBluetooth)
    sendLightState(0x81, true);
}


void light1Off(bool syncBluetooth)
{
  digitalWrite(LIGHT1_PIN, LOW);
  light1State = false;

  if (syncBluetooth)
    sendLightState(0x81, false);
}


void light1Toggle()
{
  if (light1State)
    light1Off(true);
  else
    light1On(true);
}


// ============================================================
// LIGHT 2
// ============================================================

void light2On(bool syncBluetooth)
{
  digitalWrite(LIGHT2_PIN, HIGH);
  light2State = true;

  if (syncBluetooth)
    sendLightState(0x82, true);
}


void light2Off(bool syncBluetooth)
{
  digitalWrite(LIGHT2_PIN, LOW);
  light2State = false;

  if (syncBluetooth)
    sendLightState(0x82, false);
}


void light2Toggle()
{
  if (light2State)
    light2Off(true);
  else
    light2On(true);
}


// ============================================================
// LIGHT 3
// ============================================================

void light3On(bool syncBluetooth)
{
  digitalWrite(LIGHT3_PIN, HIGH);
  light3State = true;

  if (syncBluetooth)
    sendLightState(0x83, true);
}


void light3Off(bool syncBluetooth)
{
  digitalWrite(LIGHT3_PIN, LOW);
  light3State = false;

  if (syncBluetooth)
    sendLightState(0x83, false);
}


void light3Toggle()
{
  if (light3State)
    light3Off(true);
  else
    light3On(true);
}


// ============================================================
// LIGHT 4
// ============================================================

void light4On(bool syncBluetooth)
{
  digitalWrite(LIGHT4_PIN, HIGH);
  light4State = true;

  if (syncBluetooth)
    sendLightState(0x84, true);
}


void light4Off(bool syncBluetooth)
{
  digitalWrite(LIGHT4_PIN, LOW);
  light4State = false;

  if (syncBluetooth)
    sendLightState(0x84, false);
}


void light4Toggle()
{
  if (light4State)
    light4Off(true);
  else
    light4On(true);
}


// ============================================================
// ALL LIGHTS ON
// ============================================================

void allLightsOn(bool syncBluetooth)
{
  light1On(syncBluetooth);
  light2On(syncBluetooth);
  light3On(syncBluetooth);
  light4On(syncBluetooth);
}


// ============================================================
// ALL LIGHTS OFF
// ============================================================

void allLightsOff(bool syncBluetooth)
{
  light1Off(syncBluetooth);
  light2Off(syncBluetooth);
  light3Off(syncBluetooth);
  light4Off(syncBluetooth);
}


// ============================================================
// FAN CONTROL
// ============================================================
//
// speed:
// 0 = OFF
// 1 = SPEED 1
// 2 = SPEED 2
// 3 = SPEED 3
// 4 = FULL SPEED
//
// syncBluetooth:
// true  -> send state to app
// false -> do not send feedback
//
// reportFullAsOn:
// true when command means "FAN ON"
// false when command means "SPEED 4"
// ============================================================

void setFanSpeed(int speed,
                 bool syncBluetooth,
                 bool reportFullAsOn)
{
  if (speed < 0)
    speed = 0;

  if (speed > 4)
    speed = 4;

  fanSpeed = speed;


  // ----------------------------------------------------------
  // FAN OFF
  // ----------------------------------------------------------

  if (speed == 0)
  {
    digitalWrite(FAN_DIMMER_PIN, LOW);

    Serial.println("FAN: OFF");
  }


  // ----------------------------------------------------------
  // FAN ON / SPEED
  // ----------------------------------------------------------

  else
  {
    digitalWrite(FAN_DIMMER_PIN, HIGH);

    if (speed == 4)
    {
      if (reportFullAsOn)
        Serial.println("FAN: ON - FULL SPEED");
      else
        Serial.println("FAN: SPEED 4");
    }
    else
    {
      Serial.print("FAN: SPEED ");
      Serial.println(speed);
    }
  }


  // ----------------------------------------------------------
  // Bluetooth state update
  // ----------------------------------------------------------

  if (syncBluetooth)
    sendFanState(speed, reportFullAsOn);
}


// ============================================================
// DHT11
// ============================================================

void readDHT11()
{
  if (millis() - lastDHTRead < DHT_INTERVAL)
    return;

  lastDHTRead = millis();


  float h = dht.readHumidity();
  float t = dht.readTemperature();


  if (isnan(h) || isnan(t))
  {
    Serial.println("DHT11 read failed!");
    return;
  }


  temperature = t;
  humidity = h;


  Serial.print("Temperature: ");
  Serial.print(temperature, 1);

  Serial.print(" C | Humidity: ");
  Serial.print(humidity, 1);

  Serial.println(" %");


  // ----------------------------------------------------------
  // DHT DATA TO BLUETOOTH
  //
  // Current format from your code:
  // TEMP:xx.x,HUM:xx.x
  //
  // Exact BT Home binary sensor protocol is not confirmed.
  // ----------------------------------------------------------

  if (SerialBT.hasClient())
  {
    SerialBT.print("TEMP:");
    SerialBT.print(temperature, 1);

    SerialBT.print(",HUM:");
    SerialBT.print(humidity, 1);

    SerialBT.print("\r\n");
  }
}


// ============================================================
// CHECK VALID BLUETOOTH COMMAND
// ============================================================

bool isValidBTCommand(uint8_t command)
{
  if (command == 0x81)
    return true;

  if (command == 0x82)
    return true;

  if (command == 0x83)
    return true;

  if (command == 0x84)
    return true;

  if (command == 0x85)
    return true;

  return false;
}


// ============================================================
// BLUETOOTH COMMAND PROCESSING
// ============================================================
//
// IMPORTANT:
// Bluetooth commands DO NOT call the toggle functions.
//
// 81 00 01 -> ON
// 81 00 00 -> OFF
//
// This is the main correction.
// ============================================================

void processBluetooth()
{
  while (SerialBT.available())
  {
    uint8_t incoming = SerialBT.read();


    // --------------------------------------------------------
    // First byte must be a valid command
    // --------------------------------------------------------

    if (packetIndex == 0)
    {
      if (!isValidBTCommand(incoming))
      {
        // Ignore unknown byte
        continue;
      }
    }


    packet[packetIndex] = incoming;
    packetIndex++;


    // --------------------------------------------------------
    // Complete 3-byte packet
    // --------------------------------------------------------

    if (packetIndex >= 3)
    {
      uint8_t command = packet[0];
      uint8_t middle  = packet[1];
      uint8_t value   = packet[2];


      Serial.print("BT COMMAND: ");
      Serial.print(command, HEX);

      Serial.print(" ");
      Serial.print(middle, HEX);

      Serial.print(" ");
      Serial.println(value, HEX);


      // ======================================================
      // Only accept 00 as middle byte
      // ======================================================

      if (middle == 0x00)
      {

        // ====================================================
        // LIGHT 1
        // ====================================================

        if (command == 0x81)
        {
          if (value == 0x01)
          {
            // ABSOLUTE ON
            light1On(false);
          }
          else if (value == 0x00)
          {
            // ABSOLUTE OFF
            light1Off(false);
          }
        }


        // ====================================================
        // LIGHT 2
        // ====================================================

        else if (command == 0x82)
        {
          if (value == 0x01)
          {
            // ABSOLUTE ON
            light2On(false);
          }
          else if (value == 0x00)
          {
            // ABSOLUTE OFF
            light2Off(false);
          }
        }


        // ====================================================
        // LIGHT 3
        // ====================================================

        else if (command == 0x83)
        {
          if (value == 0x01)
          {
            // ABSOLUTE ON
            light3On(false);
          }
          else if (value == 0x00)
          {
            // ABSOLUTE OFF
            light3Off(false);
          }
        }


        // ====================================================
        // LIGHT 4
        // ====================================================

        else if (command == 0x84)
        {
          if (value == 0x01)
          {
            // ABSOLUTE ON
            light4On(false);
          }
          else if (value == 0x00)
          {
            // ABSOLUTE OFF
            light4Off(false);
          }
        }


        // ====================================================
        // FAN
        // ====================================================

        else if (command == 0x85)
        {
          // -----------------------------------------------
          // 00 = OFF
          // -----------------------------------------------

          if (value == 0x00)
          {
            setFanSpeed(0, false, false);
          }


          // -----------------------------------------------
          // 01 = ON / FULL SPEED
          // -----------------------------------------------

          else if (value == 0x01)
          {
            setFanSpeed(4, false, true);
          }


          // -----------------------------------------------
          // 02 = SPEED 1
          // -----------------------------------------------

          else if (value == 0x02)
          {
            setFanSpeed(1, false, false);
          }


          // -----------------------------------------------
          // 03 = SPEED 2
          // -----------------------------------------------

          else if (value == 0x03)
          {
            setFanSpeed(2, false, false);
          }


          // -----------------------------------------------
          // 04 = SPEED 3
          // -----------------------------------------------

          else if (value == 0x04)
          {
            setFanSpeed(3, false, false);
          }


          // -----------------------------------------------
          // 05 = SPEED 4
          // -----------------------------------------------

          else if (value == 0x05)
          {
            setFanSpeed(4, false, false);
          }
        }
      }


      // ------------------------------------------------------
      // RESET PACKET
      // ------------------------------------------------------

      packetIndex = 0;
    }
  }
}


// ============================================================
// VC02 VOICE
// ============================================================

void processVoice()
{
  if (!VC02.available())
    return;


  uint8_t cmd = VC02.read();


  Serial.print("VOICE: 0x");
  Serial.println(cmd, HEX);


  // ----------------------------------------------------------
  // LIGHT 1
  // ----------------------------------------------------------

  if (cmd == VC02_L1_ON)
    light1On(true);

  else if (cmd == VC02_L1_OFF)
    light1Off(true);


  // ----------------------------------------------------------
  // LIGHT 2
  // ----------------------------------------------------------

  else if (cmd == VC02_L2_ON)
    light2On(true);

  else if (cmd == VC02_L2_OFF)
    light2Off(true);


  // ----------------------------------------------------------
  // LIGHT 3
  // ----------------------------------------------------------

  else if (cmd == VC02_L3_ON)
    light3On(true);

  else if (cmd == VC02_L3_OFF)
    light3Off(true);


  // ----------------------------------------------------------
  // LIGHT 4
  // ----------------------------------------------------------

  else if (cmd == VC02_L4_ON)
    light4On(true);

  else if (cmd == VC02_L4_OFF)
    light4Off(true);


  // ----------------------------------------------------------
  // ALL LIGHTS
  // ----------------------------------------------------------

  else if (cmd == VC02_ALL_ON)
    allLightsOn(true);

  else if (cmd == VC02_ALL_OFF)
    allLightsOff(true);


  // ----------------------------------------------------------
  // FAN OFF
  // ----------------------------------------------------------

  else if (cmd == VC02_FAN_OFF)
    setFanSpeed(0, true, false);


  // ----------------------------------------------------------
  // FAN ON = FULL SPEED
  // ----------------------------------------------------------

  else if (cmd == VC02_FAN_ON)
    setFanSpeed(4, true, true);


  // ----------------------------------------------------------
  // FAN SPEED 1
  // ----------------------------------------------------------

  else if (cmd == VC02_FAN_S1)
    setFanSpeed(1, true, false);


  // ----------------------------------------------------------
  // FAN SPEED 2
  // ----------------------------------------------------------

  else if (cmd == VC02_FAN_S2)
    setFanSpeed(2, true, false);


  // ----------------------------------------------------------
  // FAN SPEED 3
  // ----------------------------------------------------------

  else if (cmd == VC02_FAN_S3)
    setFanSpeed(3, true, false);


  // ----------------------------------------------------------
  // FAN SPEED 4
  // ----------------------------------------------------------

  else if (cmd == VC02_FAN_S4)
    setFanSpeed(4, true, false);
}


// ============================================================
// IR
// ============================================================

void processIR()
{
  if (!IrReceiver.decode())
    return;


  // ----------------------------------------------------------
  // Ignore repeat frames
  // ----------------------------------------------------------

  if (IrReceiver.decodedIRData.flags & IRDATA_FLAGS_IS_REPEAT)
  {
    IrReceiver.resume();
    return;
  }


  uint16_t command = IrReceiver.decodedIRData.command;


  Serial.print("IR COMMAND: 0x");
  Serial.println(command, HEX);


  // ----------------------------------------------------------
  // LIGHTS
  // ----------------------------------------------------------

  if (command == IR_L1_TOGGLE)
    light1Toggle();

  else if (command == IR_L2_TOGGLE)
    light2Toggle();

  else if (command == IR_L3_TOGGLE)
    light3Toggle();

  else if (command == IR_L4_TOGGLE)
    light4Toggle();


  // ----------------------------------------------------------
  // ALL LIGHTS
  // ----------------------------------------------------------

  else if (command == IR_ALL_ON)
    allLightsOn(true);

  else if (command == IR_ALL_OFF)
    allLightsOff(true);


  // ----------------------------------------------------------
  // FAN OFF
  // ----------------------------------------------------------

  else if (command == IR_FAN_OFF)
    setFanSpeed(0, true, false);


  // ----------------------------------------------------------
  // FAN ON = FULL SPEED
  // ----------------------------------------------------------

  else if (command == IR_FAN_ON)
    setFanSpeed(4, true, true);


  // ----------------------------------------------------------
  // FAN SPEEDS
  // ----------------------------------------------------------

  else if (command == IR_FAN_S1)
    setFanSpeed(1, true, false);

  else if (command == IR_FAN_S2)
    setFanSpeed(2, true, false);

  else if (command == IR_FAN_S3)
    setFanSpeed(3, true, false);

  else if (command == IR_FAN_S4)
    setFanSpeed(4, true, false);


  // ----------------------------------------------------------
  // READY FOR NEXT IR COMMAND
  // ----------------------------------------------------------

  IrReceiver.resume();
}


// ============================================================
// MANUAL BUTTONS
// ============================================================

void processManualButtons()
{
  static bool lastL1 = HIGH;
  static bool lastL2 = HIGH;
  static bool lastL3 = HIGH;
  static bool lastL4 = HIGH;
  static bool lastFan = HIGH;


  bool currentL1 = digitalRead(BUTTON_LIGHT1);
  bool currentL2 = digitalRead(BUTTON_LIGHT2);
  bool currentL3 = digitalRead(BUTTON_LIGHT3);
  bool currentL4 = digitalRead(BUTTON_LIGHT4);
  bool currentFan = digitalRead(BUTTON_FAN);


  // ----------------------------------------------------------
  // LIGHT 1
  // ----------------------------------------------------------

  if (lastL1 == HIGH && currentL1 == LOW)
  {
    light1Toggle();
    delay(30);
  }


  // ----------------------------------------------------------
  // LIGHT 2
  // ----------------------------------------------------------

  if (lastL2 == HIGH && currentL2 == LOW)
  {
    light2Toggle();
    delay(30);
  }


  // ----------------------------------------------------------
  // LIGHT 3
  // ----------------------------------------------------------

  if (lastL3 == HIGH && currentL3 == LOW)
  {
    light3Toggle();
    delay(30);
  }


  // ----------------------------------------------------------
  // LIGHT 4
  // ----------------------------------------------------------

  if (lastL4 == HIGH && currentL4 == LOW)
  {
    light4Toggle();
    delay(30);
  }


  // ----------------------------------------------------------
  // FAN BUTTON
  //
  // OFF -> FULL SPEED
  // ON  -> OFF
  // ----------------------------------------------------------

  if (lastFan == HIGH && currentFan == LOW)
  {
    if (fanSpeed == 0)
    {
      setFanSpeed(4, true, true);
    }
    else
    {
      setFanSpeed(0, true, false);
    }

    delay(30);
  }


  lastL1 = currentL1;
  lastL2 = currentL2;
  lastL3 = currentL3;
  lastL4 = currentL4;
  lastFan = currentFan;
}


// ============================================================
// SETUP
// ============================================================

void setup()
{
  Serial.begin(115200);


  // ==========================================================
  // OUTPUTS
  // ==========================================================

  pinMode(LIGHT1_PIN, OUTPUT);
  pinMode(LIGHT2_PIN, OUTPUT);
  pinMode(LIGHT3_PIN, OUTPUT);
  pinMode(LIGHT4_PIN, OUTPUT);

  pinMode(FAN_DIMMER_PIN, OUTPUT);


  // ==========================================================
  // INITIAL OFF
  // ==========================================================

  digitalWrite(LIGHT1_PIN, LOW);
  digitalWrite(LIGHT2_PIN, LOW);
  digitalWrite(LIGHT3_PIN, LOW);
  digitalWrite(LIGHT4_PIN, LOW);

  digitalWrite(FAN_DIMMER_PIN, LOW);


  // ==========================================================
  // MANUAL BUTTONS
  // ==========================================================

  pinMode(BUTTON_LIGHT1, INPUT_PULLUP);
  pinMode(BUTTON_LIGHT2, INPUT_PULLUP);
  pinMode(BUTTON_LIGHT3, INPUT_PULLUP);
  pinMode(BUTTON_LIGHT4, INPUT_PULLUP);
  pinMode(BUTTON_FAN, INPUT_PULLUP);


  // ==========================================================
  // VC02
  // ==========================================================

  VC02.begin(
    9600,
    SERIAL_8N1,
    VC02_RX,
    VC02_TX
  );


  // ==========================================================
  // BLUETOOTH
  // ==========================================================

  SerialBT.begin("ESP32_Home");


  // ==========================================================
  // IR
  // ==========================================================

  IrReceiver.begin(
    IR_PIN,
    ENABLE_LED_FEEDBACK
  );


  // ==========================================================
  // DHT11
  // ==========================================================

  dht.begin();


  // ==========================================================
  // START MESSAGE
  // ==========================================================

  Serial.println();
  Serial.println("==================================");
  Serial.println("      ESP32 SMART HOME READY");
  Serial.println("==================================");

  Serial.println("Bluetooth : ESP32_Home");
  Serial.println("VC02      : Ready");
  Serial.println("IR        : GPIO12");
  Serial.println("Manual    : Ready");
  Serial.println("DHT11     : GPIO13");
  Serial.println("Fan ON    : FULL SPEED");
  Serial.println("==================================");
}


// ============================================================
// LOOP
// ============================================================

void loop()
{
  // Bluetooth
  processBluetooth();


  // Voice
  processVoice();


  // IR
  processIR();


  // Manual buttons
  processManualButtons();


  // DHT11
  readDHT11();
}
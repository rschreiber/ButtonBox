#include <Arduino.h>
#include <Wire.h>
#include <Keyboard.h>

// Define I2C slave addresses
#define NANO_1_ADDRESS 0x04
#define NANO_2_ADDRESS 0x05

#define EMPTY_BUFFER_PLACEHOLDER 0xFF

const int nanoAddresses[] = {NANO_1_ADDRESS, NANO_2_ADDRESS};

// Define key mappings for each Nano
const char keyMappings[2][16] = {
    {'A', 'B', 'C', 'D', 'E', '-', '-', '-', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M'},
    {'N', 'O', 'P', 'Q', 'R', '-', '-', '-', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z'}};

// Debugging
#define DEBUG 1
#if DEBUG
#define DebugPrint(x) Serial.print(x)
#define DebugPrintln(x) Serial.println(x)
#else
#define DebugPrint(x)
#define DebugPrintln(x)
#endif



void setup()
{
  Serial.begin(115200);
  Wire.begin(); // Initialize I2C as master
  Keyboard.begin();
  delay(3000); // Delay for 3 seconds to allow Nano to start up
  DebugPrintln("STR host controller started");
}

// Function prototypes
void pollNano(int nanoIndex);
char mapCommandToKey(int nanoIndex, uint8_t command);

bool isTimeToRun(unsigned long &lastRun, unsigned long interval)
{
  if (millis() - lastRun >= interval)
  {
    lastRun = millis();
    return true;
  }
  return false;
}



void loop()
{
  static unsigned long lastPollTime = 0;
  const unsigned long pollingInterval = 50;

  // Poll Nanos at a regular interval
  if (isTimeToRun(lastPollTime, pollingInterval))
  {
    pollNano(0);
    pollNano(1);
  }

  // count how many loops have run and write this out every 30 seconds
  static unsigned long lastPrintTime = 0;
  const unsigned long printInterval = 30000;
  static unsigned long loopCount = 0;
  loopCount++;
  if (isTimeToRun(lastPrintTime, printInterval))
  {
    DebugPrint("Loop count: ");
    DebugPrintln(loopCount);
    loopCount = 0;
  }
}

// Poll a Nano for commands
void pollNano(int nanoIndex)
{
  int nanoAddress = nanoAddresses[nanoIndex];
  Wire.requestFrom(nanoAddress, 1); // Request 1 byte from the Nano

  if (Wire.available())
  {

    int command = Wire.read(); // Read the command byte

    if (command != EMPTY_BUFFER_PLACEHOLDER)
    {                                             // Ignore placeholder (empty buffer)
      int commandType = (command & 0x80) ? 1 : 0; // Extract command type (bit 7)
      int controllerID = command & 0x3F;          // Extract controller ID (bits 0–5)

      DebugPrint("Nano ");
      DebugPrint(nanoIndex + 1);
      DebugPrint(": Command Type: ");
      DebugPrint(commandType == 0 ? "PRESS" : "RELEASE");
      DebugPrint(", Controller ID: ");
      DebugPrintln(controllerID);

      char key = mapCommandToKey(nanoIndex, controllerID);
      if (key != 0)
      {
        if (commandType == 0)
        { // PRESS
          // Keyboard.press(key);
          DebugPrint("Key Pressed: ");
        }
        else
        { // RELEASE
          // Keyboard.release(key);
          DebugPrint("Key Released: ");
        }
        DebugPrintln(key);
      }
      else
      {
        DebugPrintln("Invalid Controller ID");
      }
    }
  }
}

// Map a command to a key based on Nano and controller ID
char mapCommandToKey(int nanoIndex, uint8_t controllerID)
{
  if (controllerID < 16)
  { // Ensure valid controller ID
    return keyMappings[nanoIndex][controllerID];
  }
  return 0; // Return 0 for invalid ID
}

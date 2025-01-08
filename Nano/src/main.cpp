#include <Arduino.h>
#include <Wire.h>
#include <Encoder.h>

// Button definitions
#define BUTTON1 2
#define BUTTON2 3
#define BUTTON3 4
#define BUTTON4 5
#define BUTTON5 6

#define BUTTON_COUNT 5

int buttons[] = {BUTTON1, BUTTON2, BUTTON3, BUTTON4, BUTTON5};
int buttonsState[] = {HIGH, HIGH, HIGH, HIGH, HIGH};
int buttonsLastState[] = {HIGH, HIGH, HIGH, HIGH, HIGH};
unsigned long lastDebounceTime[BUTTON_COUNT] = {0, 0, 0, 0, 0};

// Encoder definitions for debugging with a single encoder
// #define RC1_SWITCH 10
// #define RC1_CLK 11
// #define RC1_DATA 12

// #define RC2_SWITCH 7
// #define RC2_CLK 8
// #define RC2_DATA 9

#define RC1_SWITCH 7
#define RC1_CLK 8
#define RC1_DATA 9

#define RC2_SWITCH 10
#define RC2_CLK 11
#define RC2_DATA 12

#define ENCODER_COUNT 2

int encoderSwitch[] = {RC1_SWITCH, RC2_SWITCH};
int encoderCLK[] = {RC1_CLK, RC2_CLK};
int encoderDATA[] = {RC1_DATA, RC2_DATA};

bool encoderMode[] = {false, false}; // Track mode for each encoder
int encoderLastSwitchState[] = {HIGH, HIGH};
unsigned long encoderLastDebounceTime[] = {0, 0};

#define BUTTON_RELEASED HIGH
#define BUTTON_PRESSED LOW

// Define the pin for address selection
#define ADDRESS_PIN A3 // Use an available pin (analog or digital)

// Define possible I2C addresses
#define ADDRESS_1 0x04 // Address when switch is LOW
#define ADDRESS_2 0x05 // Address when switch is HIGH

int i2cAddress; // Variable to store the chosen address

#define DEBOUNCE_DELAY 50
#define ENCODER_DELAY 50

#define ENCODER_1_OFFSET 8
#define ENCODER_2_OFFSET 12

#define LED_RED A0
#define LED_GREEN A1
#define LED_BLUE A2

#define MODIFIER_PRESS 0
#define MODIFIER_RELEASE 128

#define BUFFER_SIZE 16 // Size of the command buffer
volatile uint8_t commandBuffer[BUFFER_SIZE];
volatile uint8_t bufferHead = 0;
volatile uint8_t bufferTail = 0;

#define EMPTY_BUFFER_PLACEHOLDER 0xFF
// Colors for modes
int colorModeA[3] = {255, 0, 0}; // Colour Mode A
int colorModeB[3] = {0, 0, 255}; // Colour Mode B

int modeAValues[] = {0, 1}; // Clockwise, Counterclockwise
int modeBValues[] = {2, 3}; // Clockwise, Counterclockwise

// Debugging
#define DEBUG 1
#if DEBUG
#define DebugPrint(x) Serial.print(x)
#define DebugPrintln(x) Serial.println(x)
#else
#define DebugPrint(x)
#define DebugPrintln(x)
#endif

static int lastColor[3] = {255, 0, 0};

void addCommand(uint8_t modifier, uint8_t command)
{
    // serial print details
    DebugPrint("Modifier: ");
    DebugPrint(modifier == MODIFIER_PRESS ? "PRESS" : "RELEASE");
    DebugPrint(" Command: ");
    DebugPrintln(command);
    // Add a command to the buffer
    uint8_t nextHead = (bufferHead + 1) % BUFFER_SIZE;
    if (nextHead == bufferTail)
    {
        bufferTail = (bufferTail + 1) % BUFFER_SIZE; // Wrap around and discard the oldest command
    }

    //calculate the command value and store it in the buffer
    uint8_t value = command | modifier;
    commandBuffer[bufferHead] = value;
    bufferHead = nextHead;
}

// I2C request handler
void onRequest()
{
    if (bufferHead != bufferTail)
    {
        uint8_t command = commandBuffer[bufferTail];
        Serial.print("Sending command: ");
        Serial.println(command, HEX);
        Wire.write(command);                         // Send the next command
        bufferTail = (bufferTail + 1) % BUFFER_SIZE; // Move to the next command
    }
    else
    {
        Wire.write(EMPTY_BUFFER_PLACEHOLDER); // Placeholder for empty buffer
    }
}



void setLedColor(int r, int g, int b)
{
    if (lastColor[0] != r || lastColor[1] != g || lastColor[2] != b)
    {
        analogWrite(LED_RED, r);
        analogWrite(LED_GREEN, g);
        analogWrite(LED_BLUE, b);
        lastColor[0] = r;
        lastColor[1] = g;
        lastColor[2] = b;
    }
}

void startupFlashLed()
{
    //cycle through RGB colors twice to indicate startup
    for (int i = 0; i < 2; i++)
    {
        setLedColor(255, 0, 0); // Red
        delay(500);
        setLedColor(0, 255, 0); // Green
        delay(500);
        setLedColor(0, 0, 255); // Blue
        delay(500);
    }
}

void setup()
{
    Serial.begin(115200);
    DebugPrintln("Starting...");

    // Set up the address for the I2C based on a pin state
    pinMode(ADDRESS_PIN, INPUT_PULLUP); // Configure the address pin with internal pull-up

    // Read the state of the address pin
    if (digitalRead(ADDRESS_PIN) == LOW)
    {
        i2cAddress = ADDRESS_1; // Use the first address
    }
    else
    {
        i2cAddress = ADDRESS_2; // Use the second address
    }

    Serial.print("I2C address: 0x");
    Serial.println(i2cAddress, HEX);
    Wire.begin(i2cAddress);    // Start I2C communication
    Wire.onRequest(onRequest); // Register the request handler

    // Initialize buttons
    for (int i = 0; i < BUTTON_COUNT; i++)
    {
        pinMode(buttons[i], INPUT_PULLUP);
    }

    // Initialize encoders
    for (int i = 0; i < ENCODER_COUNT; i++)
    {
        pinMode(encoderSwitch[i], INPUT_PULLUP);
        pinMode(encoderCLK[i], INPUT);
        pinMode(encoderDATA[i], INPUT);
    }

    // Initialize values
    for (int i = 0; i < ENCODER_COUNT; i++)
    {
        encoderLastSwitchState[i] = digitalRead(encoderSwitch[i]);
    }
    for (int i = 0; i < BUTTON_COUNT; i++)
    {
        buttonsLastState[i] = digitalRead(buttons[i]);
    }



    // Initialize RGB LED pins
    pinMode(LED_RED, OUTPUT);
    pinMode(LED_GREEN, OUTPUT);
    pinMode(LED_BLUE, OUTPUT);

    startupFlashLed();

    // Set initial color based on Encoder 1 mode
    setLedColor(colorModeA[0], colorModeA[1], colorModeA[2]);

    DebugPrintln("Started.");
}

void handleEncoderSwitch(int encoder)
{
    // Only handle the switch for Rotary Encoder 1
    if (encoder == 0)
    {
        int currentState = digitalRead(encoderSwitch[encoder]);

        // If the button is pressed
        if (currentState == BUTTON_PRESSED)
        {
            // Debounce the press event
            if ((millis() - encoderLastDebounceTime[encoder]) > DEBOUNCE_DELAY)
            {

                int checkState = digitalRead(encoderSwitch[encoder]);

                if (checkState == BUTTON_PRESSED)
                {
                    encoderMode[encoder] = !encoderMode[encoder]; // Toggle mode

                    // Change LED color based on mode
                    if (encoderMode[encoder])
                    {
                        setLedColor(colorModeB[0], colorModeB[1], colorModeB[2]); // Mode B
                    }
                    else
                    {
                        setLedColor(colorModeA[0], colorModeA[1], colorModeA[2]); // Mode A
                    }

                    DebugPrint("Encoder 1 mode switched to ");
                    DebugPrintln(encoderMode[encoder] ? "B" : "A");

                    encoderLastDebounceTime[encoder] = millis();

                    // wait for the button to be released
                    while (digitalRead(encoderSwitch[encoder]) == BUTTON_PRESSED)
                    {
                        delay(10);
                    }
                }
            }
        }
    }
}

void handleEncoderRotation(int encoder)
{
    static int lastCLKState[ENCODER_COUNT] = {HIGH, HIGH};
    static unsigned long lastReleaseTime[ENCODER_COUNT] = {0, 0}; // Track last release event timing
    static bool isReleased[ENCODER_COUNT] = {true, true};         // Track if the "release" was sent
    static int lastValue[ENCODER_COUNT] = {0, 0};                 // Store the last value for the release event

    int currentCLKState = digitalRead(encoderCLK[encoder]);
    if (currentCLKState != lastCLKState[encoder] && currentCLKState == LOW)
    {
        int dataState = digitalRead(encoderDATA[encoder]);
        int value;

        // Determine rotation direction and assign value
        if (dataState == LOW)
        {
            // Clockwise
            value = (encoder == 0)
                        ? (encoderMode[encoder] ? modeBValues[0] : modeAValues[0])
                        : modeAValues[0]; // Encoder 2 always uses mode A
        }
        else
        {
            // Counterclockwise
            value = (encoder == 0)
                        ? (encoderMode[encoder] ? modeBValues[1] : modeAValues[1])
                        : modeAValues[1]; // Encoder 2 always uses mode A
        }

        if (encoder == 0)
        {
            value += ENCODER_1_OFFSET;
        }
        else
        {
            value += ENCODER_2_OFFSET;
        }

        DebugPrint("Encoder ");
        DebugPrint(encoder + 1);
        DebugPrint(" rotated ");
        DebugPrintln(dataState == LOW ? "clockwise" : "counterclockwise");

        if (isReleased[encoder]) // Only send a "press" if "release" was sent
        {
            addCommand(MODIFIER_PRESS, value); // Send "press" event
            isReleased[encoder] = false;       // Mark as "pressed"
            lastValue[encoder] = value;        // Store the value for the release event
        }
    }

    // Handle non-blocking "release" event
    if (!isReleased[encoder] && millis() - lastReleaseTime[encoder] >= ENCODER_DELAY)
    {
        addCommand(MODIFIER_RELEASE, lastValue[encoder]); // Send "release" event with the stored value
        lastReleaseTime[encoder] = millis();              // Update the last release time
        isReleased[encoder] = true;                       // Mark as "released"
    }

    lastCLKState[encoder] = currentCLKState; // Update the last state
}

void checkButtons()
{
    for (int i = 0; i < BUTTON_COUNT; i++)
    {
        int currentState = digitalRead(buttons[i]);
        if (currentState != buttonsLastState[i])
        {
            lastDebounceTime[i] = millis();
        }

        if ((millis() - lastDebounceTime[i]) > DEBOUNCE_DELAY)
        {
            if (currentState != buttonsState[i])
            {
                buttonsState[i] = currentState;

                if (buttonsState[i] == BUTTON_PRESSED)
                {
                    addCommand(MODIFIER_PRESS, i);
                }
                else
                {
                    addCommand(MODIFIER_RELEASE, i);
                }
            }
        }

        buttonsLastState[i] = currentState;
    }
}

void loop()
{
    // Check buttons
    checkButtons();

    // Handle encoders
    for (int i = 0; i < ENCODER_COUNT; i++)
    {
        handleEncoderSwitch(i);
        handleEncoderRotation(i);
    }
}

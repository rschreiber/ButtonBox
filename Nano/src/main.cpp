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

// Encoder definitions
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

#define I2C_ADDRESS 0x04
#define I2C_ENABLED 1

#define DEBOUNCE_DELAY 50
#define ENCODER_DELAY 50

#define KEY_PRESSED_OFFSET 0
#define KEY_RELEASED_OFFSET 64

#define ENCODER_1_OFFSET 16
#define ENCODER_2_OFFSET 32



#define LED_RED A0
#define LED_GREEN A1
#define LED_BLUE A2

// Colors for modes
int colorModeA[3] = {255, 0, 0}; // Red for Mode A
int colorModeB[3] = {0, 0, 255}; // Blue for Mode B

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

void setup()
{
    Serial.begin(115200);
    DebugPrintln("Starting...");

#if I2C_ENABLED
    DebugPrint("I2C address: ");
    DebugPrintln(I2C_ADDRESS);
    DebugPrintln("I2C enabled.");
    Wire.begin();
#endif

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

    // Set initial color based on Encoder 1 mode
    setLedColor(colorModeA[0], colorModeA[1], colorModeA[2]);

    DebugPrintln("Started.");
}

void keyPress(int key)
{
    DebugPrint("Key pressed: ");
    DebugPrintln(key + KEY_PRESSED_OFFSET);

#if I2C_ENABLED
    Wire.beginTransmission(I2C_ADDRESS);
    Wire.write(key + KEY_PRESSED_OFFSET);
    Wire.endTransmission();
#endif
}

void keyRelease(int key)
{
    DebugPrint("Key released: ");
    DebugPrintln(key);

#if I2C_ENABLED
    Wire.beginTransmission(I2C_ADDRESS);
    Wire.write(key + KEY_RELEASED_OFFSET);
    Wire.endTransmission();
#endif
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
#if I2C_ENABLED
            keyPress(value); // Send "press" event
#endif
            isReleased[encoder] = false; // Mark as "pressed"
            lastValue[encoder] = value;  // Store the value for the release event
        }
    }

    // Handle non-blocking "release" event
    if (!isReleased[encoder] && millis() - lastReleaseTime[encoder] >= ENCODER_DELAY)
    {
#if I2C_ENABLED
        keyRelease(lastValue[encoder]); // Send "release" event with the stored value
#endif
        lastReleaseTime[encoder] = millis(); // Update the last release time
        isReleased[encoder] = true;          // Mark as "released"
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
                    keyPress(i);
                }
                else
                {
                    keyRelease(i);
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

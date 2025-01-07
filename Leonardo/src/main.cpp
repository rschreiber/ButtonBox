#include <Arduino.h>
#include <Keyboard.h>
#include <Wire.h>


void receiveEvent(int howMany);

//what to run when the processor starts up
void setup() {
  Serial.begin (115200);
  delay (2000);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(1000);
  digitalWrite(LED_BUILTIN, LOW);
  delay(1000);   
  Serial.println("I2C Slave Device starting up..."); 
  //start the keyboard up
  Keyboard.begin();
  //setup i2c communication for incoming data
  Wire.begin(0x04);
  //tell the arduino to call the receiveEvent function when data is received
  Wire.onReceive(receiveEvent);
  Serial.println("I2C Slave Device Ready!");
}

//what runs when a new I2C event is received
void receiveEvent(int howMany) {
  Serial.println("Data Received");
  while (Wire.available()) { // Read all available bytes
    char c = Wire.read();
    Keyboard.write(c);      // Send the byte via Keyboard
    Serial.print("Received byte: ");
    Serial.println(c);
  }
}


//what to run in the main loop
void loop() {
  delay(100);
}



#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoOTA.h>

#include <NTPClient.h>
#include <WiFiUdp.h>
#include <TimeLib.h>

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

const char* ssid = "BT-CNAKF2";  ////
const char* password = "nUMEGrLFLDbU4D";  /////

const int dataPin = 23;  // HV5622 Data IN Pin
const int clockPin = 18; // HV5622 CLK Pin
const int latchPin = 5;  // HV5622 LE Pin

const int WIFI = 16;  //  WIFI LED
const int HVOUT = 17;  // HV OUTPUT LOW 

// Global variables for time digits
int hourFirstDigit;
int hourSecondDigit;
int minuteFirstDigit;
int minuteSecondDigit;
int secondFirstDigit;
int secondSecondDigit;


// Array to represent the 6 lines of Nixie digits, mapping Line and LED to the shift register output
const int digitMapping[6][10] = {
  // IC1 HV5622 (Pins 1-30 for Lines 1, 2, and 3)
  {1, 2, 3, 4, 5, 6, 7, 8, 9, 10},   // Line 1: Nixie Tube XX:XX:X1
  {11, 12, 13, 14, 15, 16, 17, 18, 19, 20}, // Line 2: Nixie Tube XX:XX:2X
  {21, 22, 23, 24, 25, 26, 27, 28, 29, 30}, // Line 3: Nixie Tube XX:X3:XX

  // IC2 HV5622 (Pins 32-62 for Lines 4, 5, and 6)
  {33, 34, 35, 36, 37, 38, 39, 40, 41, 42},   // Line 4: Nixie Tube XX:4X:XX
  {43, 44, 45, 46, 47, 48, 49, 50, 51, 52},   // Line 5: Nixie Tube X5:XX:XX
  {53, 54, 55, 56, 57, 58, 59, 60, 61, 62}    // Line 6: Nixie Tube 6X:XX:XX
};

void setup() {
  pinMode(HVOUT, OUTPUT);
  digitalWrite(HVOUT, HIGH);
  pinMode(dataPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(latchPin, OUTPUT);
  pinMode(WIFI, OUTPUT);
  Serial.begin(9600);

  WiFi.begin(ssid, password);
 
  while (WiFi.status() != WL_CONNECTED) {

    digitalWrite(WIFI, HIGH);
    delay(50);
    digitalWrite(WIFI, LOW);
    delay(50);

    digitalWrite(HVOUT, LOW);
  }
  digitalWrite(WIFI, HIGH);

  delay(1000);

  // Initialize by clearing all outputs
  clearAll();

  ArduinoOTA.begin();
  ArduinoOTA.setPassword("Password");


  timeClient.begin();

}

void loop() {
  ArduinoOTA.handle();
  getTime();




  setMultipleDigits(secondSecondDigit,secondFirstDigit,minuteSecondDigit,minuteFirstDigit,hourSecondDigit,hourFirstDigit);

  delay(1001); // Wait before restarting the loop
  


}



  


// Function to activate specific digits on multiple lines
void setMultipleDigits(int digit1, int digit2, int digit3, int digit4, int digit5, int digit6) {
  uint64_t data = 0;  // Clear previous data

  data |= (1ULL << digitMapping[0][digit1]);  // Line 1
  data |= (1ULL << digitMapping[1][digit2]);  // Line 2
  data |= (1ULL << digitMapping[2][digit3]);  // Line 3
  data |= (1ULL << digitMapping[3][digit4]);  // Line 4
  data |= (1ULL << digitMapping[4][digit5]);  // Line 5
  data |= (1ULL << digitMapping[5][digit6]);  // Line 6

  digitalWrite(latchPin, LOW);
  for (int i = 63; i >= 0; i--) {
    digitalWrite(dataPin, (data & (1ULL << i)) ? LOW : HIGH);
    toggleClock();
  }
  digitalWrite(latchPin, HIGH);
}


void clearAll() {
  digitalWrite(dataPin, HIGH);  // Ensure data pin is low (all outputs off)
  digitalWrite(latchPin, LOW); // Prepare latch pin for data transfer

  // Shift out 64 bits of LOW (clears all outputs)
  for (int i = 0; i < 64; i++) {
    digitalWrite(dataPin, HIGH);  // Send LOW to ensure outputs are off
    toggleClock();               // Advance to the next bit
  }

  digitalWrite(latchPin, HIGH);  // Latch the cleared data into the outputs
  Serial.println("All outputs cleared.");
}



void toggleClock() {
  digitalWrite(clockPin, HIGH);  // Set clock pin HIGH
  delayMicroseconds(1);          // Short delay to ensure stability
  digitalWrite(clockPin, LOW);   // Set clock pin LOW
  delayMicroseconds(1);          // Short delay to complete the cycle
}




void getTime(){
  timeClient.update();
  // Get the current time from the internal clock

  // Sync TimeLib with NTPClient
  setTime(timeClient.getEpochTime());


  int currentHour = hour();
  int currentMinute = minute();
  int currentSecond = second();

  // Format the time as HHMMSS with leading zeros
  char formattedTime[7]; // HHMMSS takes 6 characters plus a null terminator
  sprintf(formattedTime, "%02d%02d%02d", currentHour, currentMinute, currentSecond);

    // Extract each digit
  hourFirstDigit = formattedTime[0] - '0';
  hourSecondDigit = formattedTime[1] - '0';
  minuteFirstDigit = formattedTime[2] - '0';
  minuteSecondDigit = formattedTime[3] - '0';
  secondFirstDigit = formattedTime[4] - '0';
  secondSecondDigit = formattedTime[5] - '0';

  Serial.print(hourFirstDigit);
  Serial.print(hourSecondDigit);
  Serial.print(":");
  Serial.print(minuteFirstDigit);
  Serial.print(minuteSecondDigit);
  Serial.print(":");
  Serial.print(secondFirstDigit);
  Serial.println(secondSecondDigit);
}

void printBinary(uint64_t num) {
  for (int i = 64; i >= 0; i--) {
    Serial.print((num & (1UL << i)) ? "1" : "0");
  }
  Serial.println();
}



#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <TimeLib.h>  // Include the Time library

// WiFi credentials
const char *ssid = "Wokwi-GUEST";
const char *password = "";

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000);

// Pin Definitions
const int latchPin = 16;   // ESP32 GPIO connected to 74HC595 ST_CP (RCLK)
const int clockPin = 17;  // ESP32 GPIO connected to 74HC595 SH_CP (SRCLK)
const int dataPin = 2;   // ESP32 GPIO connected to 74HC595 DS
const int DSTButton = 5;
const int Button2 = 18;

unsigned long previousMillis = 0;           // Stores the last time displayDate was called
unsigned long interval = 0;                 // Dynamic interval based on random value
unsigned long previousNtpUpdateMillis = 0;  // Stores the last time NTP was updated
//const unsigned long ntpUpdateInterval = 60000; // Update NTP every 60 seconds
const unsigned long ntpUpdateInterval = 1 * 3600 * 1000; // Update NTP every 1 hour (Change the first digit to change lenght in hours )


// Array to represent the 6 lines of LEDs, mapping Line and LED to the shift register output
const int ledMapping[7][10] = {
  {0, 1, 2, 3, 4, 5, 6, 7, 8, 9},    // Line 1: IC1 Q0 to IC2 Q1
  {10, 11, 12, 13, 14, 15, 16, 17, 18, 19}, // Line 2: IC2 Q2 to IC3 Q3
  {20, 21, 22, 23, 24, 25, 26, 27, 28, 29}, // Line 3: IC3 Q4 to IC4 Q5
  {30, 31, 32, 33, 34, 35, 36, 37, 38, 39}, // Line 4: IC4 Q6 to IC5 Q7
  {40, 41, 42, 43, 44, 45, 46, 47, 48, 49}, // Line 5: IC6 Q0 to IC7 Q1
  {50, 51, 52, 53, 54, 55, 56, 57, 58, 59},  // Line 6: IC7 Q2 to IC8 Q3
  {60, -1, -1, -1, -1, -1, -1, -1, -1, -1} // led blink line
};

uint8_t shiftRegisterData[8] = {0}; // 8 bytes for 8 ICs (one byte per shift register)

//Gets the time from NTP, strips down to individual digits and assigns to LEDS
void getTime() {
  unsigned long currentMillis = millis();


  // Get the current time from the internal clock
  int currentHour = hour();
  int currentMinute = minute();
  int currentSecond = second();

  // Format the time as HHMMSS with leading zeros
  char formattedTime[7]; // HHMMSS takes 6 characters plus a null terminator
  sprintf(formattedTime, "%02d%02d%02d", currentHour, currentMinute, currentSecond);

  // Extract each digit
  int hourFirstDigit = formattedTime[0] - '0';
  int hourSecondDigit = formattedTime[1] - '0';
  int minuteFirstDigit = formattedTime[2] - '0';
  int minuteSecondDigit = formattedTime[3] - '0';
  int secondFirstDigit = formattedTime[4] - '0';
  int secondSecondDigit = formattedTime[5] - '0';

  // Update the LEDs based on the time
  setLED(0, secondSecondDigit, true);
  setLED(1, secondFirstDigit, true);
  setLED(2, minuteSecondDigit, true);
  setLED(3, minuteFirstDigit, true);
  setLED(4, hourSecondDigit, true);
  setLED(5, hourFirstDigit, true);
  if (secondSecondDigit % 2 == 0) setLED(6, 0, true); // Blinks colon on every even second


  // Update NTP time every 60 seconds
  if (currentMillis - previousNtpUpdateMillis >= ntpUpdateInterval) {
    timeClient.update();
    setTime(timeClient.getEpochTime()); // Update internal clock with NTP time
    previousNtpUpdateMillis = currentMillis;
    Serial.println("NTP time updated");
  }
  delay(1000);  // Display for 1 second

  setLED(0, secondSecondDigit, false);
  setLED(1, secondFirstDigit, false);
  setLED(2, minuteSecondDigit, false);
  setLED(3, minuteFirstDigit, false);
  setLED(4, hourSecondDigit, false);
  setLED(5, hourFirstDigit, false);
  setLED(6, 0, false); // Turns off blink

}
//GEts date from NTP, strips down to individual digits and assings to LEDs
void displayDate() {
  timeClient.update();

  // Get current time as Unix Epoch time
  unsigned long epochTime = timeClient.getEpochTime();

  // Set the current time using the Time library
  setTime(epochTime);

  int currentDay = day();
  int currentMonth = month();
  int currentYear = year() % 100;

  // Extract digits
  int firstDigitOfDay = currentDay / 10;
  int secondDigitOfDay = currentDay % 10;
  int firstDigitOfMonth = currentMonth / 10;
  int secondDigitOfMonth = currentMonth % 10;
  int firstDigitOfYear = currentYear / 10;
  int secondDigitOfYear = currentYear % 10;

  cycle();

  setLED(0, secondDigitOfYear, true);
  setLED(1, firstDigitOfYear, true);
  setLED(2, secondDigitOfMonth, true);
  setLED(3, firstDigitOfMonth, true);
  setLED(4, secondDigitOfDay, true);
  setLED(5, firstDigitOfDay, true);

  delay(5000);

  setLED(0, secondDigitOfYear, false);
  setLED(1, firstDigitOfYear, false);
  setLED(2, secondDigitOfMonth, false);
  setLED(3, firstDigitOfMonth, false);
  setLED(4, secondDigitOfDay, false);
  setLED(5, firstDigitOfDay, false);

  cycle();
}

// Function to randomly pick a number between 12 and 18 minutes (in milliseconds)
unsigned long getRandomInterval() {
  int randomMinutes = random(14, 16);  // Generate a random number between 14 and 16 (inclusive)
  return randomMinutes * 60 * 1000;    // Convert minutes to milliseconds
}

void setLED(int line, int led, bool state) {
  if (ledMapping[line][led] == -1) {
    return;  // Skip invalid LED positions
  }
  
  int bitPosition = ledMapping[line][led];  // Get the bit position in the mapping
  int regIndex = bitPosition / 8;           // Determine which shift register byte to use
  int bitIndex = bitPosition % 8;           // Determine the specific bit in the byte

  if (state) {
    shiftRegisterData[regIndex] |= (1 << bitIndex);  // Set the specific bit to 1
  } else {
    shiftRegisterData[regIndex] &= ~(1 << bitIndex); // Set the specific bit to 0
  }

  updateShiftRegisters();
}

void updateShiftRegisters() {
  digitalWrite(latchPin, LOW); // Prepare to shift data
  for (int i = 7; i >= 0; i--) {
    shiftOut(dataPin, clockPin, MSBFIRST, shiftRegisterData[i]);
  }
  digitalWrite(latchPin, HIGH); // Latch the shifted data
}

void clearAllLEDs() {
  for (int i = 0; i < 8; i++) {
    shiftRegisterData[i] = 0;  // Clear all shift register data
  }
  updateShiftRegisters();      // Apply the changes
}
//cycles all digits from 0 - 9 - 0
void cycle() {
  int T = 80;

  // Turn on and off LEDs from 0 to 9
  for (int i = 0; i <= 9; i++) {
    setAllLinesLED(i, true);
    delay(T);
    setAllLinesLED(i, false);
    delay(T);
  }

  clearAllLEDs();  // Ensure all LEDs are off

  // Turn on and off LEDs from 8 to 0 (reverse direction)
  for (int i = 8; i >= 0; i--) {
    setAllLinesLED(i, true);
    delay(T);
    setAllLinesLED(i, false);
    delay(T);
  }
}

// Function to set the same LED across all lines
void setAllLinesLED(int ledIndex, bool state) {
  for (int line = 0; line < 6; line++) {
    setLED(line, ledIndex, state);
  }
}





void setup() {
  Serial.begin(9600);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("Connecting to WiFi...");
    delay(80);
  }
  Serial.println("Connected to WiFi");

  randomSeed(analogRead(0));  // Or any unused analog pin for generating randomness

  // Set the initial interval
  interval = getRandomInterval();

  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  pinMode(DSTButton, INPUT_PULLUP);
  pinMode(Button2, INPUT_PULLUP);
  
  
  clearAllLEDs();
  cycle();
  clearAllLEDs();
  
  timeClient.begin();  // Initialize NTP client
  timeClient.update(); // Get the initial time from NTP
  setTime(timeClient.getEpochTime()); // Set the time for the internal clock
  Serial.println(interval);
}
void loop() {
  getTime();  // Continually get and display the time
  

  unsigned long currentMillis = millis();

  // Check if the random interval has passed
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;  // Update the last time displayDate was called
    displayDate();                   // Call displayDate
    Serial.println("Display date ran");
    interval = getRandomInterval();  // Update the interval with a new random time
    Serial.print("random number ran, New Interval is ");
    Serial.println(interval);
  }
}

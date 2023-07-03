#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_PN532.h>
#include <Servo.h>

Adafruit_PN532 nfc(-1, -1);

// pin configuration
#define servo_pin D6
#define green_pin D8
#define red_pin D7
#define buzzer_pin D5
Servo s1;

// servo motor
const int unlocked = 0;
const int locked = 180;

// pre-defined UIDs with access
byte skyUID[] = {0x04, 0x96, 0x40, 0x92, 0xAD, 0x6F, 0x80};
const size_t skyUID_length = 7;

// function definitions
void checkNFC();
void lock();
void unlock();
bool checkUID(const uint8_t *uid1, size_t length1, const uint8_t *uid2, size_t length2);

void setup()
{
  Serial.begin(115200);

  s1.attach(servo_pin);
  pinMode(red_pin, OUTPUT);
  pinMode(green_pin, OUTPUT);
  s1.write(locked); // initialize servo to locked state
  digitalWrite(red_pin, LOW);
  digitalWrite(green_pin, LOW);
  noTone(buzzer_pin);

  // nfc reader setup
  nfc.begin();
  while (!nfc.getFirmwareVersion())
  {
    Serial.print("Could not find PN53x board");

    delay(1000);
  }
}

void loop()
{
  checkNFC();
  delay(1000);
}

// functions
void checkNFC()
{
  uint8_t success;
  uint8_t uid[] = {0, 0, 0, 0, 0, 0, 0}; // Buffer to store the returned UID
  uint8_t uidLength;                     // Length of the UID (4 or 7 bytes depending on ISO14443A card type)

  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);

  if (success)
  {
    /* Serial.println("Found an ISO14443A card");
    Serial.print("  UID Length: ");
    Serial.print(uidLength, DEC);
    Serial.println(" bytes");
    Serial.print("  UID Value: ");
    nfc.PrintHex(uid, uidLength);
    Serial.println(""); */

    if (checkUID(skyUID, skyUID_length, uid, uidLength))
    {
      unlock();
      delay(5000);
    }
    lock();
  }
}

// unlocks door
void unlock()
{
  s1.write(unlocked);
  digitalWrite(red_pin, LOW);
  digitalWrite(green_pin, HIGH);
  tone(buzzer_pin, 500);
}

// Locks door
void lock()
{
  s1.write(locked);
  noTone(buzzer_pin);
  digitalWrite(red_pin, HIGH);
  digitalWrite(green_pin, LOW);
  delay(200);
  digitalWrite(red_pin, LOW);
  delay(200);
  digitalWrite(red_pin, HIGH);
  delay(200);
  digitalWrite(red_pin, LOW);
  delay(200);
}

// Function to compare two NFC UIDs
bool checkUID(const uint8_t *uid1, size_t length1, const uint8_t *uid2, size_t length2)
{
  if (length1 != length2)
    return false;

  for (size_t i = 0; i < length1; i++)
  {
    if (uid1[i] != uid2[i])
    {
      return false;
    }
  }

  return true;
}

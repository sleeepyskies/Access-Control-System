#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_PN532.h>
#include <Servo.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

// WIFI credentials
const char *WIFI_SSID = "SuperTollerHotspot";
const char *WIFI_PASSWORD = "mrjh2041";
#define WIFI_RETRIES 20

// Backend host address
const char *HOST = "https://cms.leon-barth.de";

// Pin layout definitions
#define LED_GREEN D8
#define LED_RED D7
#define BUZZER D5
#define SERVO D6

// Door state definitions
#define DOOR_UNLOCKED 0
#define DOOR_LOCKED 180

/**
 * Check whether a NFC card is presented and  if there is one, it handles the validation
 * of the user. Therefore, it asks the backend for the permissions of the user and
 * notifies it if access was granted
 */
void checkNFC();

/**
 * Establishes a WiFi connection with the credentials above
 */
void connectWifi();

/**
 * Check permission for the specified user-room combination using the web backend and
 * notifies the backend if access was granted
 * @param user_key The unique key of the users's NFC card
 * @param room_id The id of the room that should be accessed
 *
 * @return true if access was granted
 */
bool checkPermissionAndNotify(char *user_key, int room_id);

/**
 * Notify backend that access was granted. Used by the "checkPermissionAndNotify"
 * method
 * @param user_id The id of the user in the database
 * @param room_id The id pf the room that should be accessed
 *
 * @return true if operation was successful
 */
bool notifyAccess(int user_id, int room_id);

/**
 * Controls the servo for locking the door
 */
void lockDoor();

/**
 * Controls the servo for unlocking the door
 */
void unlockDoor();

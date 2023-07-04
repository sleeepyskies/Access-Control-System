#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_PN532.h>
#include <Servo.h>
#include <HttpClient.h>
#include <ESP8266WiFi.h>

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

// wifi information
const char *ssid = "FRITZ!Box 7520 QC";
const char *password = "hexenhaus666";

WiFiClient client;

// function definitions
void checkNFC();
void lock();
void unlock();
bool checkUID(const uint8_t *uid1, size_t length1, const uint8_t *uid2, size_t length2);
void connectWifi();

/**
 * Check permission for the specified user-room combination using the web backend
 * @param user_key The unique key of the users's NFC card
 * @param room_id The id pf the room that should be accessed
 * @param client The HttpClient for the connection to the server
 */
int check_permission(char *user_key, int room_id, HttpClient *client);

/**
 * Notify backend that access was granted
 * @param user_key The unique key of the users's NFC card
 * @param room_id The id pf the room that should be accessed
 * @param client The HttpClient for the connection to the server
 */
int notify_access(char *user_key, int room_id, HttpClient *client);

void setup()
{
    Serial.begin(115200);

    connectWifi();

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

int check_permission(char *user_key, int room_id, HttpClient *client)
{
    char req[512];
    sprintf(req, "https://cms.leon-barth.de/items/Users?fields=rooms.id&filter[key][_eq]=%s&filter[rooms][id][_eq]=%d", user_key, room_id);

    client->get(req);
    int res = client->responseStatusCode();

    if (res >= 300)
    {
        return -1;
    }

    String payload = client->responseBody();

    if (payload.equals("{\"data\":[]}"))
        return -1;
    else
        return 0;
}

int notify_access(char *user_key, int room_id, HttpClient *client)
{
    char payload[512];
    sprintf(payload, "{\"user\":\"%s\",\"room\":%d}", user_key, room_id);

    client->post("https://cms.leon-barth.de/items/Accesses", "application/json", payload);

    int res = client->responseStatusCode();

    if (res >= 300)
        return -1;
    else
        return 0;
}

void connectWifi()
{
    // Connect to WiFi Network
    Serial.println();
    Serial.println();
    Serial.print("Connecting to WiFi");
    Serial.println("...");
    WiFi.begin(ssid, password);
    int retries = 0;
    while ((WiFi.status() != WL_CONNECTED) && (retries < 15))
    {
        retries++;
        delay(500);
        Serial.print(".");
    }
    if (retries > 14)
    {
        Serial.println(F("WiFi connection FAILED"));
    }
    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println(F("WiFi connected!"));
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP());
    }
    Serial.println(F("Setup ready"));
}

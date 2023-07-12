#include "main.h"

// NFC module definitions
Adafruit_PN532 nfc(-1, -1);

// Servo module definitions
Servo s1;

// Clients for communication with the Internet
WiFiClient client;
WiFiClientSecure https_client;
HTTPClient http_client;

// Buffer for UID conversion
char uid_buffer[32];

void setup()
{
    // Initialize serial port
    Serial.print("Initializing Serial Port with rate 115200");
    Serial.begin(115200);

    // Initialize Servo
    Serial.print("Initializing Servo and close door");
    s1.attach(SERVO);
    s1.write(DOOR_LOCKED); // initialize servo to DOOR_LOCKED state

    // Initialize LEDs
    Serial.print("Initializing LEDs and turn off");
    pinMode(LED_RED, OUTPUT);
    pinMode(LED_GREEN, OUTPUT);
    digitalWrite(LED_RED, LOW);
    digitalWrite(LED_GREEN, LOW);

    // Initialize buzzer
    Serial.print("Initializing Buzzer and turn off");
    noTone(BUZZER);

    // Initialize Wifi connection and SHA1 key of the backend TLS
    connectWifi();

    // nfc reader setup
    nfc.begin();
    while (!nfc.getFirmwareVersion())
    {
        Serial.print("Could not find PN53x board");

        delay(1000);
    }
    delay(1000);
}

void loop()
{
    // Poll the NFC reader every second
    checkNFC();
    delay(1000);
}

char *convertBytesToChar(uint8_t *bytes, int length)
{
    // Convert every byte to a string of its 2 digit hex representation
    for (int i = 0; i < length; i++)
        sprintf(uid_buffer + 2 * i, "%02X", bytes[i]);

    // Add string termination symbol
    uid_buffer[2 * length] = '\0';

    return uid_buffer;
}

void checkNFC()
{
    uint8_t uid[] = {0, 0, 0, 0, 0, 0, 0}; // Buffer to store the returned UID
    uint8_t uidLength;                     // Length of the UID (4 or 7 bytes depending on ISO14443A card type)

    // Read the NFC UID from the card
    if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength))
    {
        // Check if the user is allowed to access  and notify the backend on succes
        char *user_key = convertBytesToChar(uid, uidLength);
        if (checkPermissionAndNotify(user_key, 1))
        {
            // Open door
            unlockDoor();
            delay(5000);
        }
        // Close door after 5secs
        lockDoor();
    }
}

void unlockDoor()
{
    // Open the door
    s1.write(DOOR_UNLOCKED);

    // Turn off the RED LED
    digitalWrite(LED_RED, LOW);

    // Turn on the GREEN LED
    digitalWrite(LED_GREEN, HIGH);

    // Turn buzzer on for acoustic feedback
    tone(BUZZER, 500);
}

void lockDoor()
{
    // Close the door
    s1.write(DOOR_LOCKED);

    // Turn off the buzzer
    noTone(BUZZER);

    // Turn off the GREEN LED
    digitalWrite(LED_GREEN, LOW);

    // Blink the RED LED 2 times
    digitalWrite(LED_RED, HIGH);
    delay(200);
    digitalWrite(LED_RED, LOW);
    delay(200);
    digitalWrite(LED_RED, HIGH);
    delay(200);
    digitalWrite(LED_RED, LOW);
    delay(200);
}

bool checkPermissionAndNotify(char *user_key, int room_id)
{
    // Create GET request to check permissions
    char req[512];
    sprintf(req, "%s/items/Users?fields=id,rooms.Rooms_id&filter[key][_eq]=%s&filter[rooms][Rooms_id][_eq]=%d", HOST, user_key, room_id);

    Serial.println("Try to check permission: " + String(req));

    // Send GET request to the backend
    http_client.begin(https_client, req);
    int res = http_client.GET();
    String payload = http_client.getString();
    Serial.println("-> Got answer: " + String(res));
    Serial.println("-> Got answer: " + payload);

    // Handle failure
    if ((res < 200) || (res >= 300))
    {
        Serial.println("-> Failure");

        return false;
    }

    // Deserialize JSON response to extract user_id
    DynamicJsonDocument json(1024);
    deserializeJson(json, payload);

    // Interpretation of the answer
    if (payload.equals("{\"data\":[]}"))
    {
        Serial.println("-> Permission denied");
        return false;
    }

    // Extract user_id for access notification
    int user_id = json["data"][0]["id"];

    // Send access notification
    if (notifyAccess(user_id, room_id))
    {
        Serial.println("-> Permission granted");
        return true;
    }
    else
    {
        Serial.println("-> Permission denied");
        return false;
    }
}

bool notifyAccess(int user_id, int room_id)
{
    // Create POST request to notify the backend that acces was granted
    char payload[512];
    sprintf(payload, "{\"user\":%d,\"room\":%d}", user_id, room_id);

    Serial.println("Try to notify about access with payload: " + String(payload));

    // Send POST request to the backend
    http_client.begin(https_client, String(HOST) + "/items/Accesses");
    http_client.addHeader("Content-Type", "application/json");
    int res = http_client.POST(payload);

    Serial.println("-> Got answer: " + String(res));

    // Interpret the result
    if (res >= 300)
    {
        Serial.println("-> Failure");
        return 0;
    }
    else
    {
        Serial.println("-> Success");
        return 1;
    }
}

void connectWifi()
{
    // Start trying to connect to the WiFi
    Serial.println("Try connecting to WiFi: " + String(WIFI_SSID));
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    // Limit retries
    int retries = 0;
    while ((WiFi.status() != WL_CONNECTED) && (retries < WIFI_RETRIES))
    {
        retries++;
        delay(500);
        Serial.print(".");
    }

    // Handle failure
    if (retries >= WIFI_RETRIES)
    {
        Serial.println(F(" FAILED"));
    }

    // Handle success
    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println(F(" SUCCESS"));
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());

        https_client.setFingerprint("AE:DA:97:4E:F3:02:20:1F:0F:99:8C:12:26:E9:69:F4:7C:F5:60:88");
    }
}

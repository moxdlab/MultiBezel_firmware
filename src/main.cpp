#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>

#define MAX_TOUCHES 5

#include "TouchSensor.h"

/**
 *
 * Custom flex pcb for multi crown has pad on channels 22   23   24   25   26   27   28   29  (started counting at 0)
 *
 * inspiration for custom slider based on https://github.com/BelaPlatform/Trill-Arduino/blob/master/examples/custom-slider/custom-slider.ino
 * and https://forum.bela.io/d/4035-radial-sliders-with-trill/
 *
 * FIXME Mount cap sensor to smartwatch case. And maybe add a plain behind the sensor, so that the fingers are like to touch the sensor while rotating the crown.
 * FIXME ---does not use circular touch detection on edges of array--- simply merging touch on first and last pad if touch is recognized
*/


// magic numbers
const int prescaler = 3;

// values for wifi
const char *ssid = "MultiBezel-AP";
const char *password = "12345678";
const char *udpAddress = "192.168.4.2";
const int udpPort = 44444;
WiFiUDP udp;
int64_t lastMessageSent, timeSinceBoot;
int packetsPerSecond = 121;

MultitouchSensor multitouchSensor;
int firstChannel = 7;

void setupWiFi();
void printBezelTouches(std::array<Touch, MAX_TOUCHES> touches);
void sendDataViaUDP(std::array<Touch, MAX_TOUCHES> touches);

void setup() {
    Serial.begin(115200);

    multitouchSensor.setup(prescaler);
    delay(50);

    setupWiFi();
}

void setupWiFi() {
    Serial.println("Setup WiFi");
    Serial.println("\n[*] Creating AP");
    WiFiClass::mode(WIFI_AP);
    WiFi.softAP(ssid, password, 1, 0, 2);
    Serial.print("[+] AP Created with IP Gateway ");
    Serial.println(WiFi.softAPIP());
}


unsigned n = 0;
unsigned loop_cnt = 0;

void loop() {
    timeSinceBoot = esp_timer_get_time();
    if ((timeSinceBoot - lastMessageSent) >= 1000000 / packetsPerSecond) {
        multitouchSensor.read_data();
        std::array<Touch, MAX_TOUCHES> touches = multitouchSensor.get_touches_from_data();

        n = 0;
        //sendDataViaUDP(touches);
        printBezelTouches(touches);
    }
}

void sendDataViaUDP(std::array<Touch, MAX_TOUCHES> touches) {
    // send all raw data to receiver including number of touches as last int[] value
    int arr[MAX_TOUCHES + 1];
    for (n = 0; n < MAX_TOUCHES; n++) {
        if(touches[n].get_position() > 0) {
            arr[n] = (touches[n].get_position() * 100) - firstChannel * 100;
        }else{
            arr[n] = -1;
        }
    }
    arr[MAX_TOUCHES] = multitouchSensor.get_num_touches();

    udp.beginPacket(udpAddress, udpPort);
    udp.write((const uint8_t*)arr, sizeof(arr));
    udp.endPacket();
    lastMessageSent = esp_timer_get_time();
}

void printBezelTouches(std::array<Touch, MAX_TOUCHES> touches) {
    for (int i = 0; i < multitouchSensor.get_num_touches(); i++) {
        Touch curTouch = touches[i];
        if(i != 0)
            Serial.print("  ");
        Serial.print(curTouch.get_position());
        if(i == multitouchSensor.get_num_touches() - 1)
            Serial.println("");
    }
}

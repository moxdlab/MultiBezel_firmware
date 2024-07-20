#include <Arduino.h>
#include <Trill.h>
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
const u_int16_t noiseThreshold = 150;
const int prescaler = 3;

// values for wifi
const char *ssid = "MultiCrown-AP";
const char *password = "12345678";
const char *udpAddress = "192.168.4.2";
const int udpPort = 44444;
WiFiUDP udp;
int64_t lastMessageSent, timeSinceBoot;
int packetsPerSecond = 121;

// required objects
const uint8_t numberOfChannels = 8;
const unsigned int maxNumCentroids = 5;
const uint8_t usedChannels[numberOfChannels] = {7,8,9,10,11,12,13,14};
const unsigned int NUM_TOTAL_PADS = 30;
int numberOfTouches = 0;

uint16_t rawData[NUM_TOTAL_PADS];
CentroidDetection<maxNumCentroids, numberOfChannels> slider;
Trill trillSensor;

void setupWiFi();
void printBezelTouches();
void processRawData();
void sendDataViaUDP();

void setup() {

    // setup custom slider, i.e. radial touch ring
    slider.setup(usedChannels, numberOfChannels);
    //slider.setMinimumTouchSize(1);


    Serial.begin(115200);

    int trillsetup = 0;
    do {
        Serial.print("I2C scanner. Scanning... ");

        trillsetup = trillSensor.setup(Trill::TRILL_CRAFT );
        if (trillsetup != 0) {
            Serial.print("failed to initialise TRILL FLEX. ");
            Serial.print("Error code: ");
            Serial.print(trillsetup);
            Serial.println(" trying again.");
            delay(1000);
        } else {
            Serial.println("initialised TRILL FLEX.");
        }

    } while (trillsetup != 0);
    delay(50);

    //   pinMode(SELECTOR_PIN, INPUT);
    trillSensor.setMode(Trill::Mode::DIFF);

    //   // The value of the prescaler is an integer from 1-8. There are two rules of thumb for determining a prescaler value:
    //   // - A higher value prescaler produces a longer charging time, and is better for more resistive materials and larger conductive objects connected.
    //   // - A lower value prescaler is better for proximity sensing
    trillSensor.setPrescaler(prescaler);
    delay(50);

    //   // The value of the noise threshold is an integer from 0-255. If the measured value of a pad is less than the noise threshold, it is rounded down to 0.
    trillSensor.setNoiseThreshold(noiseThreshold);
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
        if (!trillSensor.requestRawData()) {
            Serial.println("Failed reading from device. Is it disconnected?");
            return setup();
        }


        n = 0;
        processRawData();
        sendDataViaUDP();
        printBezelTouches();
    }
}

void processRawData() {
    // read all the data from the device into a local buffer
    while (trillSensor.rawDataAvailable() > 0 && n < NUM_TOTAL_PADS) {
        rawData[n] = trillSensor.rawDataRead();
        n++;
    }
    // have custom slider process the raw data into touches
    slider.process(rawData);

    // check which pads have been touched
    bool padsActive[numberOfChannels] = {0, 0, 0, 0, 0, 0, 0, 0};
    for (int i = 0; i < slider.getNumTouches(); i++) {
        int pad = slider.touchLocation(i) >> 7;
        padsActive[pad] = 1;
    }
    numberOfTouches = slider.getNumTouches();
    // if first and last pads where touched, then correct count
    if (padsActive[0] && padsActive[7]) {
        numberOfTouches--;
    }
}

void sendDataViaUDP() {
    // send all raw data to receiver including number of touches as last int[] value
    int arr[numberOfChannels + 1];
    for (n = 0; n < numberOfChannels; n++) {
        arr[n] = rawData[usedChannels[n]];
    }
    arr[numberOfChannels] = numberOfTouches;

    udp.beginPacket(udpAddress, udpPort);
    udp.write((const uint8_t*)arr, sizeof(arr));
    udp.endPacket();
    lastMessageSent = esp_timer_get_time();
}

void printBezelTouches() {
    Serial.printf("%03d: ", loop_cnt++ % 1000);
    Serial.printf(" # ");


    // print the channels we feed into the centroid detection
    // also use threshold to remove low values
    for (n = 0; n < numberOfChannels; n++) {
        u_int16_t rawValue = rawData[usedChannels[n]];

        if ((rawValue > noiseThreshold) || (rawValue == 0)) {
            Serial.printf("%05u ", rawValue);
        } else {
            Serial.printf("--%03u ", rawValue);
        }
    }

    int numtouches = slider.getNumTouches();

    if (slider.getNumTouches() == 0) {
        Serial.print("| no fingers ");
    } else {

        // check which pads have been touched
        bool padsActive[numberOfChannels] = {0, 0, 0, 0, 0, 0, 0, 0};
        for (int i = 0; i < numtouches; i++) {
            int pad = slider.touchLocation(i) >> 7;
            padsActive[pad] = 1;
        }

        // if first and last pads where touched, then correct count
        if (padsActive[0] && padsActive[7]) {
            numtouches--;
        }

        Serial.printf("| %2d fingers ", numtouches);

        for (int i = 0; i < numtouches; i++) {
            int pad = slider.touchLocation(i) >> 7;
            //Serial.printf(" (loc: %4d", slider.touchLocation(i));
            Serial.printf(" pad: %02d ", pad);
            //Serial.printf(" size: %4d)", slider.touchSize(i));
        }

        // if first and last pads where touched, then correct count
        if (padsActive[0] && padsActive[7]) {
            Serial.print("CORRECTED TOUCH ON EDGE");
        }
    }

    Serial.println("");
}

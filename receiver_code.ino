#include <ESP8266WiFi.h>
#include <espnow.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <string.h>

extern "C" {
  #include "user_interface.h"
}

uint8_t transmitterMAC[] = {
  0x08, 0xF9, 0xE0, 0x6D, 0xC7, 0x33
};

const uint8_t DATA = 0, REQUEST = 1, READY = 2,
              COMMIT = 3, SWITCHED = 4;

const unsigned long SWITCH_DELAY_MS = 6000UL;
const unsigned long LCD_PAGE_MS = 3500UL;  // Changed from 1500 to 3500
const unsigned long LCD_REFRESH_MS = 100UL;

const uint8_t LCD_PAGE_COUNT = 5;

// Change 0x27 to 0x3F if your LCD uses that address.
LiquidCrystal_I2C lcd(0x27, 16, 2);

// MUST exactly match the transmitter's Packet structure.
struct Packet {
  uint8_t type;
  uint8_t channel;
  uint32_t id;
  float temperatureC;
  uint8_t apCount[3];
  int16_t apAvgRssi[3];
  float channelScore[3];
  char message[32];
};

uint8_t currentChannel = 1;
uint8_t targetChannel = 1;

bool radioReady = false;
bool switchPending = false;

unsigned long switchAt = 0;
unsigned long lastLCDUpdate = 0;

uint32_t receivedCount = 0;
uint32_t lostPackets = 0;
uint32_t lastReceivedID = 0;

Packet latestPacket = {};
volatile bool newDataAvailable = false;
bool haveTelemetry = false;


bool validChannel(uint8_t ch) {
  return ch == 1 || ch == 6 || ch == 11;
}


bool setPeer(uint8_t ch) {
  if (esp_now_is_peer_exist(transmitterMAC))
    esp_now_del_peer(transmitterMAC);

  int r = esp_now_add_peer(
    transmitterMAC, ESP_NOW_ROLE_COMBO, ch, NULL, 0
  );

  Serial.printf("PEER,%s,channel=%u\n",
                r == 0 ? "OK" : "FAILED", ch);

  return r == 0;
}


void sendPacket(uint8_t type, uint8_t ch, uint32_t id,
                const char *msg) {
  Packet p = {};

  p.type = type;
  p.channel = ch;
  p.id = id;

  strncpy(p.message, msg, sizeof(p.message) - 1);

  uint8_t r = esp_now_send(
    transmitterMAC, (uint8_t *)&p, sizeof(p)
  );

  if (r != 0) {
    Serial.printf("SEND_ERROR,type=%u,result=%u\n",
                  type, r);
  }
}


void onSent(uint8_t *mac, uint8_t status) {
  Serial.printf("RX_REPLY_STATUS,%s\n",
                status == 0 ? "SUCCESS" : "FAILED");
}


void onReceive(uint8_t *mac, uint8_t *data, uint8_t len) {
  if (len != sizeof(Packet)) return;

  Packet p;
  memcpy(&p, data, sizeof(p));

  // Original request/ready/commit/switch protocol.
  if (p.type == REQUEST) {
    if (!validChannel(p.channel) ||
        p.channel == currentChannel ||
        switchPending)
      return;

    targetChannel = p.channel;

    Serial.printf(
      "SWITCH_REQUEST_RECEIVED,target=%u,current=%u\n",
      targetChannel, currentChannel
    );

    sendPacket(READY, targetChannel, 0, "READY");
    Serial.println("READY_SENT");
  }

  else if (p.type == COMMIT) {
    if (!validChannel(p.channel) ||
        p.channel == currentChannel ||
        switchPending)
      return;

    targetChannel = p.channel;
    switchPending = true;
    switchAt = millis() + SWITCH_DELAY_MS;

    Serial.printf(
      "SWITCH_COMMIT_RECEIVED,target=%u,delay_ms=%lu\n",
      targetChannel, SWITCH_DELAY_MS
    );
  }

  else if (p.type == DATA) {
    latestPacket = p;
    newDataAvailable = true;
  }
}


// Write one fixed 16-character LCD line.
void printLCDLine(uint8_t row, const char *text) {
  char line[17];

  snprintf(line, sizeof(line), "%-16.16s", text);

  lcd.setCursor(0, row);
  lcd.print(line);
}


// Return "--" if no APs were detected on that channel.
void formatRSSI(uint8_t index, char *out, size_t size) {
  if (latestPacket.apCount[index] == 0) {
    snprintf(out, size, "--");
  } else {
    snprintf(out, size, "%d",
             latestPacket.apAvgRssi[index]);
  }
}


void updateLCD() {
  if (!haveTelemetry) {
    printLCDLine(0, "Cognitive Radio");
    printLCDLine(1, "Waiting for data");
    return;
  }

  char line1[17];
  char line2[17];

  uint8_t page =
    (millis() / LCD_PAGE_MS) % LCD_PAGE_COUNT;

  switch (page) {

    // PAGE 1: active channel and temperature.
    case 0:
      snprintf(line1, sizeof(line1),
               "Active Ch: %u",
               latestPacket.channel);

      snprintf(line2, sizeof(line2),
               "Temp: %.1f C",
               latestPacket.temperatureC);
      break;

    // PAGE 2: AP counts for channels 1, 6 and 11.
    case 1:
      snprintf(line1, sizeof(line1),
               "Wi-Fi APs 1/6/11");

      snprintf(line2, sizeof(line2),
               "%u     %u     %u",
               latestPacket.apCount[0],
               latestPacket.apCount[1],
               latestPacket.apCount[2]);
      break;

    // PAGE 3: average RSSI of detected APs.
    case 2: {
      char r1[6], r6[6], r11[6];

      formatRSSI(0, r1, sizeof(r1));
      formatRSSI(1, r6, sizeof(r6));
      formatRSSI(2, r11, sizeof(r11));

      snprintf(line1, sizeof(line1),
               "Avg RSSI C1/6/11");

      snprintf(line2, sizeof(line2),
               "%s  %s  %s", r1, r6, r11);
      break;
    }

    // PAGE 4: channel-selection scores.
    case 3:
      snprintf(line1, sizeof(line1),
               "Scores Ch1/6/11");

      snprintf(line2, sizeof(line2),
               "%.1f  %.1f  %.1f",
               latestPacket.channelScore[0],
               latestPacket.channelScore[1],
               latestPacket.channelScore[2]);
      break;

    // PAGE 5: packet reception and loss.
    case 4:
      snprintf(line1, sizeof(line1),
               "Packets Received");

      snprintf(line2, sizeof(line2),
               "Count:%lu Lost:%lu",
               (unsigned long)receivedCount,
               (unsigned long)lostPackets);
      break;
  }

  printLCDLine(0, line1);
  printLCDLine(1, line2);
}


void setup() {
  Serial.begin(115200);
  delay(1200);

  Serial.println("\n=== COGNITIVE RADIO RECEIVER ===");

  // I2C pins: SDA = D2/GPIO4, SCL = D1/GPIO5.
  Wire.begin(D2, D1);

  lcd.init();
  lcd.backlight();
  lcd.clear();

  printLCDLine(0, "Cognitive Radio");
  printLCDLine(1, "Starting...");

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  currentChannel = 1;
  wifi_set_channel(currentChannel);

  Serial.printf("RX_MAC,%s\n",
                WiFi.macAddress().c_str());

  if (esp_now_init() != 0) {
    Serial.println("ERROR,ESP_NOW_INIT_FAILED");

    printLCDLine(0, "ESP-NOW Error");
    printLCDLine(1, "Check setup");
    return;
  }

  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
  esp_now_register_send_cb(onSent);
  esp_now_register_recv_cb(onReceive);

  if (!setPeer(1)) return;

  radioReady = true;

  Serial.println("ESP_NOW_READY,channel=1");
}


void loop() {
  if (!radioReady) {
    delay(100);
    return;
  }

  // Original coordinated channel-switching logic.
  if (switchPending &&
      (long)(millis() - switchAt) >= 0) {

    wifi_set_channel(targetChannel);
    delay(100);

    if (!setPeer(targetChannel)) {
      Serial.println("ERROR,PEER_UPDATE_FAILED");
      switchPending = false;
      return;
    }

    currentChannel = targetChannel;
    switchPending = false;

    Serial.printf("ACTIVE_CHANNEL,%u\n", currentChannel);

    sendPacket(SWITCHED, currentChannel, 0, "SWITCHED");

    Serial.printf(
      "SWITCHED_CONFIRMATION_SENT,channel=%u\n",
      currentChannel
    );
  }

  // Process received data in the main loop.
  if (newDataAvailable) {
    Packet p = latestPacket;
    newDataAvailable = false;

    if (lastReceivedID != 0 &&
        p.id > lastReceivedID + 1) {
      lostPackets += p.id - lastReceivedID - 1;
    }

    if (p.id > lastReceivedID)
      lastReceivedID = p.id;

    receivedCount++;
    haveTelemetry = true;

    Serial.printf(
      "RX_PACKET,id=%lu,channel=%u,count=%lu,message=%s\n",
      (unsigned long)p.id,
      p.channel,
      (unsigned long)receivedCount,
      p.message
    );

    // CSV data for MATLAB.
    Serial.printf(
      "DATA_CSV,%lu,%u,%.2f,%lu,%lu,"
      "%u,%d,%.3f,%u,%d,%.3f,%u,%d,%.3f\n",

      (unsigned long)p.id,
      p.channel,
      p.temperatureC,
      (unsigned long)receivedCount,
      (unsigned long)lostPackets,

      p.apCount[0],
      p.apAvgRssi[0],
      p.channelScore[0],

      p.apCount[1],
      p.apAvgRssi[1],
      p.channelScore[1],

      p.apCount[2],
      p.apAvgRssi[2],
      p.channelScore[2]
    );
  }

  // Static display; no scrolling.
  if (millis() - lastLCDUpdate >= LCD_REFRESH_MS) {
    lastLCDUpdate = millis();
    updateLCD();
  }

  delay(5);
}

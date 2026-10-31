#include <ESP8266WiFi.h>
#include <espnow.h>
#include <string.h>
#include <math.h>
#include <DHT.h>

extern "C" {
  #include "user_interface.h"
}

// DHT11 temperature and humidity module.
#define DHTPIN D5
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

uint8_t receiverMAC[] = {
  0xD8, 0xF1, 0x5B, 0x02, 0xC5, 0x9C
};

const uint8_t CH[3] = {1, 6, 11};

const uint8_t DATA = 0, REQUEST = 1, READY = 2,
              COMMIT = 3, SWITCHED = 4;

const unsigned long RESCAN_MS = 30000UL;
const unsigned long REQUEST_MS = 1000UL;
const unsigned long SWITCH_DELAY_MS = 6000UL;
const unsigned long DATA_MS = 1000UL;
const unsigned long TEMP_READ_MS = 2000UL;

// Both transmitter and receiver must use the exact
// same Packet structure and field order.
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
uint8_t startupTarget = 1;

bool ready = false;
bool pending = false;
bool readyReceived = false;

uint32_t packetID = 0;

unsigned long lastScan = 0;
unsigned long lastRequest = 0;
unsigned long switchAt = 0;
unsigned long lastData = 0;
unsigned long resumeDataAt = 0;
unsigned long lastTempRead = 0;

// -999 indicates that no valid DHT11 reading is available yet.
float temperatureC = -999.0f;

uint8_t networkCountByChannel[3] = {0, 0, 0};
int16_t averageRssiByChannel[3] = {-100, -100, -100};
float scoreByChannel[3] = {0.0, 0.0, 0.0};


// Read temperature from the DHT11 digital sensor.
// Returns NAN if the sensor reading fails.
float readTemperatureC() {
  return dht.readTemperature();
}


void sendPacket(uint8_t type, uint8_t ch, uint32_t id,
                const char *msg) {
  Packet p = {};

  p.type = type;
  p.channel = ch;
  p.id = id;
  p.temperatureC = temperatureC;

  memcpy(p.apCount, networkCountByChannel,
         sizeof(p.apCount));

  memcpy(p.apAvgRssi, averageRssiByChannel,
         sizeof(p.apAvgRssi));

  memcpy(p.channelScore, scoreByChannel,
         sizeof(p.channelScore));

  strncpy(p.message, msg, sizeof(p.message) - 1);

  uint8_t r = esp_now_send(
    receiverMAC, (uint8_t *)&p, sizeof(p)
  );

  if (r != 0) {
    Serial.printf("SEND_ERROR,type=%u,result=%u\n",
                  type, r);
  }
}


bool setPeer(uint8_t ch) {
  if (esp_now_is_peer_exist(receiverMAC))
    esp_now_del_peer(receiverMAC);

  int r = esp_now_add_peer(
    receiverMAC, ESP_NOW_ROLE_COMBO, ch, NULL, 0
  );

  Serial.printf("PEER,%s,channel=%u\n",
                r == 0 ? "OK" : "FAILED", ch);

  return r == 0;
}


int indexOf(uint8_t ch) {
  for (int i = 0; i < 3; i++) {
    if (CH[i] == ch) return i;
  }
  return 0;
}


// Original channel-scoring algorithm.
uint8_t scanBestChannel() {
  uint8_t count[3] = {0, 0, 0};
  long sum[3] = {0, 0, 0};

  float avg[3] = {-100, -100, -100};
  float score[3];

  Serial.println("SPECTRUM_SCAN_BEGIN");

  WiFi.disconnect();
  delay(200);

  int n = WiFi.scanNetworks(false, true);

  if (n < 0) {
    Serial.println("SCAN_FAILED");
    WiFi.scanDelete();
    return currentChannel;
  }

  for (int i = 0; i < n; i++) {
    int ch = WiFi.channel(i);

    for (int j = 0; j < 3; j++) {
      if (ch == CH[j]) {
        if (count[j] < 255) count[j]++;
        sum[j] += WiFi.RSSI(i);
      }
    }
  }

  for (int j = 0; j < 3; j++) {
    if (count[j] > 0)
      avg[j] = (float)sum[j] / count[j];

    float signalPenalty =
      count[j] ? (avg[j] + 100.0f) / 100.0f : 0.0f;

    // Original scoring formula: unchanged.
    score[j] = count[j] + 0.25f * signalPenalty;

    networkCountByChannel[j] = count[j];

    averageRssiByChannel[j] =
      count[j] ? (int16_t)avg[j] : -100;

    scoreByChannel[j] = score[j];

    Serial.printf(
      "SENSE,channel=%u,networks=%u,avg_rssi=%.1f,score=%.3f\n",
      CH[j], count[j], avg[j], score[j]
    );
  }

  int best = 0;

  for (int j = 1; j < 3; j++) {
    if (score[j] < score[best])
      best = j;
  }

  uint8_t candidate = CH[best];

  Serial.printf("BEST_CHANNEL,%u\n", candidate);
  WiFi.scanDelete();

  int ci = indexOf(currentChannel);

  if (candidate == currentChannel) {
    Serial.println("KEEP_CHANNEL,already_best");
    return currentChannel;
  }

  // Original 20% hysteresis rule: unchanged.
  if (score[ci] <= 0.01f ||
      score[best] > score[ci] * 0.80f) {

    Serial.printf(
      "KEEP_CHANNEL,hysteresis,current=%u,candidate=%u\n",
      currentChannel, candidate
    );

    return currentChannel;
  }

  return candidate;
}


void onSent(uint8_t *mac, uint8_t status) {
  Serial.printf("TX_MAC_STATUS,%s\n",
                status == 0 ? "SUCCESS" : "FAILED");
}


void onReceive(uint8_t *mac, uint8_t *data, uint8_t len) {
  if (len != sizeof(Packet)) return;

  Packet p;
  memcpy(&p, data, sizeof(p));

  if (p.type == READY &&
      pending &&
      p.channel == targetChannel &&
      !readyReceived) {

    readyReceived = true;

    Serial.printf("RECEIVER_READY,target=%u\n",
                  targetChannel);

    sendPacket(COMMIT, targetChannel, 0, "SWITCH_COMMIT");

    switchAt = millis() + SWITCH_DELAY_MS;

    Serial.printf(
      "SWITCH_SCHEDULED,channel=%u,delay_ms=%lu\n",
      targetChannel, SWITCH_DELAY_MS
    );
  }

  if (p.type == SWITCHED &&
      pending &&
      p.channel == targetChannel) {

    Serial.printf(
      "RECEIVER_SWITCHED,channel=%u\n", p.channel
    );
  }
}


void startSwitch(uint8_t candidate) {
  if (candidate == currentChannel || pending) return;

  targetChannel = candidate;
  pending = true;
  readyReceived = false;
  lastRequest = 0;

  Serial.printf(
    "SWITCH_REQUEST,target=%u,current=%u\n",
    targetChannel, currentChannel
  );
}


void setup() {
  Serial.begin(115200);
  delay(1200);

  Serial.println("\n=== COGNITIVE RADIO TRANSMITTER ===");

  // Start the DHT11 digital sensor.
  dht.begin();

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  Serial.printf("TX_MAC,%s\n",
                WiFi.macAddress().c_str());

  // Preserve original startup scan and initial channel 1.
  startupTarget = scanBestChannel();

  wifi_set_channel(1);
  currentChannel = 1;
  delay(100);

  // Initial DHT11 reading.
  float initialTemperature = readTemperatureC();

  if (!isnan(initialTemperature)) {
    temperatureC = initialTemperature;

    Serial.printf("TEMPERATURE_C,%.2f\n", temperatureC);
  } else {
    Serial.println("DHT11_READ_ERROR,initial_read");
  }

  lastTempRead = millis();

  if (esp_now_init() != 0) {
    Serial.println("ERROR,ESP_NOW_INIT_FAILED");
    return;
  }

  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
  esp_now_register_send_cb(onSent);
  esp_now_register_recv_cb(onReceive);

  if (!setPeer(1)) return;

  ready = true;

  Serial.println("ESP_NOW_READY,channel=1");
  Serial.println("COORDINATION_CHANNEL,1");

  lastScan = millis();
  lastData = millis();
  resumeDataAt = millis();

  if (startupTarget != 1)
    startSwitch(startupTarget);
  else
    Serial.println("ACTIVE_CHANNEL,1");
}


void loop() {
  if (!ready) {
    delay(100);
    return;
  }

  // Read DHT11 approximately every 2 seconds.
  // If a read fails, keep the previous valid temperature.
  if (millis() - lastTempRead >= TEMP_READ_MS) {
    lastTempRead = millis();

    float newTemperature = readTemperatureC();

    if (!isnan(newTemperature)) {
      temperatureC = newTemperature;

      Serial.printf("TEMPERATURE_C,%.2f\n", temperatureC);
    } else {
      Serial.println("DHT11_READ_ERROR,previous_value_retained");
    }
  }

  // Original request retry logic.
  if (pending &&
      !readyReceived &&
      millis() - lastRequest >= REQUEST_MS) {

    sendPacket(REQUEST, targetChannel, 0, "SWITCH_REQUEST");

    Serial.printf("SWITCH_REQUEST_RETRY,target=%u\n",
                  targetChannel);

    lastRequest = millis();
  }

  // Original coordinated switching logic.
  if (pending &&
      readyReceived &&
      (long)(millis() - switchAt) >= 0) {

    wifi_set_channel(targetChannel);
    delay(100);

    if (!setPeer(targetChannel)) {
      Serial.println("ERROR,PEER_UPDATE_FAILED");
      return;
    }

    currentChannel = targetChannel;
    pending = false;
    readyReceived = false;

    resumeDataAt = millis() + 1500;
    lastData = resumeDataAt;
    lastScan = millis();

    Serial.printf("ACTIVE_CHANNEL,%u\n", currentChannel);
  }

  // Original 30-second rescan logic.
  if (!pending &&
      millis() - lastScan >= RESCAN_MS) {

    lastScan = millis();

    uint8_t candidate = scanBestChannel();

    wifi_set_channel(currentChannel);
    delay(100);
    setPeer(currentChannel);

    if (candidate != currentChannel)
      startSwitch(candidate);
  }

  // Original telemetry transmission interval.
  if (!pending &&
      (long)(millis() - resumeDataAt) >= 0 &&
      millis() - lastData >= DATA_MS) {

    packetID++;

    char msg[32];
    snprintf(msg, sizeof(msg), "T=%.1fC", temperatureC);

    Serial.printf(
      "TX_PACKET,id=%lu,channel=%u,temp=%.2f\n",
      (unsigned long)packetID,
      currentChannel,
      temperatureC
    );

    sendPacket(DATA, currentChannel, packetID, msg);
    lastData = millis();
  }

  delay(5);
}

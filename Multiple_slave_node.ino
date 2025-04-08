#include <ArduinoBLE.h>

#define SERVICE_UUID              "87E01439-99BE-45AA-9410-DB4D3F23EA99"
#define SHOOT_CHARACTERISTIC_UUID "D90A7C02-9B21-4243-8372-3E523FA7978B"
#define COUNTER_ALL_UUID          "A1B2C3D4-E5F6-7890-1234-56789ABCDEF0"
#define COUNTER_ACC_UUID          "B2C3D4E5-F678-9012-3456-789ABCDEF012"
#define SOUND_LEVEL_UUID          "C3D4E5F6-7890-1234-5678-9ABCDEF01234"
#define SOUND_THRESHOLD_UUID      "D4E5F678-9012-3456-789A-BCDEF0123456"

BLEService customService(SERVICE_UUID);
BLECharacteristic shootCharacteristic(SHOOT_CHARACTERISTIC_UUID, BLERead | BLENotify, 1);
BLECharacteristic counterAllCharacteristic(COUNTER_ALL_UUID, BLERead | BLENotify, sizeof(int));
BLECharacteristic counterAccCharacteristic(COUNTER_ACC_UUID, BLERead | BLENotify | BLEWrite, sizeof(int));
BLECharacteristic soundLevelCharacteristic(SOUND_LEVEL_UUID, BLERead | BLENotify, sizeof(int));
BLECharacteristic soundThresholdCharacteristic(SOUND_THRESHOLD_UUID, BLERead | BLEWrite, sizeof(int));

const int MIC_PIN = A0;
int SOUND_THRESHOLD = 100;
int counterAll = 0;
int counterAcc = 0;
bool thresholdReceived = false;

// สำหรับ edge-trigger shoot
bool shootTriggered = false;
unsigned long lastShootTime = 0;

void onThresholdWrite(BLEDevice central, BLECharacteristic characteristic) {
  int newThreshold = 0;
  characteristic.readValue(&newThreshold, sizeof(newThreshold));
  SOUND_THRESHOLD = newThreshold;
  thresholdReceived = true;
  Serial.print("🔧 SOUND_THRESHOLD updated to: ");
  Serial.println(SOUND_THRESHOLD);
}

void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial.println("🚀 Starting Slave Node...");

  if (!BLE.begin()) {
    Serial.println("❌ Failed to start BLE!");
    while (1);
  }

  BLE.setLocalName("SLAVE-1"); // เปลี่ยนชื่อได้ตาม node
  BLE.setAdvertisedService(customService);

  customService.addCharacteristic(shootCharacteristic);
  customService.addCharacteristic(counterAllCharacteristic);
  customService.addCharacteristic(counterAccCharacteristic);
  customService.addCharacteristic(soundLevelCharacteristic);
  customService.addCharacteristic(soundThresholdCharacteristic);
  BLE.addService(customService);

  shootCharacteristic.setValue("0");
  counterAllCharacteristic.setValue((byte*)&counterAll, sizeof(counterAll));
  counterAccCharacteristic.setValue((byte*)&counterAcc, sizeof(counterAcc));
  soundLevelCharacteristic.setValue((byte*)&counterAll, sizeof(counterAll));
  soundThresholdCharacteristic.setValue((byte*)&SOUND_THRESHOLD, sizeof(SOUND_THRESHOLD));

  soundThresholdCharacteristic.setEventHandler(BLEWritten, onThresholdWrite);

  BLE.advertise();
  Serial.println("📡 Advertising BLE Service...");
}

void loop() {
  BLEDevice central = BLE.central();
  if (central) {
    Serial.print("✅ Connected to: ");
    Serial.println(central.address());

    while (central.connected()) {
      if (!thresholdReceived) {
        Serial.println("⏳ Waiting for SOUND_THRESHOLD from Master...");
        BLE.poll();
        delay(500);
        continue;
      }

      int soundLevel = analogRead(MIC_PIN);
      Serial.print("🔊 Sound Level: ");
      Serial.println(soundLevel);
      soundLevelCharacteristic.setValue((byte*)&soundLevel, sizeof(soundLevel));

      if (soundLevel > SOUND_THRESHOLD && !shootTriggered) {
        shootTriggered = true;
        lastShootTime = millis();

        Serial.println("🔥 ยิงแล้ว!");
        shootCharacteristic.setValue("1");

        counterAll++;
        counterAcc++;
        counterAllCharacteristic.setValue((byte*)&counterAll, sizeof(counterAll));
        counterAccCharacteristic.setValue((byte*)&counterAcc, sizeof(counterAcc));
      }

      if (shootTriggered && millis() - lastShootTime > 300) {
        shootCharacteristic.setValue("0");
        shootTriggered = false;
      }

      BLE.poll();
      delay(100); // เพิ่ม responsiveness
    }

    thresholdReceived = false;
    Serial.println("🔄 Disconnected. Restarting advertise...");
    BLE.advertise();
  }
}

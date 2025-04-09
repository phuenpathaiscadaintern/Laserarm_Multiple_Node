#include <ArduinoBLE.h>

  #define SERVICE_UUID              "87E01439-99BE-45AA-9410-DB4D3F23EA99"
  #define SHOOT_CHARACTERISTIC_UUID "D90A7C02-9B21-4243-8372-3E523FA7978B"
  #define COUNTER_ALL_UUID          "A1B2C3D4-E5F6-7890-1234-56789ABCDEF0"
  #define COUNTER_ACC_UUID          "B2C3D4E5-F678-9012-3456-789ABCDEF012"
  #define SOUND_LEVEL_UUID          "C3D4E5F6-7890-1234-5678-9ABCDEF01234"
  #define SOUND_THRESHOLD_UUID      "D4E5F678-9012-3456-789A-BCDEF0123456"
  #define START_CHARACTERISTIC_UUID "E5F6A1B2-C3D4-5678-9012-3456789ABCDE"

  BLEService customService(SERVICE_UUID);
  BLECharacteristic shootCharacteristic(SHOOT_CHARACTERISTIC_UUID, BLERead | BLENotify, 1);
  BLECharacteristic counterAllCharacteristic(COUNTER_ALL_UUID, BLERead | BLENotify, sizeof(int));
  BLECharacteristic counterAccCharacteristic(COUNTER_ACC_UUID, BLERead | BLENotify, sizeof(int));
  BLECharacteristic soundLevelCharacteristic(SOUND_LEVEL_UUID, BLERead | BLENotify, sizeof(int));
  BLECharacteristic soundThresholdCharacteristic(SOUND_THRESHOLD_UUID, BLERead | BLEWrite, sizeof(int));
  BLECharacteristic startCharacteristic(START_CHARACTERISTIC_UUID, BLERead | BLEWrite, 1);

  const int MIC_PIN = A0;
  int SOUND_THRESHOLD = 100;
  int counterAll = 0;
  int counterAcc = 0;
  bool shootTriggered = false;
  unsigned long lastShootTime = 0;
  bool started = false;

  void onThresholdWrite(BLEDevice central, BLECharacteristic characteristic) {
    characteristic.readValue(&SOUND_THRESHOLD, sizeof(SOUND_THRESHOLD));
    Serial.print("🔧 SOUND_THRESHOLD updated to: ");
    Serial.println(SOUND_THRESHOLD);
  }

  void setup() {
    Serial.begin(115200);
    if (!BLE.begin()) {
      Serial.println("❌ BLE init failed!");
      while (1);
    }

    BLE.setLocalName("SLAVE-1");
    BLE.setAdvertisedService(customService);

    customService.addCharacteristic(shootCharacteristic);
    customService.addCharacteristic(counterAllCharacteristic);
    customService.addCharacteristic(counterAccCharacteristic);
    customService.addCharacteristic(soundLevelCharacteristic);
    customService.addCharacteristic(soundThresholdCharacteristic);
    customService.addCharacteristic(startCharacteristic);

    BLE.addService(customService);

    soundThresholdCharacteristic.setEventHandler(BLEWritten, onThresholdWrite);

    // ตั้งค่าเริ่มต้น
    shootCharacteristic.setValue("0");
    startCharacteristic.setValue("0");

    BLE.advertise();
    Serial.println("📡 Advertising...");
  }

  void loop() {
    BLEDevice central = BLE.central();
    if (central) {
      Serial.print("✅ Connected to: ");
      Serial.println(central.address());

      while (central.connected()) {
        BLE.poll();

        byte startFlag = 0;
        startCharacteristic.readValue(&startFlag, sizeof(startFlag));
        if (startFlag == 1 && !started) {
          Serial.println("🚦 START received. Begin sensing...");
          started = true;
        }

        if (started) {
          int soundLevel = analogRead(MIC_PIN);
          soundLevelCharacteristic.setValue((byte*)&soundLevel, sizeof(soundLevel));
          Serial.print("🔊 Sound Level: ");
          Serial.println(soundLevel);

          if (soundLevel > SOUND_THRESHOLD && !shootTriggered) {
            shootTriggered = true;
            lastShootTime = millis();
            shootCharacteristic.setValue("1");

            counterAll++;
            counterAcc++;
            counterAllCharacteristic.setValue((byte*)&counterAll, sizeof(counterAll));
            counterAccCharacteristic.setValue((byte*)&counterAcc, sizeof(counterAcc));

            Serial.println("🔥 Triggered!");
          }

          if (shootTriggered && millis() - lastShootTime > 300) {
            shootCharacteristic.setValue("0");
            shootTriggered = false;
          }
        }

        delay(100);
      }

      // Reset state after disconnect
      started = false;
      startCharacteristic.setValue("0");
      shootCharacteristic.setValue("0");
      Serial.println("🔄 Disconnected. Advertising again...");
      BLE.advertise();
    }
  }

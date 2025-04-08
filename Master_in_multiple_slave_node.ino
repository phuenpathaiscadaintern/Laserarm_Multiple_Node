#include <ArduinoBLE.h>
#include <vector>

#define SERVICE_UUID              "87E01439-99BE-45AA-9410-DB4D3F23EA99"
#define SHOOT_CHARACTERISTIC_UUID "D90A7C02-9B21-4243-8372-3E523FA7978B"
#define COUNTER_ALL_UUID          "A1B2C3D4-E5F6-7890-1234-56789ABCDEF0"
#define COUNTER_ACC_UUID          "B2C3D4E5-F678-9012-3456-789ABCDEF012"
#define SOUND_LEVEL_UUID          "C3D4E5F6-7890-1234-5678-9ABCDEF01234"
#define SOUND_THRESHOLD_UUID      "D4E5F678-9012-3456-789A-BCDEF0123456"

struct SlaveDevice {
  BLEDevice peripheral;
  BLECharacteristic shootChar;
  BLECharacteristic counterAllChar;
  BLECharacteristic counterAccChar;
  BLECharacteristic soundLevelChar;
  BLECharacteristic soundThresholdChar;
  String name;
};

std::vector<SlaveDevice> slaveDevices;
String knownSlaves[] = {"SLAVE-1", "SLAVE-2", "SLAVE-3", "SLAVE-4"}; // ระบุชื่อที่เรารู้จัก

bool isKnownSlave(String name) {
  for (String known : knownSlaves) {
    if (name.equalsIgnoreCase(known)) return true;
  }
  return false;
}

void TaskBLEScan(void *pvParameters) {
  Serial.println("🔍 Scanning for BLE devices...");
  BLE.scan();
  unsigned long startTime = millis();

  while (millis() - startTime < 15000) {
    BLEDevice device = BLE.available();
    if (device) {
      String name = device.localName();
      Serial.print("📡 Found device: ");
      Serial.println(name);

      if (isKnownSlave(name)) {
        Serial.println("✅ Known device. Connecting to: " + name);
        BLE.stopScan();

        if (device.connect()) {
          Serial.println("🔗 Connected to: " + name);
          if (!device.discoverAttributes()) {
            Serial.println("❌ Failed to discover attributes!");
            continue;
          }

          BLEService service = device.service(SERVICE_UUID);
          if (!service) {
            Serial.println("❌ Service UUID not found");
            continue;
          }

          SlaveDevice slave;
          slave.peripheral = device;
          slave.name = name;
          slave.shootChar = service.characteristic(SHOOT_CHARACTERISTIC_UUID);
          slave.counterAllChar = service.characteristic(COUNTER_ALL_UUID);
          slave.counterAccChar = service.characteristic(COUNTER_ACC_UUID);
          slave.soundLevelChar = service.characteristic(SOUND_LEVEL_UUID);
          slave.soundThresholdChar = service.characteristic(SOUND_THRESHOLD_UUID);

          slaveDevices.push_back(slave);
          Serial.println("📥 Device added. Total connected: " + String(slaveDevices.size()));
        } else {
          Serial.println("❌ Connection failed.");
        }

        BLE.scan(); // กลับไป scan ตัวอื่นต่อ
      }
    }
    vTaskDelay(50 / portTICK_PERIOD_MS);
  }

  BLE.stopScan();
  Serial.println("🔍 Scan complete.");
  vTaskDelete(NULL);
}

void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial.println("🚀 Master Node Starting...");

  if (!BLE.begin()) {
    Serial.println("❌ BLE failed to initialize!");
    while (1);
  }

  BLE.setLocalName("ESP32-Master");
  xTaskCreatePinnedToCore(TaskBLEScan, "BLEScan", 8192, NULL, 1, NULL, 1);
}

void loop() {
  for (auto &slave : slaveDevices) {
    if (slave.peripheral.connected()) {
      Serial.println("📡 Reading from: " + slave.name);

      char shotStatus[2] = {0};
      if (slave.shootChar.canRead()) {
        int len = slave.shootChar.readValue(shotStatus, sizeof(shotStatus) - 1);
        shotStatus[len] = '\0';
        Serial.print("📩 Shot Status: ");
        Serial.println(shotStatus);
      }

      int value = 0;
      if (slave.counterAllChar.canRead()) {
        slave.counterAllChar.readValue(&value, sizeof(value));
        Serial.print("🎯 Total Shots: ");
        Serial.println(value);
      }

      if (slave.counterAccChar.canRead()) {
        slave.counterAccChar.readValue(&value, sizeof(value));
        Serial.print("🎯 Shots Since Reset: ");
        Serial.println(value);
      }

      if (slave.soundLevelChar.canRead()) {
        slave.soundLevelChar.readValue(&value, sizeof(value));
        Serial.print("🔊 Sound Level: ");
        Serial.println(value);
      }

      if (Serial.available()) {
        int newThreshold = Serial.parseInt();
        if (newThreshold > 0) {
          slave.soundThresholdChar.writeValue(newThreshold);
          Serial.print("📡 Sent new SOUND_THRESHOLD to ");
          Serial.print(slave.name);
          Serial.print(": ");
          Serial.println(newThreshold);
        }
      }
    }
  }

  delay(1000);
}

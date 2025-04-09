#include <ArduinoBLE.h>

#define SERVICE_UUID              "87E01439-99BE-45AA-9410-DB4D3F23EA99"
#define SOUND_LEVEL_UUID          "C3D4E5F6-7890-1234-5678-9ABCDEF01234"
#define SOUND_THRESHOLD_UUID      "D4E5F678-9012-3456-789A-BCDEF0123456"
#define START_CHARACTERISTIC_UUID "E5F6A1B2-C3D4-5678-9012-3456789ABCDE"
#define COUNTER_ALL_UUID          "A1B2C3D4-E5F6-7890-1234-56789ABCDEF0"
#define COUNTER_ACC_UUID          "B2C3D4E5-F678-9012-3456-789ABCDEF012"

#define MAX_SLAVES 10 // can change to n numbers.

struct SlaveNode {
  String name;
  BLEDevice device;
  BLECharacteristic startChar;
  BLECharacteristic thresholdChar;
  BLECharacteristic soundLevelChar;
  BLECharacteristic counterAllChar;
  BLECharacteristic counterAccChar;
};

SlaveNode connectedSlaves[MAX_SLAVES];
bool thresholdsReceived[MAX_SLAVES] = {false};
int numSlaves = 0;
bool started = false;

void scanAndConnectSlaves() {
  BLE.scan();
  Serial.println("🔄 Scanning for SLAVE-* for 15s...");

  unsigned long startTime = millis();
  while (millis() - startTime < 15000) {
    BLEDevice dev = BLE.available();
    if (dev) {
      String devName = dev.localName();
      if (devName.startsWith("SLAVE-")) {
        Serial.println("🔗 Connecting to: " + devName);
        BLE.stopScan();
        if (dev.connect()) {
          if (!dev.discoverAttributes()) {
            Serial.println("❌ Discover attributes failed.");
            return;
          }

          BLEService service = dev.service(SERVICE_UUID);
          if (!service) return;

          BLECharacteristic thresholdChar = service.characteristic(SOUND_THRESHOLD_UUID);
          BLECharacteristic startChar = service.characteristic(START_CHARACTERISTIC_UUID);
          BLECharacteristic soundLevelChar = service.characteristic(SOUND_LEVEL_UUID);
          BLECharacteristic counterAllChar = service.characteristic(COUNTER_ALL_UUID);
          BLECharacteristic counterAccChar = service.characteristic(COUNTER_ACC_UUID);

          if (thresholdChar && startChar && soundLevelChar && counterAllChar && counterAccChar) {
            connectedSlaves[numSlaves++] = { devName, dev, startChar, thresholdChar, soundLevelChar, counterAllChar, counterAccChar };
            Serial.println("📥 Slave added: " + devName);
          }
        }
        BLE.scan();
      }
    }
    delay(200);
  }

  BLE.stopScan();
}

void sendStartSignal() {
  Serial.println("🚦 Sending START signal to all slaves...");
  for (int i = 0; i < numSlaves; i++) {
    connectedSlaves[i].startChar.writeValue(1);
    Serial.print("✅ START sent to ");
    Serial.println(connectedSlaves[i].name);
  }
  started = true;
}

void pollSlaves() {
  for (int i = 0; i < numSlaves; i++) {
    int soundLevel = 0;
    int counterAll = 0;
    int counterAcc = 0;

    // อ่านค่า soundLevel จาก Slave
    if (connectedSlaves[i].soundLevelChar.readValue((byte*)&soundLevel, sizeof(soundLevel))) {
      Serial.print("📥 ");
      Serial.print(connectedSlaves[i].name);
      Serial.print(" Sound Level: ");
      Serial.println(soundLevel);
    }

    // อ่านค่า counterAll จาก Slave
    if (connectedSlaves[i].counterAllChar.readValue((byte*)&counterAll, sizeof(counterAll))) {
      Serial.print("📥 ");
      Serial.print(connectedSlaves[i].name);
      Serial.print(" Counter All: ");
      Serial.println(counterAll);
    }

    // อ่านค่า counterAcc จาก Slave
    if (connectedSlaves[i].counterAccChar.readValue((byte*)&counterAcc, sizeof(counterAcc))) {
      Serial.print("📥 ");
      Serial.print(connectedSlaves[i].name);
      Serial.print(" Counter Acc: ");
      Serial.println(counterAcc);
    }
  }
}

void setup() {
  Serial.begin(115200);
  if (!BLE.begin()) {
    Serial.println("❌ BLE init failed!");
    while (1);
  }
  scanAndConnectSlaves();
  Serial.println("📨 Input like SLAVE-1:120 to set thresholds.");
}

void loop() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();

    if (input.indexOf(':') > 0) {
      String name = input.substring(0, input.indexOf(':'));
      int value = input.substring(input.indexOf(':') + 1).toInt();

      for (int i = 0; i < numSlaves; i++) {
        if (connectedSlaves[i].name == name) {
          connectedSlaves[i].thresholdChar.writeValue(value);
          thresholdsReceived[i] = true;
          Serial.println("✅ Threshold set for " + name);
        }
      }

      // Check if all thresholds set
      bool allSet = true;
      for (int i = 0; i < numSlaves; i++) {
        if (!thresholdsReceived[i]) {
          allSet = false;
          break;
        }
      }

      if (allSet && !started) {
        sendStartSignal();
      }
    } else {
      Serial.println("❌ Format: SLAVE-#:value");
    }
  }

  if (started) {
    pollSlaves();
  }

  delay(500);
}

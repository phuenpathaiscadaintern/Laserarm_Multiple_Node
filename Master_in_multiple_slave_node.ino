#include <ArduinoBLE.h>

#define SERVICE_UUID              "87E01439-99BE-45AA-9410-DB4D3F23EA99"
#define SHOOT_CHARACTERISTIC_UUID "D90A7C02-9B21-4243-8372-3E523FA7978B"
#define COUNTER_ALL_UUID          "A1B2C3D4-E5F6-7890-1234-56789ABCDEF0"
#define COUNTER_ACC_UUID          "B2C3D4E5-F678-9012-3456-789ABCDEF012"
#define SOUND_LEVEL_UUID          "C3D4E5F6-7890-1234-5678-9ABCDEF01234"
#define SOUND_THRESHOLD_UUID      "D4E5F678-9012-3456-789A-BCDEF0123456"

const char* TARGET_ADDRESSES[] = {
  "54:91:e9:a3:b9:b8",  // ตัวอย่าง Slave-1
  "54:91:e9:a3:b9:cd"   // ตัวอย่าง Slave-2
};
const int NUM_SLAVES = sizeof(TARGET_ADDRESSES) / sizeof(TARGET_ADDRESSES[0]);
const int SOUND_THRESHOLD = 150;

struct SlaveNode {
  String address;
  BLEDevice device;
  BLECharacteristic shootChar;
  BLECharacteristic counterAllChar;
  BLECharacteristic counterAccChar;
  BLECharacteristic soundLevelChar;
  BLECharacteristic thresholdChar;
  bool connected = false;
};

SlaveNode slaves[NUM_SLAVES];

void connectToSlave(int index) {
  Serial.print("🔍 Scanning for: ");
  Serial.println(slaves[index].address);

  BLE.scan();
  unsigned long startTime = millis();
  while (millis() - startTime < 10000) {
    BLEDevice foundDevice = BLE.available();
    if (foundDevice) {
      String addr = String(foundDevice.address());
      if (addr.equalsIgnoreCase(slaves[index].address)) {
        Serial.println("📡 Found target, connecting...");
        BLE.stopScan();
        if (foundDevice.connect()) {
          Serial.println("✅ Connected to: " + addr);
          slaves[index].device = foundDevice;
          slaves[index].connected = true;

          if (!foundDevice.discoverAttributes()) {
            Serial.println("❌ Failed to discover attributes!");
            return;
          }

          BLEService service = foundDevice.service(SERVICE_UUID);
          if (!service) {
            Serial.println("❌ Service not found!");
            return;
          }

          slaves[index].shootChar       = service.characteristic(SHOOT_CHARACTERISTIC_UUID);
          slaves[index].counterAllChar  = service.characteristic(COUNTER_ALL_UUID);
          slaves[index].counterAccChar  = service.characteristic(COUNTER_ACC_UUID);
          slaves[index].soundLevelChar  = service.characteristic(SOUND_LEVEL_UUID);
          slaves[index].thresholdChar   = service.characteristic(SOUND_THRESHOLD_UUID);

          // ส่งค่า threshold ทันทีหลังเชื่อมต่อ
          if (slaves[index].thresholdChar.canWrite()) {
            slaves[index].thresholdChar.writeValue(SOUND_THRESHOLD);
            Serial.print("📤 Sent SOUND_THRESHOLD to ");
            Serial.println(addr);
          }

          return;
        } else {
          Serial.println("❌ Failed to connect to: " + addr);
        }
      }
    }
    delay(200);
  }

  BLE.stopScan();
  Serial.println("⌛ Timeout trying to connect to: " + slaves[index].address);
}

void setup() {
  Serial.begin(115200);
  Serial.println("🚀 Starting BLE Multi-Slave Master");

  if (!BLE.begin()) {
    Serial.println("❌ BLE failed to initialize!");
    while (1);
  }

  // ตั้งค่า address
  for (int i = 0; i < NUM_SLAVES; i++) {
    slaves[i].address = TARGET_ADDRESSES[i];
  }

  // เชื่อมต่อทุก slave ทีละตัว
  for (int i = 0; i < NUM_SLAVES; i++) {
    connectToSlave(i);
    delay(1000); // ให้เวลา BLE stack
  }

  Serial.print("📡 Total connected: ");
  int total = 0;
  for (int i = 0; i < NUM_SLAVES; i++) {
    if (slaves[i].connected) total++;
  }
  Serial.println(total);
}

void loop() {
  for (int i = 0; i < NUM_SLAVES; i++) {
    if (slaves[i].connected && slaves[i].device.connected()) {
      char shootStatus[2] = {0};
      if (slaves[i].shootChar.canRead()) {
        slaves[i].shootChar.readValue(shootStatus, sizeof(shootStatus) - 1);
        Serial.print("📩 [");
        Serial.print(slaves[i].address);
        Serial.print("] Shoot: ");
        Serial.println(shootStatus);
      }

      int counterAll = 0;
      if (slaves[i].counterAllChar.canRead()) {
        slaves[i].counterAllChar.readValue(&counterAll, sizeof(counterAll));
        Serial.print("🎯 [");
        Serial.print(slaves[i].address);
        Serial.print("] Total: ");
        Serial.println(counterAll);
      }

      int soundLevel = 0;
      if (slaves[i].soundLevelChar.canRead()) {
        slaves[i].soundLevelChar.readValue(&soundLevel, sizeof(soundLevel));
        Serial.print("🔊 [");
        Serial.print(slaves[i].address);
        Serial.print("] Sound: ");
        Serial.println(soundLevel);
      }
    }
  }

  delay(1000); // หรือปรับตามความต้องการ refresh rate
}

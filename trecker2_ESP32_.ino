#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

#define BUZZER_PIN 13
#define SERVICE_UUID "12345678-1234-1234-1234-123456789abc"
#define CHARACTERISTIC_UUID "abcdef12-1234-1234-1234-123456789abc"
#define BUTTON_CHARACTERISTIC_UUID "abcdef13-1234-1234-1234-123456789abc"

void triggerAlarm();
void manualBuzzer();

BLEServer *pServer;
bool deviceConnected = false;
bool alarmTriggered = false;
bool alarmEnabled = true;

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      alarmTriggered = false;
      Serial.println("Device Connected");
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println("Device Disconnected");
      
      if (alarmEnabled) {
        triggerAlarm();
      } else {
        Serial.println("Alarm disabled - no trigger on disconnect");
      }
      
      BLEDevice::startAdvertising();
    }
};

class ButtonCharacteristicCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      String value = pCharacteristic->getValue();
      
      if (value.length() > 0) {
        char command = value[0];
        Serial.print("Received command: ");
        Serial.println(command);
        
        if (command == '1') {
          Serial.println("Manual buzzer activation");
          manualBuzzer();
        }
        else if (command == '0') {
          Serial.println("Manual buzzer deactivation");
          digitalWrite(BUZZER_PIN, LOW);
          alarmTriggered = false;
        }
        else if (command == '2') {
          alarmEnabled = true;
          Serial.println("Alarm ENABLED - will trigger on disconnect");
        }
        else if (command == '3') {
          alarmEnabled = false;
          Serial.println("Alarm DISABLED - will NOT trigger on disconnect");
        }
      }
    }
};

void setup() {
  Serial.begin(115200);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  BLEDevice::init("Tracker");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);
  
  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );

  BLECharacteristic *pButtonCharacteristic = pService->createCharacteristic(
                                         BUTTON_CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_WRITE
                                       );
  
  pButtonCharacteristic->setCallbacks(new ButtonCharacteristicCallbacks());
 
  pCharacteristic->setValue("Tracker Ready");
  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();

  Serial.println("Waiting for a connection...");
  Serial.println("Commands: '1'-buzzer, '0'-stop, '2'-enable alarm, '3'-disable alarm");
  Serial.println("Alarm is currently ENABLED");
}

void loop() {
  if (deviceConnected) { 
    delay(1000);
  }
  
  if (Serial.available()) {
    char command = Serial.read();
    if (command == '1') {
      manualBuzzer();
    } else if (command == '0') {
      digitalWrite(BUZZER_PIN, LOW);
      alarmTriggered = false;
    } else if (command == '2') {
      alarmEnabled = true;
      Serial.println("Alarm ENABLED");
    } else if (command == '3') {
      alarmEnabled = false;
      Serial.println("Alarm DISABLED");
    } else if (command == 't') {
      triggerAlarm();
    } else if (command == 's') {
      // Статус системы
      Serial.print("Device connected: ");
      Serial.println(deviceConnected ? "YES" : "NO");
      Serial.print("Alarm enabled: ");
      Serial.println(alarmEnabled ? "YES" : "NO");
      Serial.print("Alarm triggered: ");
      Serial.println(alarmTriggered ? "YES" : "NO");
    }
  }
}

void triggerAlarm() {
  if (alarmTriggered) return;
  
  Serial.println("ALARM! Device is out of range!");
  alarmTriggered = true;
  
  for (int i = 0; i < 10; i++) {
    if (!alarmTriggered) break;
    digitalWrite(BUZZER_PIN, HIGH);
    delay(200);
    digitalWrite(BUZZER_PIN, LOW);
    delay(200);
  }
  alarmTriggered = false;
}

void manualBuzzer() {
  for (int i = 0; i < 5; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(500);
    digitalWrite(BUZZER_PIN, LOW);
    delay(300);
  }
}

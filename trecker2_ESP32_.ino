#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

// Настройки
#define BUZZER_PIN 13
#define SERVICE_UUID "12345678-1234-1234-1234-123456789abc"
#define CHARACTERISTIC_UUID "abcdef12-1234-1234-1234-123456789abc"
#define BUTTON_CHARACTERISTIC_UUID "abcdef13-1234-1234-1234-123456789abc"

// Объявление функций ДО их использования
void triggerAlarm();
void manualBuzzer();

BLEServer *pServer;
bool deviceConnected = false;
bool alarmTriggered = false;
bool alarmEnabled = true; // Флаг для включения/выключения тревоги

// Класс для обработки событий подключения/отключения
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      alarmTriggered = false;
      Serial.println("Device Connected");
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println("Device Disconnected");
      
      // Тревога срабатывает ТОЛЬКО если включена
      if (alarmEnabled) {
        triggerAlarm();
      } else {
        Serial.println("Alarm disabled - no trigger on disconnect");
      }
      
      // Перезапускаем рекламу
      BLEDevice::startAdvertising();
    }
};

// Класс для обработки команд от приложения
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
          // Включить тревогу при отключении
          alarmEnabled = true;
          Serial.println("Alarm ENABLED - will trigger on disconnect");
        }
        else if (command == '3') {
          // Выключить тревогу при отключении
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

  // Создаем BLE-устройство
  BLEDevice::init("Tracker");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Создаем BLE-сервис
  BLEService *pService = pServer->createService(SERVICE_UUID);
  
  // Создаем характеристику для данных
  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );

  // Создаем характеристику для кнопки
  BLECharacteristic *pButtonCharacteristic = pService->createCharacteristic(
                                         BUTTON_CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_WRITE
                                       );
  
  pButtonCharacteristic->setCallbacks(new ButtonCharacteristicCallbacks());
  
  // Начальное значение характеристики
  pCharacteristic->setValue("Tracker Ready");
  pService->start();

  // Настройка рекламирования
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
    // Периодическая проверка соединения
    delay(1000);
  }
  
  // Обработка команд из Serial для тестирования
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

// Реализация функций ПОСЛЕ их объявления
void triggerAlarm() {
  if (alarmTriggered) return; // Уже в режиме тревоги
  
  Serial.println("ALARM! Device is out of range!");
  alarmTriggered = true;
  
  for (int i = 0; i < 10; i++) {
    if (!alarmTriggered) break; // Проверка отмены тревоги
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

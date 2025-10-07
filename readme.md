# Documentação de Códigos Arduino/ESP32

Este documento fornece a documentação para dois projetos Arduino/ESP32: um para controle de um display LCD I2C e outro para leitura de cartões RFID.

## 1. Projeto: Display LCD I2C com ESP32

### Descrição

Este código demonstra como inicializar e exibir uma mensagem em um display LCD 16x2 via interface I2C utilizando um ESP32 DevKit V1. Ele exibe as mensagens "Hello, World!" e "ESP32 DevKit V1" nas duas linhas do display.

### Bibliotecas Necessárias

*   `Wire.h`: Para comunicação I2C.
*   `LiquidCrystal_I2C.h`: Para controle do display LCD I2C. Você pode instalá-la através do Gerenciador de Bibliotecas do Arduino IDE (pesquise por "LiquidCrystal I2C").

### Pinagem (ESP32 DevKit V1)

O código configura a comunicação I2C nos pinos 21 (SDA) e 22 (SCL) do ESP32.

*   **SDA**: GPIO 21
*   **SCL**: GPIO 22

Certifique-se de conectar o display LCD I2C a esses pinos.

### Endereço I2C do LCD

O endereço I2C padrão no código é `0x27`. Se o seu display tiver um endereço diferente, você precisará alterá-lo na linha:

```cpp
LiquidCrystal_I2C lcd(0x27, 16, 2); // troque pelo endereço correto
```

### Código (`lcd.ino`)

```cpp
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2); // troque pelo endereço correto

void setup() {
  Wire.begin(21, 22);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Hello, World!");
  lcd.setCursor(0, 1);
  lcd.print("ESP32 DevKit V1");
}

void loop() {}
```

## 2. Projeto: Leitor RFID com ESP32

### Descrição

Este código implementa um leitor de RFID utilizando o módulo MFRC522 e um ESP32. Ele inicializa a comunicação SPI, detecta a presença de novos cartões RFID, lê seus UIDs (Unique Identifiers) e os exibe no Monitor Serial.

### Bibliotecas Necessárias

*   `SPI.h`: Para comunicação Serial Peripheral Interface (SPI).
*   `MFRC522.h`: Para interface com o módulo RFID MFRC522. Você pode instalá-la através do Gerenciador de Bibliotecas do Arduino IDE (pesquise por "MFRC522").

### Pinagem (ESP32 DevKit V1 para MFRC522)

O código define os seguintes pinos para a comunicação SPI com o módulo MFRC522:

*   **SS_PIN (SDA/SS)**: GPIO 5
*   **RST_PIN**: GPIO 4
*   **MISO_PIN**: GPIO 19
*   **MOSI_PIN**: GPIO 23
*   **SCK_PIN**: GPIO 18

Certifique-se de conectar o módulo MFRC522 a esses pinos do seu ESP32.

### Como Usar

1.  Faça o upload do código para o seu ESP32.
2.  Abra o Monitor Serial na IDE do Arduino (ou outro terminal serial) com a taxa de transmissão de `115200` baud.
3.  Aproxime um cartão RFID (como um cartão MIFARE Classic) do módulo MFRC522.
4.  O UID do cartão será exibido no Monitor Serial.

### Código (`rfid.ino`)

```cpp
#include <SPI.h>
#include <MFRC522.h>

// ==== DEFINIÇÃO DOS PINOS ====
#define SS_PIN 5 // SDA/SS do MFRC522
#define RST_PIN 4 // Pino RST (pode trocar por 4, se preferir)
#define MISO_PIN 19 // SPI MISO
#define MOSI_PIN 23 // SPI MOSI
#define SCK_PIN 18 // SPI SCK

// ==== OBJETO DO RFID ====
MFRC522 mfrc522(SS_PIN, RST_PIN);

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);

  // Inicializa o barramento SPI com pinos definidos
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, SS_PIN);

  // Inicializa o módulo RFID
  mfrc522.PCD_Init();

  Serial.println("📡 Leitor RFID iniciado!");
  Serial.println("Aproxime um cartão para leitura...");
}

void loop() {
  // Verifica se há um novo cartão presente
  if (!mfrc522.PICC_IsNewCardPresent()) return;
  if (!mfrc522.PICC_ReadCardSerial()) return;

  // Exibe UID do cartão
  Serial.print("🎫 UID: ");
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(mfrc522.uid.uidByte[i], HEX);
  }
  Serial.println();

  // Encerra comunicação com o cartão
  mfrc522.PICC_HaltA();
  delay(500);
}
```


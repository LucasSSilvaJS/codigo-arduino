#include <SPI.h>
#include <MFRC522.h>

// ==== DEFINIÇÃO DOS PINOS ====
#define SS_PIN   5     // SDA/SS do MFRC522
#define RST_PIN  4    // Pino RST (pode trocar por 4, se preferir)
#define MISO_PIN 19    // SPI MISO
#define MOSI_PIN 23    // SPI MOSI
#define SCK_PIN  18    // SPI SCK

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

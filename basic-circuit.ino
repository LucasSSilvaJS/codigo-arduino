#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>

// ==== LCD I2C ====
#define SDA_PIN 21
#define SCL_PIN 22
LiquidCrystal_I2C lcd(0x27, 16, 2); // Altere o endereço se necessário (0x27 ou 0x3F)

// ==== RFID MFRC522 ====
#define SS_PIN   5     // SDA/SS
#define RST_PIN  4     // RST
#define MISO_PIN 19
#define MOSI_PIN 23
#define SCK_PIN  18

MFRC522 mfrc522(SS_PIN, RST_PIN);

// ==== SETUP ====
void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);

  // Inicializa o LCD
  Wire.begin(SDA_PIN, SCL_PIN);
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Iniciando...");
  
  // Inicializa SPI e RFID
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, SS_PIN);
  mfrc522.PCD_Init();

  delay(1000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("RFID pronto!");
  lcd.setCursor(0, 1);
  lcd.print("Aprox. cartao");

  Serial.println("📡 Leitor RFID iniciado!");
  Serial.println("Aproxime um cartão para leitura...");
}

// ==== LOOP ====
void loop() {
  // Verifica se há novo cartão
  if (!mfrc522.PICC_IsNewCardPresent()) return;
  if (!mfrc522.PICC_ReadCardSerial()) return;

  // Mostra UID no Serial
  Serial.print("🎫 UID: ");
  String uidString = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(mfrc522.uid.uidByte[i], HEX);
    uidString += String(mfrc522.uid.uidByte[i] < 0x10 ? "0" : "");
    uidString += String(mfrc522.uid.uidByte[i], HEX);
  }
  Serial.println();

  // Mostra UID no LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Cartao lido:");
  lcd.setCursor(0, 1);
  lcd.print(uidString.substring(0, 16)); // Limita a 16 caracteres
  delay(2000);

  // Retorna à tela inicial
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Aprox. cartao");
  
  // Encerra comunicação com o cartão
  mfrc522.PICC_HaltA();
  delay(500);
}

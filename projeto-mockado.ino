#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>

// =========================
// ==== CONFIGURAÇÕES LCD ===
// =========================
#define SDA_PIN 21
#define SCL_PIN 22
LiquidCrystal_I2C lcd(0x27, 16, 2); // Endereço I2C do LCD

// ===========================
// ==== CONFIGURAÇÕES RFID ====
// ===========================
// Leitor 1 → AGORA É O "NÃO"
#define SS_PIN_NAO 5
#define RST_PIN_NAO 4
// Leitor 2 → AGORA É O "SIM"
#define SS_PIN_SIM 25
#define RST_PIN_SIM 26
// Barramento SPI
#define MISO_PIN 19
#define MOSI_PIN 23
#define SCK_PIN  18

MFRC522 rfidSim(SS_PIN_SIM, RST_PIN_SIM);
MFRC522 rfidNao(SS_PIN_NAO, RST_PIN_NAO);

// ========================
// ==== VARIÁVEIS GLOBAIS ===
// ========================
String usuarioUID = "";
String voto = "";
String pergunta = "";

// Lista de perguntas (32 caracteres = 16 + 16)
const String perguntas[] = {
  "Recife melhora com gestao atual?",
  "Voce confia nos vereadores locais?",
  "A mobilidade em Recife esta boa?",
  "A saude publica tem melhorado?",
  "A educacao em Recife avanca?",
  "A coleta de lixo e eficiente?",
  "O turismo da cidade cresce?",
  "As pracas estao bem cuidadas?",
  "A gestao e transparente?",
  "A violencia tem diminuido?",
  "O transporte pub esta eficaz?",
  "Voce apoia os projetos novos?",
  "A orla esta mais valorizada?",
  "Os bairros estao mais seguros?",
  "O centro merece mais atencao?",
  "A cidade tem boa acessibilidade?"
};

// ========================
// ==== FUNÇÕES AUXILIARES ===
// ========================

// Exibe mensagem no LCD (duas linhas)
void mostrarMensagem(const String &linha1, const String &linha2, int tempo = 0) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(linha1.substring(0, 16));
  lcd.setCursor(0, 1);
  lcd.print(linha2.substring(0, 16));
  if (tempo > 0) delay(tempo);
}

// Lê cartão de um leitor específico
String lerCartao(MFRC522 &rfid) {
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) return "";
  String uid = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    uid += String(rfid.uid.uidByte[i] < 0x10 ? "0" : "");
    uid += String(rfid.uid.uidByte[i], HEX);
  }
  rfid.PICC_HaltA();
  return uid;
}

// Escolhe pergunta aleatória
String perguntaAleatoria() {
  int total = sizeof(perguntas) / sizeof(perguntas[0]);
  return perguntas[random(total)];
}

// Gera resultados falsos (percentuais)
void mostrarResultadoFake() {
  int sim = random(40, 90);
  int nao = 100 - sim;

  mostrarMensagem("Resultado:", "Sim:" + String(sim) + "% Nao:" + String(nao) + "%");
  delay(4000);
}

// ========================
// ==== FLUXO PRINCIPAL ====
// ========================
void setup() {
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);
  lcd.init();
  lcd.backlight();

  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, SS_PIN_SIM);
  rfidSim.PCD_Init();
  rfidNao.PCD_Init();

  randomSeed(analogRead(0));

  mostrarMensagem("Bussula Digital", "Aprox. VEM");
}

void loop() {
  // === Etapa 1: Identificação do usuário ===
  if (usuarioUID == "") {
    String uidSim = lerCartao(rfidSim);
    String uidNao = lerCartao(rfidNao);
    if (uidSim != "") usuarioUID = uidSim;
    if (uidNao != "") usuarioUID = uidNao;

    if (usuarioUID != "") {
      Serial.println("Usuario identificado: " + usuarioUID);
      mostrarMensagem("Usuario", "Identificado");
      delay(1000);
      mostrarMensagem("UID:", usuarioUID.substring(0, 16), 2000);

      pergunta = perguntaAleatoria();
      mostrarMensagem(pergunta.substring(0, 16), pergunta.substring(16), 1000);
    }
  }

  // === Etapa 2: Leitura do voto ===
  else if (voto == "") {
    String uidSim = lerCartao(rfidSim);
    String uidNao = lerCartao(rfidNao);

    // Confere se o mesmo usuário votou
    if (uidSim == usuarioUID) {
      voto = "Sim";
    } else if (uidNao == usuarioUID) {
      voto = "Nao";
    }

    if (voto != "") {
      Serial.println("Voto registrado: " + voto);
      mostrarMensagem("Voto registrado:", voto, 1500);
      mostrarResultadoFake();
      mostrarMensagem("Nova votacao", "Aprox. VEM", 1500);

      // Reinicia variáveis
      usuarioUID = "";
      voto = "";
    }
  }
}

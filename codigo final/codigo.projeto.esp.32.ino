#include <WiFi.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>

// ========================
// ==== CONFIG WIFI =======
const char* ssid = "Lucas";
const char* password = "lucas2025";
const String API_BASE = "https://projeto-bigdata.onrender.com";
const String TOTEM_ID = "5e652a794087";

// ========================
// ==== CONFIG SERIAL COM DUE ====
// Serial2 para comunicação com Arduino Due
// TX2 (pino 17) -> RX1 do Due
// RX2 (pino 16) -> TX1 do Due
#define SERIAL_DUE Serial2
#define BAUD_RATE_DUE 115200

// ========================
// ==== CONFIG RFID =======
#define SS_PIN_NAO 5
#define RST_PIN_NAO 4
#define SS_PIN_SIM 25
#define RST_PIN_SIM 26
#define MISO_PIN 19
#define MOSI_PIN 23
#define SCK_PIN 18

MFRC522 rfidSim(SS_PIN_SIM, RST_PIN_SIM);
MFRC522 rfidNao(SS_PIN_NAO, RST_PIN_NAO);

// ========================
// ==== CONFIG LEDS =======
// LEDs verdes (para voto "sim") 
#define LED_VERDE_1 14
#define LED_VERDE_2 27
// LEDs vermelhos (para voto "não")
#define LED_VERMELHO_1 2
#define LED_VERMELHO_2 13

// ========================
// ==== CONFIG BUZZER =====
#define BUZZER_PIN 32
int tone1 = 1437;
int tone2 = 1337;

// ========================
// ==== VARIAVEIS =========
String usuarioUID = "";
String voto = "";
String pergunta = "";
String pergunta_id = "";
int pontuacaoAtual = 0;

// ========================
// ==== ESTADOS ===========
enum Estado { ESPERA_CARTAO, VERIFICANDO_USUARIO, CADASTRANDO, PERGUNTA, AGUARDANDO_VOTO, RESULTADO };
Estado estado = ESPERA_CARTAO;

// ========================
// ==== FUNÇÕES COMUNICAÇÃO COM DUE ====
void enviarComandoDue(const String &comando, const String &dado1 = "", const String &dado2 = "") {
  String mensagem = "TELA:" + comando;
  if(dado1 != "") {
    mensagem += "|" + dado1;
    if(dado2 != "") {
      mensagem += "|" + dado2;
    }
  }
  mensagem += "\n";
  SERIAL_DUE.print(mensagem);
  SERIAL_DUE.flush();
  String logMsg = mensagem;
  logMsg.trim();
  Serial.println("[ESP32->Due] " + logMsg);
  delay(10); // Pequeno delay para garantir envio
}

// ========================
// ==== FUNÇÕES AUX =======
String lerCartao(MFRC522 &rfid) {
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) return "";
  String uid = "";
  for(byte i=0;i<rfid.uid.size;i++){
    uid += String(rfid.uid.uidByte[i]<0x10 ? "0":"");
    uid += String(rfid.uid.uidByte[i], HEX);
  }
  rfid.PICC_HaltA();
  return uid;
}

void mostrarTelaInicial() {
  usuarioUID = "";
  voto = "";
  pergunta = "";
  pergunta_id = "";
  pontuacaoAtual = 0;
  estado = ESPERA_CARTAO;
  enviarComandoDue("INICIAL");
}

void tratarErro(const String &estadoErro){
  enviarComandoDue("ERRO", estadoErro);
  delay(2000);
  mostrarTelaInicial();
}

// ========================
// ==== FUNÇÃO SOM URNA ===
void somUrna() {
  delay(300);
  for (int i = 0; i < 5; i++) {
    tone(BUZZER_PIN, tone2);
    delay(90);
    tone(BUZZER_PIN, tone1);
    delay(90);
  }
  tone(BUZZER_PIN, tone2);
  delay(120);
  noTone(BUZZER_PIN);
}

// ========================
// ==== SOM CONFIRMAÇÃO ===
// LED + som juntos
// ========================

// Som e LED para confirmação "SIM"
void somConfirmacaoSim() {
  digitalWrite(LED_VERDE_1, HIGH);
  digitalWrite(LED_VERDE_2, HIGH);
  tone(BUZZER_PIN, tone1);
  delay(700);
  noTone(BUZZER_PIN);
  digitalWrite(LED_VERDE_1, LOW);
  digitalWrite(LED_VERDE_2, LOW);
}

// Som e LED para confirmação "NÃO"
void somConfirmacaoNao() {
  digitalWrite(LED_VERMELHO_1, HIGH);
  digitalWrite(LED_VERMELHO_2, HIGH);
  for (int i = 0; i < 2; i++) {
    tone(BUZZER_PIN, tone2);
    delay(300);
    noTone(BUZZER_PIN);
    delay(150);
  }
  digitalWrite(LED_VERMELHO_1, LOW);
  digitalWrite(LED_VERMELHO_2, LOW);
}

// ========================
// ==== WIFI ===============
bool conectarWiFi() {
  WiFi.begin(ssid,password);
  enviarComandoDue("CARREGANDO", "WiFi conectando");
  int tentativas = 0;
  while(WiFi.status()!=WL_CONNECTED && tentativas<20){
    delay(200);
    tentativas++;
  }
  if(WiFi.status()==WL_CONNECTED){
    Serial.println("WiFi conectado: "+WiFi.localIP().toString());
    mostrarTelaInicial();
    return true;
  } else {
    Serial.println("Erro WiFi");
    enviarComandoDue("ERRO", "Erro WiFi");
    delay(2000);
    mostrarTelaInicial();
    return false;
  }
}

// ========================
// ==== USUÁRIO ===========
bool usuarioExiste(const String &vem_hash){
  if(WiFi.status()!=WL_CONNECTED) return false;
  WiFiClientSecure client; client.setInsecure();
  HTTPClient http;
  String url = API_BASE+"/usuarios/"+vem_hash;
  http.begin(client,url);
  int code = http.GET();
  http.end();
  Serial.println("Verificar usuario: HTTP "+String(code));
  return (code==200);
}

bool cadastrarUsuario(const String &vem_hash){
  if(WiFi.status()!=WL_CONNECTED) return false;
  WiFiClientSecure client; client.setInsecure();
  HTTPClient http;
  String url = API_BASE+"/usuarios/"+vem_hash;
  http.begin(client,url);
  Serial.println("Cadastrando novo usuario...");
  int code = http.POST("");
  String resposta = http.getString();
  http.end();
  Serial.println("HTTP code cadastro: "+String(code));
  Serial.println("Resposta: "+resposta);
  if(code==200||code==201){
    enviarComandoDue("CADASTRO");
    delay(1000);
    return true;
  } else {
    enviarComandoDue("ERRO", "Erro cadastro");
    delay(2000);
    return false;
  }
}

// ========================
// ==== PONTUAÇÃO =========
bool atualizarPontuacao(const String &vem_hash, int pontos){
  if(WiFi.status() != WL_CONNECTED) return false;
  WiFiClientSecure client; client.setInsecure();
  HTTPClient http;
  String url = API_BASE + "/usuarios/" + vem_hash + "/pontuacao/" + String(pontos);
  http.begin(client, url);
  int code = http.PATCH("");
  String resposta = http.getString();
  http.end();
  Serial.println("HTTP code pontuacao: " + String(code));
  Serial.println("Resposta: " + resposta);
  if(code == 200 || code == 201){
    pontuacaoAtual += pontos;
    enviarComandoDue("PONTUACAO", String(pontos), String(pontuacaoAtual));
    delay(1000);
    return true;
  } else {
    enviarComandoDue("ERRO", "Erro pontos");
    delay(2000);
    return false;
  }
}

// ========================
// ==== INTERAÇÃO =========
bool enviarInteracao(String vem_hash, String voto){
  if(WiFi.status()!=WL_CONNECTED) return false;
  enviarComandoDue("CARREGANDO", "Enviando voto");
  WiFiClientSecure client; client.setInsecure();
  HTTPClient http;
  String url = API_BASE+"/interacoes/?vem_hash="+vem_hash+"&pergunta_id="+pergunta_id+"&totem_id="+TOTEM_ID+"&resposta="+voto;
  http.begin(client,url);
  int code = http.POST("");
  String resposta = http.getString();
  http.end();
  Serial.println("HTTP code voto: "+String(code));
  Serial.println("Resposta: "+resposta);
  return (code==200 || code==201);
}

// ========================
// ==== VERIFICAR INTERAÇÃO
bool usuarioJaInteragiu(const String &vem_hash, const String &pergunta_id) {
  if(WiFi.status() != WL_CONNECTED) return false;

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;

  String url = API_BASE + "/interacoes/verificar?vem_hash=" + vem_hash + "&pergunta_id=" + pergunta_id;
  http.begin(client, url);
  int code = http.GET();

  if(code != 200) {
    Serial.println("Erro verificar interacao HTTP: " + String(code));
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();

  DynamicJsonDocument doc(256);
  if(deserializeJson(doc, payload)) {
    Serial.println("Erro JSON verificar interação");
    return false;
  }

  bool interagiu = doc["interagiu"];
  Serial.println("Usuario ja interagiu? " + String(interagiu));
  return interagiu;
}

// ========================
// ==== SCORE ==============
bool mostrarResultadoReal(String pergunta_id){
  if(WiFi.status()!=WL_CONNECTED) return false;
  enviarComandoDue("CARREGANDO", "Calculando score");
  WiFiClientSecure client; client.setInsecure();
  HTTPClient http;
  String url = API_BASE+"/interacoes/score/"+pergunta_id;
  http.begin(client,url);
  int code = http.GET();
  if(code!=200){
    enviarComandoDue("ERRO", "Falha score");
    http.end();
    delay(2000);
    return false;
  }
  String payload = http.getString();
  http.end();
  DynamicJsonDocument doc(512);
  if(deserializeJson(doc,payload)){
    enviarComandoDue("ERRO", "Falha score");
    delay(2000);
    return false;
  }
  float sim = doc["sim"];
  float nao = doc["nao"];
  enviarComandoDue("RESULTADO", String((int)sim), String((int)nao));
  delay(2500);
  return true;
}

// ========================
// ==== PERGUNTA ==========
bool obterUltimaPergunta(){
  if(WiFi.status()!=WL_CONNECTED) return false;
  enviarComandoDue("CARREGANDO", "Carregando pergunta");
  WiFiClientSecure client; client.setInsecure();
  HTTPClient http;
  String url = API_BASE+"/perguntas/ultima";
  http.begin(client,url);
  int code = http.GET();
  if(code!=200){
    enviarComandoDue("ERRO", "Erro pergunta");
    http.end();
    delay(2000);
    return false;
  }
  String payload = http.getString();
  http.end();
  DynamicJsonDocument doc(1024);
  if(deserializeJson(doc,payload)){
    enviarComandoDue("ERRO", "Erro JSON");
    delay(2000);
    return false;
  }
  pergunta = doc["texto"].as<String>();
  pergunta_id = doc["pergunta_id"].as<String>();

  enviarComandoDue("PERGUNTA", pergunta);
  estado = AGUARDANDO_VOTO;
  return true;
}

// ========================
// ==== LED CONTROLE ======
void acenderLedVerde() {
  digitalWrite(LED_VERDE_1, HIGH);
  digitalWrite(LED_VERDE_2, HIGH);
}

void acenderLedVermelho() {
  digitalWrite(LED_VERMELHO_1, HIGH);
  digitalWrite(LED_VERMELHO_2, HIGH);
}

void apagarLeds() {
  digitalWrite(LED_VERDE_1, LOW);
  digitalWrite(LED_VERDE_2, LOW);
  digitalWrite(LED_VERMELHO_1, LOW);
  digitalWrite(LED_VERMELHO_2, LOW);
}

// ========================
// ==== SETUP =============
void setup(){
  Serial.begin(115200);
  delay(1000);
  
  // Inicializar Serial2 para comunicação com Arduino Due
  // TX2 (pino 17) -> RX1 do Due
  // RX2 (pino 16) -> TX1 do Due
  SERIAL_DUE.begin(BAUD_RATE_DUE, SERIAL_8N1, 16, 17);
  delay(1000);
  Serial.println("Serial2 inicializada para comunicacao com Due (TX2=17, RX2=16)");
  
  SPI.begin(SCK_PIN,MISO_PIN,MOSI_PIN,SS_PIN_SIM);
  rfidSim.PCD_Init(); 
  rfidNao.PCD_Init();
  delay(500);
  
  conectarWiFi();
  randomSeed(analogRead(0));
  mostrarTelaInicial();

  pinMode(LED_VERDE_1, OUTPUT);
  pinMode(LED_VERDE_2, OUTPUT);
  pinMode(LED_VERMELHO_1, OUTPUT);
  pinMode(LED_VERMELHO_2, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  apagarLeds();
}

// ========================
// ==== LOOP ==============
void loop(){
  if(WiFi.status()!=WL_CONNECTED){
    if(!conectarWiFi()){
      tratarErro("WiFi");
      return;
    }
  }

  switch(estado){
    case ESPERA_CARTAO:{
      String uidSim = lerCartao(rfidSim);
      String uidNao = lerCartao(rfidNao);
      if(uidSim!="") usuarioUID=uidSim;
      else if(uidNao!="") usuarioUID=uidNao;
      if(usuarioUID!=""){
        enviarComandoDue("VERIFICANDO", usuarioUID.substring(0,16));
        estado = VERIFICANDO_USUARIO;
      }
      break;
    }

    case VERIFICANDO_USUARIO:{
      if(usuarioUID==""){
        tratarErro("Verificação Usuário");
        break;
      }
      if(usuarioExiste(usuarioUID)){
        estado = PERGUNTA;
      } else {
        estado = CADASTRANDO;
      }
      break;
    }

    case CADASTRANDO:{
      if(!cadastrarUsuario(usuarioUID)){
        tratarErro("Cadastro");
      } else {
        atualizarPontuacao(usuarioUID, 10);
        estado = PERGUNTA;
      }
      break;
    }

    case PERGUNTA:{
      if(!obterUltimaPergunta()){
        tratarErro("Carregar Pergunta");
      }
      break;
    }

    case AGUARDANDO_VOTO:{
      String cartaoSim = lerCartao(rfidSim);
      String cartaoNao = lerCartao(rfidNao);

      if(cartaoSim != "") {
        voto = "sim";
        somConfirmacaoSim(); // 🔊 LED + som juntos (SIM)
      }
      else if(cartaoNao != "") {
        voto = "nao";
        somConfirmacaoNao(); // 🔊 LED + som juntos (NÃO)
      }

      if(voto != ""){
        estado = RESULTADO;

        bool jaInteragiu = usuarioJaInteragiu(usuarioUID, pergunta_id);

        if(!enviarInteracao(usuarioUID,voto)){
          tratarErro("Enviar Voto");
        } else {
          if(!jaInteragiu){
            atualizarPontuacao(usuarioUID, 10);
          } else {
            enviarComandoDue("VOTO_ATUALIZADO", String(pontuacaoAtual));
            delay(2000);
          }
        }

        somUrna();
        delay(1000);
        apagarLeds();
      }
      break;
    }

    case RESULTADO:{
      if(!mostrarResultadoReal(pergunta_id)){
        tratarErro("Score");
      } else {
        mostrarTelaInicial();
        voto="";
        usuarioUID="";
      }
      break;
    }
  }
}


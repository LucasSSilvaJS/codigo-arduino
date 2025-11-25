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
}

void tratarErro(const String &estadoErro){
  Serial.println("Erro: " + estadoErro);
  delay(2000);
  mostrarTelaInicial();
}

// ========================
// ==== SONS =============
// ========================
// DOCUMENTAÇÃO DOS SONS:
// Cada etapa do sistema possui um som característico para feedback ao usuário

// 🔊 Som de inicialização (ao ligar o sistema)
// Descrição: Sequência ascendente de 3 tons (800Hz → 1200Hz → 1600Hz)
// Quando: No setup(), após tudo estar inicializado
void somInicializacao() {
  tone(BUZZER_PIN, 800);
  delay(200);
  noTone(BUZZER_PIN);
  delay(100);
  
  tone(BUZZER_PIN, 1200);
  delay(200);
  noTone(BUZZER_PIN);
  delay(100);
  
  tone(BUZZER_PIN, 1600);
  delay(300);
  noTone(BUZZER_PIN);
}

// 🔊 Som de verificação (ao verificar usuário na API)
// Descrição: Dois bips curtos em 1500Hz
// Quando: No estado VERIFICANDO_USUARIO, ao iniciar verificação
void somVerificacao() {
  tone(BUZZER_PIN, 1500);
  delay(150);
  noTone(BUZZER_PIN);
  delay(100);
  
  tone(BUZZER_PIN, 1500);
  delay(150);
  noTone(BUZZER_PIN);
}

// 🔊 Som de cadastro (ao cadastrar novo usuário)
// Descrição: Sequência de 3 tons ascendentes alegres (1000Hz → 1300Hz → 1600Hz)
// Quando: Ao cadastrar novo usuário com sucesso
void somCadastro() {
  tone(BUZZER_PIN, 1000);
  delay(150);
  noTone(BUZZER_PIN);
  delay(80);
  
  tone(BUZZER_PIN, 1300);
  delay(150);
  noTone(BUZZER_PIN);
  delay(80);
  
  tone(BUZZER_PIN, 1600);
  delay(200);
  noTone(BUZZER_PIN);
}

// 🔊 Som de pergunta carregada (ao carregar pergunta da API)
// Descrição: Bip único médio
// Quando: Ao carregar pergunta com sucesso
void somPerguntaCarregada() {
  tone(BUZZER_PIN, 1300);
  delay(250);
  noTone(BUZZER_PIN);
}

// 🔊 Som de voto enviado (ao enviar voto para API)
// Descrição: Bip curto de confirmação
// Quando: Ao enviar voto com sucesso
void somVotoEnviado() {
  tone(BUZZER_PIN, 1600);
  delay(200);
  noTone(BUZZER_PIN);
}

// 🔊 Som de urna (som final após votar)
// Descrição: Sequência alternada de tons (tone2/tone1) repetida 5 vezes
// Quando: Após processar o voto completamente
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

// 🔊 Som de resultado exibido (ao mostrar resultado)
// Descrição: Bip duplo descendente
// Quando: Ao exibir resultado da votação
void somResultadoExibido() {
  tone(BUZZER_PIN, 1500);
  delay(200);
  noTone(BUZZER_PIN);
  delay(100);
  
  tone(BUZZER_PIN, 1200);
  delay(200);
  noTone(BUZZER_PIN);
}

// ========================
// ==== SOM CONFIRMAÇÃO ===
// LED + som juntos
// ========================

// 🔊 Som e LED para confirmação "SIM"
// Descrição: Bip longo em tone1 (1437Hz) com LEDs verdes acesos
// Quando: Ao detectar cartão no leitor SIM durante votação
void somConfirmacaoSim() {
  digitalWrite(LED_VERDE_1, HIGH);
  digitalWrite(LED_VERDE_2, HIGH);
  tone(BUZZER_PIN, tone1);
  delay(700);
  noTone(BUZZER_PIN);
  digitalWrite(LED_VERDE_1, LOW);
  digitalWrite(LED_VERDE_2, LOW);
}

// 🔊 Som e LED para confirmação "NÃO"
// Descrição: Dois bips em tone2 (1337Hz) com LEDs vermelhos acesos
// Quando: Ao detectar cartão no leitor NÃO durante votação
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
  Serial.println("WiFi conectando...");
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
    Serial.println("Usuario cadastrado com sucesso");
    somCadastro(); // 🔊 Som de cadastro
    delay(1000);
    return true;
  } else {
    Serial.println("Erro ao cadastrar usuario");
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
    Serial.println("Pontuação +" + String(pontos) + " | Total: " + String(pontuacaoAtual));
    delay(1000);
    return true;
  } else {
    Serial.println("Erro ao atualizar pontuacao");
    delay(2000);
    return false;
  }
}

// ========================
// ==== INTERAÇÃO =========
bool enviarInteracao(String vem_hash, String voto){
  if(WiFi.status()!=WL_CONNECTED) return false;
  Serial.println("Enviando voto: " + voto);
  WiFiClientSecure client; client.setInsecure();
  HTTPClient http;
  String url = API_BASE+"/interacoes/?vem_hash="+vem_hash+"&pergunta_id="+pergunta_id+"&totem_id="+TOTEM_ID+"&resposta="+voto;
  http.begin(client,url);
  int code = http.POST("");
  String resposta = http.getString();
  http.end();
  Serial.println("HTTP code voto: "+String(code));
  Serial.println("Resposta: "+resposta);
  if(code==200 || code==201){
    somVotoEnviado(); // 🔊 Som de voto enviado
    return true;
  }
  return false;
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
  Serial.println("Calculando score...");
  WiFiClientSecure client; client.setInsecure();
  HTTPClient http;
  String url = API_BASE+"/interacoes/score/"+pergunta_id;
  http.begin(client,url);
  int code = http.GET();
  if(code!=200){
    Serial.println("Falha ao calcular score");
    http.end();
    delay(2000);
    return false;
  }
  String payload = http.getString();
  http.end();
  DynamicJsonDocument doc(512);
  if(deserializeJson(doc,payload)){
    Serial.println("Falha ao processar score");
    delay(2000);
    return false;
  }
  float sim = doc["sim"];
  float nao = doc["nao"];
  Serial.println("Resultado - Sim: " + String((int)sim) + "% | Nao: " + String((int)nao) + "%");
  somResultadoExibido(); // 🔊 Som de resultado exibido
  delay(2500);
  return true;
}

// ========================
// ==== PERGUNTA ==========
bool obterUltimaPergunta(){
  if(WiFi.status()!=WL_CONNECTED) return false;
  Serial.println("Carregando pergunta...");
  WiFiClientSecure client; client.setInsecure();
  HTTPClient http;
  String url = API_BASE+"/perguntas/ultima";
  http.begin(client,url);
  int code = http.GET();
  if(code!=200){
    Serial.println("Erro ao carregar pergunta");
    http.end();
    delay(2000);
    return false;
  }
  String payload = http.getString();
  http.end();
  DynamicJsonDocument doc(1024);
  if(deserializeJson(doc,payload)){
    Serial.println("Erro ao processar JSON da pergunta");
    delay(2000);
    return false;
  }
  pergunta = doc["texto"].as<String>();
  pergunta_id = doc["pergunta_id"].as<String>();

  Serial.println("Pergunta carregada: " + pergunta);
  somPerguntaCarregada(); // 🔊 Som de pergunta carregada
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
  SPI.begin(SCK_PIN,MISO_PIN,MOSI_PIN,SS_PIN_SIM);
  rfidSim.PCD_Init(); rfidNao.PCD_Init();
  conectarWiFi();
  randomSeed(analogRead(0));
  mostrarTelaInicial();

  pinMode(LED_VERDE_1, OUTPUT);
  pinMode(LED_VERDE_2, OUTPUT);
  pinMode(LED_VERMELHO_1, OUTPUT);
  pinMode(LED_VERMELHO_2, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  apagarLeds();
  
  // 🔊 Som de inicialização após tudo estar pronto
  delay(500);
  somInicializacao();
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
      delay(10); // Pequeno delay para evitar interferência entre leitores
      String uidNao = lerCartao(rfidNao);
      if(uidSim!="") {
        usuarioUID=uidSim;
        Serial.println("Cartao SIM detectado: " + usuarioUID);
        estado = VERIFICANDO_USUARIO;
      }
      else if(uidNao!="") {
        usuarioUID=uidNao;
        Serial.println("Cartao NAO detectado: " + usuarioUID);
        estado = VERIFICANDO_USUARIO;
      }
      break;
    }

    case VERIFICANDO_USUARIO:{
      if(usuarioUID==""){
        tratarErro("Verificação Usuário");
        break;
      }
      somVerificacao(); // 🔊 Som de verificação
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
      delay(10); // Pequeno delay para evitar interferência entre leitores
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
            Serial.println("Voto atualizado - Sem pontos");
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


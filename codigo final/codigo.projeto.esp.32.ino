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
unsigned long ultimaDetecaoCartao = 0; // Controle de intervalo entre detecções
const unsigned long INTERVALO_MINIMO_CARTAO = 2000; // 2 segundos entre detecções

// ========================
// ==== ESTADOS ===========
enum Estado { ESPERA_CARTAO, VERIFICANDO_USUARIO, AGUARDANDO_CARTAO_APOS_HASH, CADASTRANDO, PERGUNTA, AGUARDANDO_VOTO, RESULTADO };
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

// Delay que pode ser interrompido ao aproximar cartão
// Retorna true se cartão foi detectado, false se delay completo
// Inclui intervalo mínimo entre detecções para evitar avanços muito rápidos
bool delayComLeituraCartao(unsigned long tempoMs) {
  unsigned long inicio = millis();
  while(millis() - inicio < tempoMs) {
    // Verificar se passou o intervalo mínimo desde a última detecção
    unsigned long tempoAtual = millis();
    if(tempoAtual - ultimaDetecaoCartao < INTERVALO_MINIMO_CARTAO) {
      // Ainda não passou o intervalo mínimo, continuar delay sem verificar cartões
      delay(100);
      continue;
    }
    
    // Verificar cartão SIM primeiro
    String cartaoSim = lerCartao(rfidSim);
    if(cartaoSim != "") {
      Serial.println("Cartao SIM detectado durante delay - interrompendo");
      ultimaDetecaoCartao = millis(); // Registrar momento da detecção
      delay(500); // Pequeno delay adicional para estabilizar
      return true; // Cartão detectado, interromper delay
    }
    delay(50); // Pequeno delay para não sobrecarregar
    // Verificar cartão NÃO
    String cartaoNao = lerCartao(rfidNao);
    if(cartaoNao != "") {
      Serial.println("Cartao NAO detectado durante delay - interrompendo");
      ultimaDetecaoCartao = millis(); // Registrar momento da detecção
      delay(500); // Pequeno delay adicional para estabilizar
      return true; // Cartão detectado, interromper delay
    }
    delay(50); // Pequeno delay para não sobrecarregar
  }
  return false; // Nenhum cartão detectado, delay completo
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
  if(delayComLeituraCartao(2000)) {
    // Cartão detectado durante delay, voltar ao início
    mostrarTelaInicial();
    return;
  }
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
  enviarComandoDue("CARREGANDO", "Conectando WiFi...");
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
    delayComLeituraCartao(2000);
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
    somCadastro(); // 🔊 Som de cadastro
    delayComLeituraCartao(1000);
    return true;
  } else {
    enviarComandoDue("ERRO", "Erro cadastro");
    delayComLeituraCartao(2000);
    return false;
  }
}

// ========================
// ==== PONTUAÇÃO =========
int obterPontuacaoTotal(const String &vem_hash){
  if(WiFi.status() != WL_CONNECTED) return -1;
  WiFiClientSecure client; client.setInsecure();
  HTTPClient http;
  String url = API_BASE + "/usuarios/" + vem_hash;
  http.begin(client, url);
  int code = http.GET();
  if(code != 200){
    http.end();
    return -1;
  }
  String resposta = http.getString();
  http.end();
  DynamicJsonDocument doc(512);
  if(deserializeJson(doc, resposta)){
    return -1;
  }
  if(doc.containsKey("pontuacao")){
    return doc["pontuacao"].as<int>();
  }
  return -1;
}

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
    // Tentar extrair pontuação da resposta
    DynamicJsonDocument doc(512);
    int pontuacaoTotal = -1;
    if(!deserializeJson(doc, resposta)){
      if(doc.containsKey("pontuacao")){
        pontuacaoTotal = doc["pontuacao"].as<int>();
      }
    }
    
    // Se não conseguiu da resposta, buscar da API
    if(pontuacaoTotal < 0){
      pontuacaoTotal = obterPontuacaoTotal(vem_hash);
    }
    
    if(pontuacaoTotal >= 0){
      pontuacaoAtual = pontuacaoTotal;
    } else {
      // Se não conseguir obter da API, usa o valor local
      pontuacaoAtual += pontos;
    }
    
    // Exibir pontuação no Due
    enviarComandoDue("PONTUACAO", String(pontos), String(pontuacaoAtual));
    if(delayComLeituraCartao(2000)) {
      // Cartão detectado durante delay, voltar ao início
      mostrarTelaInicial();
      return false; // Retornar false para não continuar o fluxo atual
    }
    return true;
  } else {
    enviarComandoDue("ERRO", "Erro pontos");
    delayComLeituraCartao(2000);
    return false;
  }
}

// ========================
// ==== INTERAÇÃO =========
bool enviarInteracao(String vem_hash, String voto){
  if(WiFi.status()!=WL_CONNECTED) return false;
  enviarComandoDue("CARREGANDO", "Enviando voto...");
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
  enviarComandoDue("CARREGANDO", "Calculando resultado...");
  WiFiClientSecure client; client.setInsecure();
  HTTPClient http;
  String url = API_BASE+"/interacoes/score/"+pergunta_id;
  http.begin(client,url);
  int code = http.GET();
  if(code!=200){
    enviarComandoDue("ERRO", "Falha score");
    http.end();
    delayComLeituraCartao(2000);
    return false;
  }
  String payload = http.getString();
  http.end();
  DynamicJsonDocument doc(512);
  if(deserializeJson(doc,payload)){
    enviarComandoDue("ERRO", "Falha score");
    delayComLeituraCartao(2000);
    return false;
  }
  float sim = doc["sim"];
  float nao = doc["nao"];
  enviarComandoDue("RESULTADO", String((int)sim), String((int)nao));
  somResultadoExibido(); // 🔊 Som de resultado exibido
  if(delayComLeituraCartao(2500)) {
    // Cartão detectado durante delay, voltar ao início
    mostrarTelaInicial();
    return false; // Retornar false para não continuar o fluxo atual
  }
  return true;
}

// ========================
// ==== PERGUNTA ==========
bool obterUltimaPergunta(){
  if(WiFi.status()!=WL_CONNECTED) return false;
  enviarComandoDue("CARREGANDO", "Buscando pergunta...");
  WiFiClientSecure client; client.setInsecure();
  HTTPClient http;
  String url = API_BASE+"/perguntas/ultima";
  http.begin(client,url);
  int code = http.GET();
  if(code!=200){
    enviarComandoDue("ERRO", "Erro pergunta");
    http.end();
    delayComLeituraCartao(2000);
    return false;
  }
  String payload = http.getString();
  http.end();
  DynamicJsonDocument doc(1024);
  if(deserializeJson(doc,payload)){
    enviarComandoDue("ERRO", "Erro JSON");
    delayComLeituraCartao(2000);
    return false;
  }
  pergunta = doc["texto"].as<String>();
  pergunta_id = doc["pergunta_id"].as<String>();

  enviarComandoDue("PERGUNTA", pergunta);
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
      // Verificar intervalo mínimo entre detecções
      unsigned long tempoAtual = millis();
      if(tempoAtual - ultimaDetecaoCartao < INTERVALO_MINIMO_CARTAO) {
        // Ainda não passou o intervalo mínimo, não verificar cartões
        delay(100);
        break;
      }
      
      // Ler SIM primeiro (prioridade)
      String uidSim = lerCartao(rfidSim);
      if(uidSim!="") {
        usuarioUID=uidSim;
        Serial.println("Cartao SIM detectado: " + usuarioUID);
        ultimaDetecaoCartao = millis(); // Registrar momento da detecção
        delay(500); // Delay para estabilizar
        enviarComandoDue("VERIFICANDO", usuarioUID.substring(0,16));
        estado = VERIFICANDO_USUARIO;
      } else {
        // Só ler NÃO se SIM não detectou nada
        delay(20); // Delay maior para evitar interferência
        String uidNao = lerCartao(rfidNao);
        if(uidNao!="") {
          usuarioUID=uidNao;
          Serial.println("Cartao NAO detectado: " + usuarioUID);
          ultimaDetecaoCartao = millis(); // Registrar momento da detecção
          delay(500); // Delay para estabilizar
          enviarComandoDue("VERIFICANDO", usuarioUID.substring(0,16));
          estado = VERIFICANDO_USUARIO;
        }
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
        // Obter pontuação atual do usuário
        int pontuacaoTotal = obterPontuacaoTotal(usuarioUID);
        if(pontuacaoTotal >= 0){
          pontuacaoAtual = pontuacaoTotal;
        }
        // Exibir hash do usuário no Due e aguardar cartão
        enviarComandoDue("HASH_USUARIO", usuarioUID);
        estado = AGUARDANDO_CARTAO_APOS_HASH;
      } else {
        estado = CADASTRANDO;
      }
      break;
    }

    case AGUARDANDO_CARTAO_APOS_HASH:{
      // Verificar intervalo mínimo entre detecções
      unsigned long tempoAtual = millis();
      if(tempoAtual - ultimaDetecaoCartao < INTERVALO_MINIMO_CARTAO) {
        // Ainda não passou o intervalo mínimo, não verificar cartões
        delay(100);
        break;
      }
      
      // Aguardar cartão em qualquer um dos leitores
      // Ler SIM primeiro (prioridade)
      String cartaoSim = lerCartao(rfidSim);
      if(cartaoSim != ""){
        Serial.println("Cartao SIM confirmado! Continuando...");
        ultimaDetecaoCartao = millis(); // Registrar momento da detecção
        delay(500); // Delay para estabilizar
        estado = PERGUNTA;
      } else {
        // Só ler NÃO se SIM não detectou nada
        delay(20); // Delay maior para evitar interferência
        String cartaoNao = lerCartao(rfidNao);
        if(cartaoNao != ""){
          Serial.println("Cartao NAO confirmado! Continuando...");
          ultimaDetecaoCartao = millis(); // Registrar momento da detecção
          delay(500); // Delay para estabilizar
          estado = PERGUNTA;
        }
      }
      break;
    }

    case CADASTRANDO:{
      if(!cadastrarUsuario(usuarioUID)){
        tratarErro("Cadastro");
      } else {
        if(!atualizarPontuacao(usuarioUID, 10)){
          // Se atualizarPontuacao retornou false por cartão detectado, já voltou ao início
          break;
        }
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
      // Verificar intervalo mínimo entre detecções
      unsigned long tempoAtual = millis();
      if(tempoAtual - ultimaDetecaoCartao < INTERVALO_MINIMO_CARTAO) {
        // Ainda não passou o intervalo mínimo, não verificar cartões
        delay(100);
        break;
      }
      
      // Ler SIM primeiro (prioridade)
      String cartaoSim = lerCartao(rfidSim);
      if(cartaoSim != "") {
        voto = "sim";
        ultimaDetecaoCartao = millis(); // Registrar momento da detecção
        somConfirmacaoSim(); // 🔊 LED + som juntos (SIM)
      } else {
        // Só ler NÃO se SIM não detectou nada
        delay(20); // Delay maior para evitar interferência
        String cartaoNao = lerCartao(rfidNao);
        if(cartaoNao != "") {
          voto = "nao";
          ultimaDetecaoCartao = millis(); // Registrar momento da detecção
          somConfirmacaoNao(); // 🔊 LED + som juntos (NÃO)
        }
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
            // Obter pontuação total atual para exibir
            int pontuacaoTotal = obterPontuacaoTotal(usuarioUID);
            if(pontuacaoTotal >= 0){
              pontuacaoAtual = pontuacaoTotal;
              enviarComandoDue("VOTO_ATUALIZADO", String(pontuacaoAtual));
            } else {
              enviarComandoDue("VOTO_ATUALIZADO", String(pontuacaoAtual));
            }
            if(delayComLeituraCartao(2000)) {
              // Cartão detectado durante delay, voltar ao início
              mostrarTelaInicial();
              voto = "";
              usuarioUID = "";
              return; // Sair do case
            }
          }
        }

        somUrna();
        if(delayComLeituraCartao(1000)) {
          // Cartão detectado durante delay, voltar ao início
          mostrarTelaInicial();
          voto = "";
          usuarioUID = "";
          return; // Sair do case
        }
        apagarLeds();
      }
      break;
    }

    case RESULTADO:{
      if(!mostrarResultadoReal(pergunta_id)){
        tratarErro("Score");
      } else {
        // Exibir QR Code como última tela (permanece por muito tempo)
        enviarComandoDue("QRCODE");
        if(delayComLeituraCartao(20000)) {
          // Cartão detectado durante delay, voltar ao início imediatamente
          mostrarTelaInicial();
          voto="";
          usuarioUID="";
          return; // Sair do case
        }
        // Delay completo, voltar ao início normalmente
        mostrarTelaInicial();
        voto="";
        usuarioUID="";
      }
      break;
    }
  }
}

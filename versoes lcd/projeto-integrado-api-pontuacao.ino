#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>

// ========================
// ==== CONFIG WIFI =======
const char* ssid = "SENAC-Mesh";
const char* password = "09080706";
const String API_BASE = "https://projeto-bigdata.onrender.com";
const String TOTEM_ID = "5e652a794087";

// ========================
// ==== CONFIG LCD ========
#define SDA_PIN 21
#define SCL_PIN 22
LiquidCrystal_I2C lcd(0x27,16,2);

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
// ==== VARIÁVEIS ROLAGEM
unsigned long ultimaAtualizacao = 0;
int indiceRolagem = 0;
String perguntaRolar = "";

// ========================
// ==== LIMPAR PARA LCD ===
String limparParaLCD(const String &texto){
  String t = texto;
  t.replace("á","a"); t.replace("à","a"); t.replace("ã","a"); t.replace("â","a"); t.replace("ä","a");
  t.replace("é","e"); t.replace("è","e"); t.replace("ê","e"); t.replace("ë","e");
  t.replace("í","i"); t.replace("ì","i"); t.replace("î","i"); t.replace("ï","i");
  t.replace("ó","o"); t.replace("ò","o"); t.replace("õ","o"); t.replace("ô","o"); t.replace("ö","o");
  t.replace("ú","u"); t.replace("ù","u"); t.replace("û","u"); t.replace("ü","u");
  t.replace("ç","c");
  t.replace("Á","A"); t.replace("À","A"); t.replace("Ã","A"); t.replace("Â","A"); t.replace("Ä","A");
  t.replace("É","E"); t.replace("È","E"); t.replace("Ê","E"); t.replace("Ë","E");
  t.replace("Í","I"); t.replace("Ì","I"); t.replace("Î","I"); t.replace("Ï","I");
  t.replace("Ó","O"); t.replace("Ò","O"); t.replace("Õ","O"); t.replace("Ô","O"); t.replace("Ö","O");
  t.replace("Ú","U"); t.replace("Ù","U"); t.replace("Û","U"); t.replace("Ü","U");
  t.replace("Ç","C");

  String result = "";
  for(int i=0;i<t.length();i++){
    if(t[i] >= 32 && t[i] <= 126) result += t[i];
    else result += '?';
  }
  return result;
}

// ========================
// ==== FUNÇÕES AUX =======
void mostrarMensagem(const String &linha1, const String &linha2 = "") {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(limparParaLCD(linha1).substring(0,16));
  lcd.setCursor(0,1);
  lcd.print(limparParaLCD(linha2).substring(0,16));
}

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
  mostrarMensagem("Bem-vindo!","Aproxime o VEM");
}

void tratarErro(const String &estadoErro){
  mostrarMensagem("Erro em:", estadoErro);
  delay(2000);
  mostrarTelaInicial();
}

// ========================
// ==== WIFI ===============
bool conectarWiFi() {
  WiFi.begin(ssid,password);
  mostrarMensagem("WiFi conectando","Aguarde...");
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
    mostrarMensagem("Erro WiFi","Verifique rede");
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
    mostrarMensagem("Usuario","cadastrado");
    delay(1000);
    return true;
  } else {
    mostrarMensagem("Erro cadastro");
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
    mostrarMensagem("Pontuação +" + String(pontos), "Total:" + String(pontuacaoAtual));
    delay(1000);
    return true;
  } else {
    mostrarMensagem("Erro pontos");
    delay(2000);
    return false;
  }
}

// ========================
// ==== INTERAÇÃO =========
bool enviarInteracao(String vem_hash, String voto){
  if(WiFi.status()!=WL_CONNECTED) return false;
  mostrarMensagem("Enviando","voto: "+voto);
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
    return false; // Em caso de erro, assume que não interagiu
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
  mostrarMensagem("Calculando score","");
  WiFiClientSecure client; client.setInsecure();
  HTTPClient http;
  String url = API_BASE+"/interacoes/score/"+pergunta_id;
  http.begin(client,url);
  int code = http.GET();
  if(code!=200){
    mostrarMensagem("Falha score");
    http.end();
    delay(2000);
    return false;
  }
  String payload = http.getString();
  http.end();
  DynamicJsonDocument doc(512);
  if(deserializeJson(doc,payload)){
    mostrarMensagem("Falha score");
    delay(2000);
    return false;
  }
  float sim = doc["sim"];
  float nao = doc["nao"];
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Sim:"+String((int)sim)+"%");
  lcd.setCursor(0,1);
  lcd.print("Nao:"+String((int)nao)+"%");
  delay(2500);
  return true;
}

// ========================
// ==== PERGUNTA ==========
bool obterUltimaPergunta(){
  if(WiFi.status()!=WL_CONNECTED) return false;
  mostrarMensagem("Carregando","pergunta");
  WiFiClientSecure client; client.setInsecure();
  HTTPClient http;
  String url = API_BASE+"/perguntas/ultima";
  http.begin(client,url);
  int code = http.GET();
  if(code!=200){
    mostrarMensagem("Erro pergunta");
    http.end();
    delay(2000);
    return false;
  }
  String payload = http.getString();
  http.end();
  DynamicJsonDocument doc(1024);
  if(deserializeJson(doc,payload)){
    mostrarMensagem("Erro JSON");
    delay(2000);
    return false;
  }
  pergunta = doc["texto"].as<String>();
  pergunta_id = doc["pergunta_id"].as<String>();

  // Inicializa rolagem
  perguntaRolar = limparParaLCD(pergunta) + "    ";
  indiceRolagem = 0;
  ultimaAtualizacao = millis();
  lcd.setCursor(0,1);
  lcd.print("Sim ou nao?");
  estado = AGUARDANDO_VOTO;
  return true;
}

// ========================
// ==== ROLAGEM ===========
void atualizarRolagem(){
  if(perguntaRolar.length() <= 16) {
    lcd.setCursor(0,0);
    lcd.print(perguntaRolar);
    return;
  }
  if(millis() - ultimaAtualizacao > 300){
    String exibicao = "";
    for(int j=0;j<16;j++){
      int idx = (indiceRolagem + j) % perguntaRolar.length();
      exibicao += perguntaRolar[idx];
    }
    lcd.setCursor(0,0);
    lcd.print(exibicao);
    indiceRolagem = (indiceRolagem + 1) % perguntaRolar.length();
    ultimaAtualizacao = millis();
  }
}

// ========================
// ==== SETUP ============
void setup(){
  Serial.begin(115200);
  Wire.begin(SDA_PIN,SCL_PIN);
  lcd.init(); lcd.backlight();
  SPI.begin(SCK_PIN,MISO_PIN,MOSI_PIN,SS_PIN_SIM);
  rfidSim.PCD_Init(); rfidNao.PCD_Init();
  conectarWiFi();
  randomSeed(analogRead(0));
  mostrarTelaInicial();
}

// ========================
// ==== LOOP ============
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
        mostrarMensagem("Verificando...",usuarioUID.substring(0,16));
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
      atualizarRolagem();

      String cartaoSim = lerCartao(rfidSim);
      String cartaoNao = lerCartao(rfidNao);

      if(cartaoSim != "") voto = "sim";
      else if(cartaoNao != "") voto = "nao";

      if(voto != ""){
        estado = RESULTADO;

        // ✅ Verifica se já interagiu antes de enviar
        bool jaInteragiu = usuarioJaInteragiu(usuarioUID, pergunta_id);

        if(!enviarInteracao(usuarioUID,voto)){
          tratarErro("Enviar Voto");
        } else {
          if(!jaInteragiu){
            atualizarPontuacao(usuarioUID, 10);
          } else {
            mostrarMensagem("Voto atualizado","Sem pontos");
            delay(2000);
          }
        }
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

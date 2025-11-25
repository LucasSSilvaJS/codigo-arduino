// ========================
// ==== CÓDIGO ARDUINO DUE ====
// Sistema de exibição TFT Shield
// Comunicação Serial com ESP32
// ========================

#include <MCUFRIEND_kbv.h>
#include <Adafruit_GFX.h>
#include <math.h>
#include "qrcode.h"

#ifndef PI
#define PI 3.14159265359
#endif

MCUFRIEND_kbv tft;

// ========================
// ==== CONFIG SERIAL =====
// Serial1 para comunicação com ESP32
// TX1 -> RX2 da ESP32
// RX1 -> TX2 da ESP32
#define SERIAL_ESP32 Serial1
#define BAUD_RATE 115200

// ========================
// ==== CORES =============
#define BLACK   0x0000
#define WHITE   0xFFFF
#define YELLOW  0xFFE0
#define CYAN    0x07FF
#define GREEN   0x07E0
#define RED     0xF800
#define MAGENTA 0xF81F
#define BLUE    0x001F

// ========================
// ==== VARIAVEIS =========
String comandoRecebido = "";
String dados[3]; // Array para armazenar dados do comando
bool tftInicializado = false;
unsigned long ultimoComandoRecebido = 0;

// ========================
// ==== SISTEMA DE LOG =====
void logInfo(const String &mensagem) {
  Serial.println("[INFO] " + mensagem);
}

void logErro(const String &mensagem) {
  Serial.println("[ERRO] " + mensagem);
}

void logDebug(const String &mensagem) {
  Serial.println("[DEBUG] " + mensagem);
}

// ========================
// ==== FUNÇÕES AUXILIARES =
int centralizar(const char* texto, int textSize = 2) {
  int charWidth = 7 * textSize;
  int textWidth = strlen(texto) * charWidth;
  return (320 - textWidth) / 2;
}

// ========================
// ==== LIMPAR CARACTERES ESPECIAIS ===
String limparCaracteresEspeciais(const String &texto) {
  String t = texto;
  // Remover acentos minúsculos
  t.replace("á","a"); t.replace("à","a"); t.replace("ã","a"); t.replace("â","a"); t.replace("ä","a");
  t.replace("é","e"); t.replace("è","e"); t.replace("ê","e"); t.replace("ë","e");
  t.replace("í","i"); t.replace("ì","i"); t.replace("î","i"); t.replace("ï","i");
  t.replace("ó","o"); t.replace("ò","o"); t.replace("õ","o"); t.replace("ô","o"); t.replace("ö","o");
  t.replace("ú","u"); t.replace("ù","u"); t.replace("û","u"); t.replace("ü","u");
  t.replace("ç","c");
  // Remover acentos maiúsculos
  t.replace("Á","A"); t.replace("À","A"); t.replace("Ã","A"); t.replace("Â","A"); t.replace("Ä","A");
  t.replace("É","E"); t.replace("È","E"); t.replace("Ê","E"); t.replace("Ë","E");
  t.replace("Í","I"); t.replace("Ì","I"); t.replace("Î","I"); t.replace("Ï","I");
  t.replace("Ó","O"); t.replace("Ò","O"); t.replace("Õ","O"); t.replace("Ô","O"); t.replace("Ö","O");
  t.replace("Ú","U"); t.replace("Ù","U"); t.replace("Û","U"); t.replace("Ü","U");
  t.replace("Ç","C");
  
  // Remover outros caracteres especiais e manter apenas ASCII 32-126
  String result = "";
  for(int i = 0; i < t.length(); i++) {
    if(t[i] >= 32 && t[i] <= 126) {
      result += t[i];
    } else if(t[i] == '\n' || t[i] == '\r') {
      result += ' '; // Substituir quebras de linha por espaço
    } else {
      result += '?'; // Substituir caracteres não suportados por ?
    }
  }
  return result;
}

void printTextoQuebrado(const char* txt, int x, int y, int larguraMax, int textSize) {
  tft.setTextSize(textSize);
  // Largura aproximada de cada caractere (6 pixels base * textSize)
  int charWidth = 6 * textSize;
  // Altura de cada linha
  int alturaLinha = textSize * 14;
  int yInicial = y;
  // Limite de altura - garantir que não ultrapasse a tela
  int yMax = 240; // Altura máxima da tela
  // Limite de largura - garantir que não ultrapasse x + larguraMax ou 320 pixels
  int xMax = (x + larguraMax < 320) ? (x + larguraMax) : 320;
  int larguraDisponivel = xMax - x;
  
  String texto = String(txt);
  String palavra = "";
  String linha = "";
  int pos = 0;
  
  while(pos < texto.length() && y < yMax) {
    char c = texto.charAt(pos);
    
    if(c == ' ' || c == '\n' || pos == texto.length() - 1) {
      // Adicionar último caractere se não for espaço ou quebra
      if(c != ' ' && c != '\n') palavra += c;
      
      // Calcular largura da linha atual + palavra
      int larguraLinhaAtual = linha.length() * charWidth;
      int larguraPalavra = palavra.length() * charWidth;
      int larguraTotal = larguraLinhaAtual + larguraPalavra;
      
      // Se tiver linha anterior, adicionar espaço
      if(linha.length() > 0) larguraTotal += charWidth;
      
      // Verificar se cabe na largura disponível
      if(larguraTotal > larguraDisponivel && linha.length() > 0) {
        // Não cabe - imprimir linha atual e começar nova linha
        if(y < yMax) {
          tft.setCursor(x, y);
          tft.print(linha);
          y += alturaLinha;
        }
        linha = palavra;
        palavra = "";
      } else {
        // Cabe - adicionar palavra à linha
        if(linha.length() > 0) linha += " ";
        linha += palavra;
        palavra = "";
      }
      
      // Quebra de linha forçada
      if(c == '\n') {
        if(linha.length() > 0 && y < yMax) {
          tft.setCursor(x, y);
          tft.print(linha);
          y += alturaLinha;
        }
        linha = "";
      }
    } else {
      palavra += c;
    }
    
    pos++;
  }
  
  // Imprimir última linha se houver e couber
  if(linha.length() > 0 && y < yMax) {
    // Verificar se a última linha cabe na largura
    int larguraUltimaLinha = linha.length() * charWidth;
    if(larguraUltimaLinha > larguraDisponivel) {
      // Se não couber, truncar
      int maxChars = larguraDisponivel / charWidth;
      if(maxChars > 0) {
        linha = linha.substring(0, maxChars);
      }
    }
    tft.setCursor(x, y);
    tft.print(linha);
  }
}

// ========================
// ==== FUNÇÕES TELA ======
void limparTela() {
  if(!tftInicializado) {
    logErro("TFT nao inicializado! Nao e possivel limpar tela.");
    return;
  }
  tft.fillScreen(BLACK);
}

void mostrarTelaInicial() {
  logInfo("Exibindo tela inicial...");
  if(!tftInicializado) {
    logErro("TFT nao inicializado! Nao e possivel exibir tela inicial.");
    return;
  }
  
  limparTela();
  
  // Header padronizado (40px)
  tft.fillRect(0, 0, 320, 40, BLUE);
  tft.setTextColor(WHITE);
  tft.setTextSize(2);
  tft.setCursor(20, 12);
  tft.print("BUSSULA CIDADA");
  
  // Mensagem principal (alinhada à esquerda)
  tft.setTextColor(WHITE);
  tft.setTextSize(3);
  tft.setCursor(20, 70);
  tft.print("Aproxime");
  
  tft.setCursor(20, 110);
  tft.print("o cartao");
  
  logInfo("Tela inicial exibida com sucesso!");
}

void mostrarTelaVerificando(String uid) {
  limparTela();
  
  // Header padronizado (40px)
  tft.fillRect(0, 0, 320, 40, BLUE);
  tft.setTextColor(WHITE);
  tft.setTextSize(2);
  tft.setCursor(20, 12);
  tft.print("VERIFICANDO");
  
  // Mensagem principal (alinhada à esquerda)
  tft.setTextColor(WHITE);
  tft.setTextSize(3);
  tft.setCursor(20, 70);
  tft.print("Verificando");
  
  tft.setCursor(20, 110);
  tft.print("cartao");
  
  // Loading padronizado na parte inferior
  static int loadingFrame = 0;
  loadingFrame = (loadingFrame + 1) % 8;
  
  int loadingY = 200;
  int centerX = 160;
  int radius = 12;
  
  for(int i = 0; i < 8; i++) {
    float angle = (i * 45.0 - 90.0) * PI / 180.0;
    int x = centerX + (int)(radius * cos(angle));
    int y = loadingY + (int)(radius * sin(angle));
    
    if(i == loadingFrame) {
      tft.fillCircle(x, y, 3, BLUE);
    } else {
      tft.fillCircle(x, y, 2, CYAN);
    }
  }
}

void mostrarTelaCarregando(String mensagem) {
  limparTela();
  
  // Header padronizado (40px)
  tft.fillRect(0, 0, 320, 40, BLUE);
  tft.setTextColor(WHITE);
  tft.setTextSize(2);
  tft.setCursor(20, 12);
  tft.print("CARREGANDO");
  
  // Mensagem principal (alinhada à esquerda) - usar mensagem dinâmica
  tft.setTextColor(WHITE);
  tft.setTextSize(2);
  
  // Limpar caracteres especiais da mensagem
  String msgLimpa = limparCaracteresEspeciais(mensagem);
  
  // Exibir mensagem quebrada em linhas se necessário
  if(msgLimpa.length() > 0) {
    printTextoQuebrado(msgLimpa.c_str(), 20, 70, 280, 2);
  } else {
    tft.setCursor(20, 70);
    tft.print("Aguarde...");
  }
  
  // Loading padronizado na parte inferior
  static int loadingFrame = 0;
  loadingFrame = (loadingFrame + 1) % 8;
  
  int loadingY = 200;
  int centerX = 160;
  int radius = 12;
  
  for(int i = 0; i < 8; i++) {
    float angle = (i * 45.0 - 90.0) * PI / 180.0;
    int x = centerX + (int)(radius * cos(angle));
    int y = loadingY + (int)(radius * sin(angle));
    
    if(i == loadingFrame) {
      tft.fillCircle(x, y, 3, BLUE);
    } else {
      tft.fillCircle(x, y, 2, CYAN);
    }
  }
}

void mostrarTelaCadastro() {
  limparTela();
  
  // Header padronizado (40px)
  tft.fillRect(0, 0, 320, 40, GREEN);
  tft.setTextColor(BLACK);
  tft.setTextSize(2);
  tft.setCursor(20, 12);
  tft.print("SUCESSO");
  
  // Mensagem principal (alinhada à esquerda)
  tft.setTextColor(WHITE);
  tft.setTextSize(3);
  tft.setCursor(20, 70);
  tft.print("Cadastrado");
  
  // Pontos destacados (alinhado à esquerda)
  tft.setTextColor(YELLOW);
  tft.setTextSize(4);
  tft.setCursor(20, 120);
  tft.print("+10");
  
  delay(2000);
}

void mostrarTelaHashUsuario(String hash) {
  limparTela();
  
  // Header padronizado (40px)
  tft.fillRect(0, 0, 320, 40, CYAN);
  tft.setTextColor(BLACK);
  tft.setTextSize(2);
  tft.setCursor(20, 12);
  tft.print("USUARIO");
  
  // Mensagem principal (alinhada à esquerda)
  tft.setTextColor(WHITE);
  tft.setTextSize(2);
  tft.setCursor(20, 60);
  tft.print("Hash:");
  
  // Hash do usuário (quebrado em linhas se necessário)
  tft.setTextColor(CYAN);
  tft.setTextSize(2);
  printTextoQuebrado(hash.c_str(), 20, 90, 280, 2);
  
  // Instrução (alinhada à esquerda)
  tft.setTextColor(WHITE);
  tft.setTextSize(2);
  tft.setCursor(20, 180);
  tft.print("Aproxime");
  
  tft.setCursor(20, 210);
  tft.print("o cartao");
}

void mostrarTelaPergunta(String pergunta) {
  limparTela();
  
  logInfo("Exibindo pergunta: " + pergunta.substring(0, 50));
  
  // Header padronizado (40px)
  tft.fillRect(0, 0, 320, 40, CYAN);
  tft.setTextColor(BLACK);
  tft.setTextSize(2);
  tft.setCursor(20, 12);
  tft.print("PERGUNTA");
  
  // Card da pergunta padronizado (mais espaço)
  tft.fillRect(15, 50, 290, 110, BLACK);
  tft.drawRect(15, 50, 290, 110, CYAN);
  
  // Pergunta do ESP32 (alinhada à esquerda, fonte maior)
  tft.setTextColor(WHITE);
  tft.setTextSize(2);
  // Limpar caracteres especiais e exibir pergunta completa
  if(pergunta.length() > 0) {
    String perguntaLimpa = limparCaracteresEspeciais(pergunta);
    printTextoQuebrado(perguntaLimpa.c_str(), 25, 65, 270, 2);
  } else {
    tft.setCursor(25, 65);
    tft.print("Carregando pergunta...");
  }
  
  // Divisor padronizado
  tft.drawLine(15, 170, 305, 170, CYAN);
  
  // Botões padronizados
  tft.setTextSize(3);
  
  // SIM (verde)
  int simX = 20;
  int simY = 185;
  int simW = 130;
  int simH = 50;
  tft.fillRect(simX, simY, simW, simH, GREEN);
  tft.drawRect(simX, simY, simW, simH, BLACK);
  tft.setTextColor(BLACK);
  tft.setCursor(simX + 40, simY + 15);
  tft.print("SIM");
  
  // NAO (vermelho)
  int naoX = 170;
  int naoW = 130;
  tft.fillRect(naoX, simY, naoW, simH, RED);
  tft.drawRect(naoX, simY, naoW, simH, BLACK);
  tft.setTextColor(BLACK);
  tft.setCursor(naoX + 40, simY + 15);
  tft.print("NAO");
}

void mostrarTelaResultado(String simStr, String naoStr) {
  limparTela();
  
  int simPercent = simStr.toInt();
  int naoPercent = naoStr.toInt();
  
  // Header padronizado (40px)
  tft.fillRect(0, 0, 320, 40, YELLOW);
  tft.setTextColor(BLACK);
  tft.setTextSize(2);
  tft.setCursor(20, 12);
  tft.print("RESULTADO");
  
  // Barras VERTICAIS (de cima para baixo)
  int topY = 50;
  int maxBarHeight = 120;
  int barWidth = 100;
  int simX = 50;
  int naoX = 170;
  int simH = (simPercent * maxBarHeight) / 100;
  int naoH = (naoPercent * maxBarHeight) / 100;
  
  // SIM - barra vertical (verde)
  tft.fillRect(simX, topY, barWidth, simH, GREEN);
  tft.drawRect(simX, topY, barWidth, simH, BLACK);
  // Percentual SIM dentro da barra
  tft.setTextSize(3);
  if(simH > 25) {
    tft.setTextColor(BLACK);
    tft.setCursor(simX + 25, topY + simH/2 - 10);
  } else {
    tft.setTextColor(GREEN);
    tft.setCursor(simX + 20, topY + simH + 5);
  }
  tft.print(String(simPercent) + "%");
  
  // NAO - barra vertical (vermelho)
  tft.fillRect(naoX, topY, barWidth, naoH, RED);
  tft.drawRect(naoX, topY, barWidth, naoH, BLACK);
  // Percentual NAO dentro da barra
  if(naoH > 25) {
    tft.setTextColor(BLACK);
    tft.setCursor(naoX + 30, topY + naoH/2 - 10);
  } else {
    tft.setTextColor(RED);
    tft.setCursor(naoX + 25, topY + naoH + 5);
  }
  tft.print(String(naoPercent) + "%");
  
  // Labels SIM e NAO embaixo das barras
  tft.setTextSize(2);
  tft.setTextColor(GREEN);
  tft.setCursor(simX + 30, topY + maxBarHeight + 10);
  tft.print("SIM");
  
  tft.setTextColor(RED);
  tft.setCursor(naoX + 30, topY + maxBarHeight + 10);
  tft.print("NAO");
}

void mostrarTelaErro(String mensagem) {
  limparTela();
  
  // Header padronizado (40px)
  tft.fillRect(0, 0, 320, 40, RED);
  tft.setTextColor(WHITE);
  tft.setTextSize(2);
  tft.setCursor(20, 12);
  tft.print("ERRO");
  
  // Mensagem principal (alinhada à esquerda)
  tft.setTextColor(WHITE);
  tft.setTextSize(3);
  tft.setCursor(20, 70);
  tft.print("Erro");
  
  // Mensagem do ESP32 (alinhada à esquerda)
  tft.setTextColor(WHITE);
  tft.setTextSize(2);
  String msgCurta = mensagem;
  if(msgCurta.length() > 28) {
    msgCurta = msgCurta.substring(0, 28);
  }
  printTextoQuebrado(msgCurta.c_str(), 20, 120, 280, 2);
}

void mostrarTelaPontuacao(String pontos, String total) {
  limparTela();
  
  // Header padronizado (40px)
  tft.fillRect(0, 0, 320, 40, YELLOW);
  tft.setTextColor(BLACK);
  tft.setTextSize(2);
  tft.setCursor(20, 12);
  tft.print("PONTUACAO");
  
  // Pontos ganhos (alinhado à esquerda)
  tft.setTextColor(GREEN);
  tft.setTextSize(4);
  tft.setCursor(20, 80);
  tft.print("+" + pontos);
  
  // Total (alinhado à esquerda)
  tft.setTextColor(CYAN);
  tft.setTextSize(2);
  tft.setCursor(20, 140);
  tft.print("Total: " + total);
}

void mostrarTelaVotoAtualizado(String pontuacaoTotal) {
  limparTela();
  
  // Header padronizado (40px)
  tft.fillRect(0, 0, 320, 40, YELLOW);
  tft.setTextColor(BLACK);
  tft.setTextSize(2);
  tft.setCursor(20, 12);
  tft.print("ATUALIZADO");
  
  // Mensagem principal (alinhada à esquerda)
  tft.setTextColor(WHITE);
  tft.setTextSize(3);
  tft.setCursor(20, 70);
  tft.print("Voto");
  
  tft.setCursor(20, 110);
  tft.print("atualizado");
  
  // Pontuação total (se disponível)
  if(pontuacaoTotal.length() > 0) {
    tft.setTextColor(CYAN);
    tft.setTextSize(2);
    tft.setCursor(20, 160);
    tft.print("Pontuacao:");
    
    tft.setTextColor(YELLOW);
    tft.setTextSize(3);
    tft.setCursor(20, 190);
    tft.print(pontuacaoTotal);
  }
}

void mostrarTelaQRCode() {
  limparTela();
  
  // Header padronizado (40px)
  tft.fillRect(0, 0, 320, 40, CYAN);
  tft.setTextColor(BLACK);
  tft.setTextSize(2);
  tft.setCursor(20, 12);
  tft.print("CONFIRA NOSSO SITE");
  
  // Gerar QR Code
  String url = "https://bussola-cidada-usuario.vercel.app/";
  
  // Criar buffer para QR code (versão 4 = até 100 caracteres)
  uint8_t qrcodeData[qrcode_getBufferSize(4)];
  QRCode qrcode;
  qrcode_initText(&qrcode, qrcodeData, 4, ECC_LOW, url.c_str());
  
  // Tamanho do QR code (versão 4 = 33x33 módulos)
  int qrSize = qrcode.size;
  int moduleSize = 5; // Tamanho de cada módulo em pixels (aumentado para QR code grande)
  int qrDisplaySize = qrSize * moduleSize;
  int qrX = (320 - qrDisplaySize) / 2; // Centralizar horizontalmente
  int qrY = 60; // Posição vertical (ajustada para centralizar melhor)
  
  // Desenhar QR code
  for (uint8_t y = 0; y < qrSize; y++) {
    for (uint8_t x = 0; x < qrSize; x++) {
      if (qrcode_getModule(&qrcode, x, y)) {
        // Preencher módulo (preto)
        tft.fillRect(qrX + (x * moduleSize), qrY + (y * moduleSize), moduleSize, moduleSize, BLACK);
      } else {
        // Preencher módulo (branco)
        tft.fillRect(qrX + (x * moduleSize), qrY + (y * moduleSize), moduleSize, moduleSize, WHITE);
      }
    }
  }
  
  // Borda ao redor do QR code
  tft.drawRect(qrX - 3, qrY - 3, qrDisplaySize + 6, qrDisplaySize + 6, CYAN);
  
  logInfo("QR Code exibido: " + url);
}

void mostrarTelaSemComunicacao(bool indicadorPiscante = false) {
  limparTela();
  
  // Header padronizado (40px)
  tft.fillRect(0, 0, 320, 40, RED);
  tft.setTextColor(WHITE);
  tft.setTextSize(2);
  tft.setCursor(20, 12);
  tft.print("SEM COMUNICACAO");
  
  // Mensagem principal (alinhada à esquerda)
  tft.setTextColor(WHITE);
  tft.setTextSize(3);
  tft.setCursor(20, 70);
  tft.print("Aguardando");
  
  tft.setCursor(20, 110);
  tft.print("ESP32");
}

// ========================
// ==== PROCESSAR COMANDO =
void processarComando() {
  if(comandoRecebido.length() == 0) {
    logErro("Tentativa de processar comando vazio!");
    return;
  }
  
  logDebug("Processando comando recebido: " + comandoRecebido);
  
  // Separar comando e dados
  int posPipe1 = comandoRecebido.indexOf('|');
  int posPipe2 = comandoRecebido.indexOf('|', posPipe1 + 1);
  
  String comando = comandoRecebido;
  dados[0] = "";
  dados[1] = "";
  dados[2] = "";
  
  if(posPipe1 > 0) {
    comando = comandoRecebido.substring(0, posPipe1);
    dados[0] = comandoRecebido.substring(posPipe1 + 1);
    
    if(posPipe2 > 0) {
      dados[1] = comandoRecebido.substring(posPipe2 + 1);
      dados[0] = comandoRecebido.substring(posPipe1 + 1, posPipe2);
    }
  }
  
  logInfo("Comando extraido: " + comando);
  if(dados[0] != "") logDebug("Dado 1: " + dados[0]);
  if(dados[1] != "") logDebug("Dado 2: " + dados[1]);
  
  // Processar comando
  if(comando == "INICIAL") {
    logInfo("Executando comando: INICIAL");
    mostrarTelaInicial();
  }
  else if(comando == "VERIFICANDO") {
    logInfo("Executando comando: VERIFICANDO com UID: " + dados[0]);
    mostrarTelaVerificando(dados[0]);
  }
  else if(comando == "CARREGANDO") {
    logInfo("Executando comando: CARREGANDO - " + dados[0]);
    mostrarTelaCarregando(dados[0]);
  }
  else if(comando == "CADASTRO") {
    logInfo("Executando comando: CADASTRO");
    mostrarTelaCadastro();
  }
  else if(comando == "HASH_USUARIO") {
    logInfo("Executando comando: HASH_USUARIO");
    logInfo("Hash recebido: " + dados[0]);
    if(dados[0].length() > 0) {
      mostrarTelaHashUsuario(dados[0]);
    } else {
      logErro("HASH_USUARIO recebido mas dados[0] está vazio!");
      mostrarTelaErro("Hash vazio");
    }
  }
  else if(comando == "PERGUNTA") {
    logInfo("Executando comando: PERGUNTA");
    logInfo("Tamanho do dado recebido: " + String(dados[0].length()));
    if(dados[0].length() > 0) {
      logInfo("Texto da pergunta (primeiros 50 chars): " + dados[0].substring(0, dados[0].length() > 50 ? 50 : dados[0].length()));
      mostrarTelaPergunta(dados[0]);
    } else {
      logErro("PERGUNTA recebida mas dados[0] está vazio!");
      mostrarTelaErro("Pergunta vazia");
    }
  }
  else if(comando == "RESULTADO") {
    logInfo("Executando comando: RESULTADO - SIM: " + dados[0] + "%, NAO: " + dados[1] + "%");
    mostrarTelaResultado(dados[0], dados[1]);
  }
  else if(comando == "ERRO") {
    logErro("Executando comando: ERRO - " + dados[0]);
    mostrarTelaErro(dados[0]);
  }
  else if(comando == "PONTUACAO") {
    logInfo("Executando comando: PONTUACAO - +" + dados[0] + " pontos, Total: " + dados[1]);
    mostrarTelaPontuacao(dados[0], dados[1]);
  }
  else if(comando == "VOTO_ATUALIZADO") {
    logInfo("Executando comando: VOTO_ATUALIZADO");
    logInfo("Pontuacao total recebida: " + dados[0]);
    mostrarTelaVotoAtualizado(dados[0]);
  }
  else if(comando == "QRCODE") {
    logInfo("Executando comando: QRCODE");
    mostrarTelaQRCode();
  }
  else if(comando == "TESTE") {
    logInfo("Comando de TESTE recebido! Comunicacao funcionando!");
    logInfo("Dado recebido: " + dados[0]);
    limparTela();
    tft.setTextColor(GREEN);
    tft.setTextSize(3);
    tft.setCursor(50, 100);
    tft.println("TESTE OK!");
    tft.setTextColor(WHITE);
    tft.setTextSize(2);
    tft.setCursor(50, 150);
    tft.println("Comunicacao OK");
    delay(2000);
  }
  else if(comando == "HEARTBEAT") {
    logInfo("Heartbeat recebido! Tempo: " + dados[0] + " segundos");
  }
  else {
    logErro("Comando desconhecido: " + comando);
  }
  
  comandoRecebido = "";
}

// ========================
// ==== SETUP =============
void setup() {
  // Inicializar Serial para debug
  Serial.begin(115200);
  delay(1000);
  
  logInfo("=== INICIALIZANDO ARDUINO DUE ===");
  
  // Inicializar Serial1 para comunicação com ESP32
  logInfo("Inicializando Serial1 (TX1, RX1) com baud rate: " + String(BAUD_RATE));
  SERIAL_ESP32.begin(BAUD_RATE);
  delay(500);
  
  if(SERIAL_ESP32) {
    logInfo("Serial1 inicializada com sucesso!");
  } else {
    logErro("FALHA ao inicializar Serial1!");
  }
  
  // Inicializar TFT
  logInfo("Inicializando TFT Shield (MCUFRIEND_kbv)...");
  
  uint16_t id = tft.readID();
  logInfo("ID do TFT detectado: 0x" + String(id, HEX));
  
  if(id == 0) {
    logErro("FALHA ao detectar ID do TFT! Usando ID padrao 0x9341");
    id = 0x9341;
  }
  
  tft.begin(id);
  delay(100);
  tft.setRotation(1); // Orientação landscape
  
  tftInicializado = true;
  logInfo("TFT inicializado com sucesso!");
  
  // Mostrar tela inicial
  logInfo("Exibindo tela inicial...");
  mostrarTelaInicial();
  
  logInfo("=== SETUP CONCLUIDO ===");
  logInfo("Aguardando comandos da ESP32...");
  logInfo("Verifique se as conexoes estao corretas:");
  logInfo("  ESP32 TX2 (pino 17) -> Due RX1");
  logInfo("  ESP32 RX2 (pino 16) -> Due TX1");
  logInfo("  GND comum entre as placas");
  
  delay(500);
}

// ========================
// ==== LOOP ==============
void loop() {
  // Ler dados da Serial1 (ESP32)
  if(SERIAL_ESP32.available()) {
    int bytesDisponiveis = SERIAL_ESP32.available();
    logInfo("*** DADOS RECEBIDOS! Bytes disponiveis: " + String(bytesDisponiveis) + " ***");
    
    // Ler todos os bytes disponíveis com timeout
    String linha = "";
    unsigned long inicioLeitura = millis();
    const unsigned long timeoutLeitura = 2000; // 2 segundos de timeout para mensagens longas
    const int tamanhoMaximo = 2048; // Aumentado para suportar perguntas muito longas
    unsigned long ultimoByteRecebido = millis();
    
    // Aguardar até receber \n ou timeout
    while(millis() - inicioLeitura < timeoutLeitura) {
      if(SERIAL_ESP32.available() > 0) {
        char c = SERIAL_ESP32.read();
        if(c == '\n' || c == '\r') {
          break; // Fim da mensagem
        }
        linha += c;
        ultimoByteRecebido = millis();
        // Verificar limite de tamanho
        if(linha.length() >= tamanhoMaximo) {
          logErro("Mensagem muito longa! Limite: " + String(tamanhoMaximo));
          break;
        }
      } else {
        // Se não há dados disponíveis, verificar se passou muito tempo desde o último byte
        if(millis() - ultimoByteRecebido > 200 && linha.length() > 0) {
          // Se já recebeu algo e passou 200ms sem novos dados, considerar mensagem completa
          logDebug("Timeout parcial - mensagem pode estar incompleta");
          break;
        }
      }
      delay(1); // Pequeno delay para não sobrecarregar
    }
    
    linha.trim();
    
    if(linha.length() == 0 && bytesDisponiveis > 0) {
      logErro("Timeout na leitura ou mensagem vazia!");
    }
    
    if(linha.length() > 0) {
      logInfo("Dados brutos recebidos: [" + linha + "]");
      logInfo("Tamanho: " + String(linha.length()) + " caracteres");
      
      // Mostrar primeiros e últimos caracteres
      if(linha.length() > 40) {
        logDebug("Primeiros 20: " + linha.substring(0, 20));
        logDebug("Ultimos 20: " + linha.substring(linha.length() - 20));
      }
      
      if(linha.startsWith("TELA:")) {
        comandoRecebido = linha.substring(5); // Remover "TELA:"
        logInfo("*** COMANDO VALIDO RECEBIDO! ***");
        ultimoComandoRecebido = millis();
        processarComando();
      } else {
        logErro("Dados recebidos nao comecam com 'TELA:'");
        int len = linha.length() > 30 ? 30 : linha.length();
        logErro("Primeiros caracteres recebidos: [" + linha.substring(0, len) + "]");
        if(linha.length() > 0) {
          logErro("Codigo ASCII do primeiro char: " + String((int)linha.charAt(0)));
        }
      }
    } else {
      logDebug("Linha vazia ou apenas quebras de linha recebidas");
    }
  }
  
  delay(10);
}

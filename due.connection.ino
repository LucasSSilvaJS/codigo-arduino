#include <MCUFRIEND_kbv.h>
#include <Adafruit_GFX.h>

MCUFRIEND_kbv tft;

// Cores
#define BLACK   0x0000
#define WHITE   0xFFFF
#define RED     0xF800
#define GREEN   0x07E0
#define BLUE    0x001F
#define YELLOW  0xFFE0
#define CYAN    0x07FF

// ==========================================================
// Funções auxiliares
// ==========================================================
void limparTela() {
    tft.fillScreen(BLACK);
}

int centralizar(String texto, int textSize = 2) {
    int16_t x1, y1;
    uint16_t w, h;
    tft.setTextSize(textSize);
    tft.getTextBounds(texto, 0, 0, &x1, &y1, &w, &h);
    return (240 - w) / 2;
}

// ==========================================================
// Telas
// ==========================================================

// Tela: Aproxime o cartão
void telaAproximeCartao() {
    limparTela();

    tft.setTextColor(CYAN);
    tft.setTextSize(3);

    int x = centralizar("Aproxime", 3);
    tft.setCursor(x, 60);
    tft.print("Aproxime");

    x = centralizar("o cartao", 3);
    tft.setCursor(x, 120);
    tft.print("o cartao");
}

// Tela: Lendo
void telaLendoCartao() {
    limparTela();

    tft.setTextColor(YELLOW);
    tft.setTextSize(3);

    int x = centralizar("Lendo...", 3);
    tft.setCursor(x, 90);
    tft.print("Lendo...");
}

// Tela: Acesso Liberado
void telaAcessoLiberado(String nome) {
    limparTela();

    tft.setTextColor(GREEN);
    tft.setTextSize(3);

    int x = centralizar("ACESSO", 3);
    tft.setCursor(x, 40);
    tft.print("ACESSO");

    x = centralizar("LIBERADO", 3);
    tft.setCursor(x, 100);
    tft.print("LIBERADO");

    tft.setTextColor(WHITE);
    tft.setTextSize(2);

    x = centralizar(nome, 2);
    tft.setCursor(x, 160);
    tft.print(nome);
}

// Tela: Acesso negado
void telaAcessoNegado() {
    limparTela();

    tft.setTextColor(RED);
    tft.setTextSize(3);

    int x = centralizar("ACESSO", 3);
    tft.setCursor(x, 60);
    tft.print("ACESSO");

    x = centralizar("NEGADO", 3);
    tft.setCursor(x, 130);
    tft.print("NEGADO");
}

// Tela: Erro API
void telaErroAPI() {
    limparTela();

    tft.setTextColor(RED);
    tft.setTextSize(3);

    int x = centralizar("ERRO NA", 3);
    tft.setCursor(x, 60);
    tft.print("ERRO NA");

    x = centralizar("API", 3);
    tft.setCursor(x, 130);
    tft.print("API");
}

// Tela: Score Atual
void telaScoreAtual(int score) {
    limparTela();

    tft.setTextColor(YELLOW);
    tft.setTextSize(3);

    int x = centralizar("Seu score:", 3);
    tft.setCursor(x, 40);
    tft.print("Seu score:");

    String str = String(score);
    tft.setTextSize(5);
    x = centralizar(str, 5);
    tft.setCursor(x, 120);
    tft.print(str);
}

// Tela: Finalizado
void telaFinalizado() {
    limparTela();

    tft.setTextColor(GREEN);
    tft.setTextSize(3);

    int x = centralizar("PROCESSO", 3);
    tft.setCursor(x, 70);
    tft.print("PROCESSO");

    x = centralizar("FINALIZADO", 3);
    tft.setCursor(x, 130);
    tft.print("FINALIZADO");
}

// ==========================================================
// SETUP
// ==========================================================
void setup() {
    Serial.begin(115200);

    uint16_t ID = tft.readID();
    tft.begin(ID);
    tft.setRotation(1);

    telaAproximeCartao();  // Tela inicial
}

// ==========================================================
// LOOP PRINCIPAL – OBEDECE O ESP32
// ==========================================================
void loop() {

    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();

        // Comandos de fluxo do ESP32
        if (cmd == "APROXIME") telaAproximeCartao();
        else if (cmd == "LENDO") telaLendoCartao();
        else if (cmd == "LIBERADO") telaAcessoLiberado("Usuario");
        else if (cmd == "NEGADO") telaAcessoNegado();
        else if (cmd == "ERRO_API") telaErroAPI();
        else if (cmd == "FINALIZADO") telaFinalizado();

        // Comando com score
        else if (cmd.startsWith("SCORE:")) {
            int valor = cmd.substring(6).toInt();
            telaScoreAtual(valor);
        }

        // Comando com nome enviado pelo ESP32
        else if (cmd.startsWith("NOME:")) {
            String nome = cmd.substring(5);
            telaAcessoLiberado(nome);
        }
    }
}
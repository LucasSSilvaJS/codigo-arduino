#include <MCUFRIEND_kbv.h>
#include <Adafruit_GFX.h>

MCUFRIEND_kbv tft;

// Cores úteis
#define BLACK   0x0000
#define WHITE   0xFFFF
#define YELLOW  0xFFE0
#define CYAN    0x07FF
#define GREEN   0x07E0
#define RED     0xF800
#define MAGENTA 0xF81F
#define BLUE    0x001F

//---------------------------------------------
// Função: limpar tela
//---------------------------------------------
void limparTela() {
    tft.fillScreen(BLACK);
}

//---------------------------------------------
// Função: centralizar texto dinamicamente
//---------------------------------------------
int centralizar(const char* texto, int textSize = 2) {
    int charWidth = 7 * textSize; // <-- corrigido (6 + 1 espaçamento)
    int textWidth = strlen(texto) * charWidth;
    return (320 - textWidth) / 2;
}

//---------------------------------------------
// Tela 1: Bem-vindo
//---------------------------------------------
void telaBemVindo() {
    limparTela();

    tft.setTextColor(YELLOW);
    tft.setTextSize(3);
    tft.setCursor(20, 40);
    tft.print("Bem vindo ao");

    tft.setCursor(20, 90);
    tft.print("Bussula Cidada!");

    tft.setTextColor(CYAN);
    tft.setTextSize(2);
    tft.setCursor(20, 150);
    tft.print("Aproxime seu VEM");
}

//---------------------------------------------
// Tela 2: Verificando cartão VEM
//---------------------------------------------
void telaVerificandoVEM() {
    limparTela();

    tft.setTextColor(GREEN);
    tft.setTextSize(3);
    tft.setCursor(20, 70);
    tft.print("Verificando");

    tft.setCursor(20, 110);
    tft.print("cartao VEM");

    tft.setTextColor(CYAN);
    tft.setTextSize(2);
    tft.setCursor(20, 170);
    tft.print("Aguarde um momento");
}

//---------------------------------------------
// Tela 3: Exibir hash do cartão VEM
//---------------------------------------------
void telaHashVEM(const char* hashValor) {
    limparTela();

    tft.setTextColor(MAGENTA);
    tft.setTextSize(3);
    tft.setCursor(10, 40);
    tft.print("O hash do seu");

    tft.setCursor(10, 80);
    tft.print("cartao VEM e:");

    tft.setTextColor(CYAN);
    tft.setTextSize(2);
    tft.setCursor(10, 130);
    tft.print(hashValor);

    tft.setTextColor(WHITE);
    tft.setTextSize(2);
    tft.setCursor(10, 190);
    tft.print("Aproxime novamente");
    tft.setCursor(10, 215);
    tft.print("o cartao para continuar");
}

//---------------------------------------------
// Tela 4: Carregando Pergunta
//---------------------------------------------
void telaCarregandoPergunta() {
    limparTela();

    tft.setTextColor(BLUE);
    tft.setTextSize(3);
    tft.setCursor(10, 70);
    tft.print("Carregando");

    tft.setCursor(10, 110);
    tft.print("pergunta");

    tft.setTextColor(CYAN);
    tft.setTextSize(2);
    tft.setCursor(10, 170);
    tft.print("Aguarde um momento");
}

//---------------------------------------------
// Função auxiliar: imprimir texto quebrado
//---------------------------------------------
void printTextoQuebrado(const char* txt, int x, int y, int larguraMax, int textSize) {
    tft.setTextSize(textSize);
    int charWidth = 6 * textSize;
    int maxChars = larguraMax / charWidth;

    char linha[60];
    int idx = 0;

    while (*txt) {
        linha[idx++] = *txt++;

        if (idx >= maxChars || linha[idx - 1] == '\n') {
            linha[idx] = '\0';
            tft.setCursor(x, y);
            tft.print(linha);
            y += textSize * 14;
            idx = 0;
        }
    }

    if (idx > 0) {
        linha[idx] = '\0';
        tft.setCursor(x, y);
        tft.print(linha);
    }
}

//---------------------------------------------
// Tela 5: Exibir pergunta com bordas + SIM/NÃO
//---------------------------------------------
void telaPergunta(const char* pergunta) {
    limparTela();

    // Borda
    tft.drawRect(5, 5, 310, 160, WHITE);
    tft.drawRect(8, 8, 304, 154, WHITE);

    // Conteúdo da pergunta
    tft.setTextColor(WHITE);
    printTextoQuebrado(pergunta, 15, 20, 290, 2);

    // Texto “Voce concorda?”
    tft.setTextColor(CYAN);
    tft.setTextSize(2);
    int x = centralizar("Voce concorda?");
    tft.setCursor(x, 175);
    tft.print("Voce concorda?");

    // Linha com SIM e NAO
    tft.setTextSize(3);

    // SIM
    tft.setTextColor(GREEN);
    int xSim = centralizar("SIM     NAO");
    tft.setCursor(xSim, 205);
    tft.print("SIM");

    // NAO
    tft.setTextColor(RED);
    tft.setCursor(xSim + 85, 205);
    tft.print("NAO");
}

//---------------------------------------------
// Tela 6: Calculando Score
//---------------------------------------------
void telaCalculandoScore() {
    limparTela();

    tft.setTextSize(3);
    tft.setTextColor(YELLOW);
    tft.setCursor(10, 70);
    tft.print("Calculando");

    tft.setCursor(10, 110);
    tft.print("Score");

    tft.setTextSize(2);
    tft.setTextColor(CYAN);
    tft.setCursor(10, 170);
    tft.print("Aguarde um momento");
}

//---------------------------------------------
// Tela 7: Score com gráfico
//---------------------------------------------
void telaScore(int simPercent, int naoPercent) {
    limparTela();

    // ---------- TÍTULO ----------
    tft.setTextColor(YELLOW);
    tft.setTextSize(4);
    tft.setCursor(100, 10);
    tft.print("SCORE");

    // ---------- PARÂMETROS ----------
    int baseY = 160;
    int maxBarHeight = 80;
    int barWidth = 70;

    int simX = 60;
    int naoX = 180;

    int simH = (simPercent * maxBarHeight) / 100;
    int naoH = (naoPercent * maxBarHeight) / 100;

    // ---------- BARRAS ----------
    tft.fillRect(simX, baseY - simH, barWidth, simH, GREEN);
    tft.fillRect(naoX, baseY - naoH, barWidth, naoH, RED);

    // ---------- LABELS ----------
    tft.setTextSize(2);

    tft.setTextColor(GREEN);
    tft.setCursor(simX + 15, baseY + 5);
    tft.print("SIM");

    tft.setTextColor(RED);
    tft.setCursor(naoX + 20, baseY + 5);
    tft.print("NAO");

    // ---------- MENSAGEM ----------
    tft.setTextColor(CYAN);
    tft.setTextSize(2);

    int x1 = centralizar("Aproxime o cartao");
    tft.setCursor(x1, 185);
    tft.print("Aproxime o cartao");

    int x2 = centralizar("para continuar");
    tft.setCursor(x2, 210);
    tft.print("para continuar");
}

//---------------------------------------------
// Tela 8: Exibir score atual (ALINHADO À ESQUERDA)
//---------------------------------------------
void telaScoreAtual(int score) {
    limparTela();

    // Linha 1
    tft.setTextColor(YELLOW);
    tft.setTextSize(3);
    tft.setCursor(20, 40);  // alinhado à esquerda
    tft.print("Seu score:");

    // Linha 2 — Score destacado
    char scoreStr[30];
    sprintf(scoreStr, "%d pontos", score);

    tft.setTextColor(CYAN);
    tft.setTextSize(4);
    tft.setCursor(20, 100);  // esquerda
    tft.print(scoreStr);

    // Linha 3 — mensagem final
    tft.setTextColor(WHITE);
    tft.setTextSize(2);
    tft.setCursor(20, 180);  // esquerda
    tft.print("10 pontos adicionados");
}

//---------------------------------------------
// Tela 9: Agradecimento + redirecionamento (ESQUERDA)
//---------------------------------------------
void telaAgradecimento() {
    limparTela();

    tft.setTextColor(CYAN);
    tft.setTextSize(3);

    tft.setCursor(20, 60);   // esquerda
    tft.print("Agradecendo sua");

    tft.setCursor(20, 100);  // esquerda
    tft.print("participacao");

    tft.setTextColor(YELLOW);
    tft.setTextSize(2);

    tft.setCursor(20, 160);  // esquerda
    tft.print("Redirecionando para");

    tft.setCursor(20, 185);  // esquerda
    tft.print("Tela inicial");
}


//---------------------------------------------
// Setup
//---------------------------------------------
void setup() {
    Serial.begin(9600);

    uint16_t id = tft.readID();
    tft.begin(id);
    tft.setRotation(1); // paisagem

    telaBemVindo();
    delay(2000);

    telaVerificandoVEM();
    delay(2000);

    telaHashVEM("A1F9C33B90D55FFE22AB");
    delay(2000);

    telaCarregandoPergunta();
    delay(2000);

    telaPergunta("Voce concorda com a privatizacao das praias da Zona Sul?");
    delay(3000);

    telaCalculandoScore();
    delay(2000);

    telaScore(63, 37);
    delay(3000);

    telaScoreAtual(72);
    delay(3000);

    telaAgradecimento();
    delay(3000);
}

//---------------------------------------------
// Loop
//---------------------------------------------
void loop() {
}

#include <Keypad.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

// ============================================================
//  URNA ELETRONICA - Arduino Mega + ILI9341 (240x320) + Keypad 4x4
//
//  Teclas:
//    0-9 = digitar numero
//    A   = BRANCO
//    *   = CORRIGE
//    #   = CONFIRMA
// ============================================================

// ---------- Display ----------
#define TFT_CS 10
#define TFT_DC 53
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

const int LARGURA = 240;

// ---------- Cores (RGB565) ----------
#define COR_FUNDO       0xF7BE            // cinza muito claro (como a urna real)
#define COR_TEXTO       ILI9341_BLACK
#define COR_CABECALHO   ILI9341_NAVY
#define COR_DESTAQUE    0x333F            // azul
#define COR_CINZA       0x7BEF
#define COR_CINZA_CLARO 0xDEFB
#define COR_VERDE_ESC   ILI9341_DARKGREEN
#define COR_BTN_BRANCO  ILI9341_WHITE
#define COR_BTN_CORRIGE ILI9341_ORANGE
#define COR_BTN_CONFIRMA ILI9341_GREEN

// ---------- Teclado matricial ----------
const byte ROWS = 4;
const byte COLS = 4;

char hexaKeys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

byte rowPins[ROWS] = {5, 4, 3, 2};
byte colPins[COLS] = {A3, A2, A1, A0};

Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

// ---------- Buzzer (passivo) ----------
const int buzzerPin = 14;

// Som longo de FIM: valores medidos do audio real da urna
const int FREQ_FIM = 2332;   // Hz
const int DUR_FIM  = 1343;   // ms

// Bipe de tecla: AINDA sao valores de teste.
// Para ficar fiel, corte um bipe de tecla no video, rode o analisa.py e troque aqui.
const int FREQ_TECLA = 1000; // Hz
const int DUR_TECLA  = 50;   // ms

// ---------- Estado da votacao ----------
String numeroDigitado = "";
bool votoBranco = false;
int etapaVotacao = 1;        // 1 = Vereador, 2 = Prefeito
String votoCargo1 = "";
String votoCargo2 = "";

const char* nomesCargo[] = {"", "VEREADOR", "PREFEITO"};

// ---------- Candidatos ----------
struct Candidato {
  const char* numero;
  const char* nome;
  const char* partido;
};

Candidato candidatos[] = {
  {"13", "Ana Souza",    "Partido A"},
  {"22", "Carlos Silva", "Partido B"},
  {"45", "Mariana Lima", "Partido C"}
};
const int totalCandidatos = 3;

// ============================================================
//  SETUP / LOOP
// ============================================================
void setup() {
  Serial.begin(9600);
  pinMode(buzzerPin, OUTPUT);

  tft.begin();
  tft.setRotation(0);
  desenharTelaVotacao();
}

void loop() {
  char tecla = customKeypad.getKey();
  if (!tecla) return;

  bipeTecla();

  // Digitos
  if (tecla >= '0' && tecla <= '9') {
    if (!votoBranco && numeroDigitado.length() < 2) {
      numeroDigitado += tecla;
      atualizarCampos();
    }
  }
  // BRANCO
  else if (tecla == 'A') {
    numeroDigitado = "";
    votoBranco = true;
    atualizarCampos();
  }
  // CORRIGE
  else if (tecla == '*') {
    numeroDigitado = "";
    votoBranco = false;
    atualizarCampos();
  }
  // CONFIRMA
  else if (tecla == '#') {
    if (votoBranco || numeroDigitado.length() == 2) {
      confirmarVoto();
    }
  }
}

// ============================================================
//  SONS
// ============================================================
void bipeTecla() {
  tone(buzzerPin, FREQ_TECLA, DUR_TECLA);
}

// Som longo de FIM, com a frequencia e a duracao medidas
void tocarSomUrna() {
  tone(buzzerPin, FREQ_FIM, DUR_FIM);
}

// ============================================================
//  AUXILIARES DE DESENHO
// ============================================================
void textoCentral(const __FlashStringHelper* txt, int y, uint8_t tamanho, uint16_t cor) {
  int largura = strlen_P((PGM_P)txt) * 6 * tamanho;
  tft.setTextSize(tamanho);
  tft.setTextColor(cor);
  tft.setCursor((LARGURA - largura) / 2, y);
  tft.print(txt);
}

// Nome em ate 2 linhas (quebra no primeiro espaco)
void imprimirNome(const char* nome, int x, int y) {
  char buf[24];
  strncpy(buf, nome, sizeof(buf) - 1);
  buf[sizeof(buf) - 1] = 0;

  char* esp = strchr(buf, ' ');
  tft.setTextSize(2);
  tft.setTextColor(COR_VERDE_ESC);
  tft.setCursor(x, y);
  if (esp) {
    *esp = 0;
    tft.print(buf);
    tft.setCursor(x, y + 18);
    tft.print(esp + 1);
  } else {
    tft.print(buf);
  }
}

void desenharBotao(int x, int y, int w, int h, uint16_t cor,
                   const __FlashStringHelper* rotulo, const __FlashStringHelper* tecla) {
  tft.fillRoundRect(x, y, w, h, 5, cor);
  tft.drawRoundRect(x, y, w, h, 5, COR_CINZA);
  tft.setTextColor(COR_TEXTO);
  tft.setTextSize(1);

  int lr = strlen_P((PGM_P)rotulo) * 6;
  tft.setCursor(x + (w - lr) / 2, y + 10);
  tft.print(rotulo);

  int lt = strlen_P((PGM_P)tecla) * 6;
  tft.setCursor(x + (w - lt) / 2, y + 24);
  tft.print(tecla);
}

// ============================================================
//  TELA DE VOTACAO
// ============================================================
void desenharTelaVotacao() {
  tft.fillScreen(COR_FUNDO);

  // ---- Cabecalho ----
  tft.fillRect(0, 0, LARGURA, 34, COR_CABECALHO);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.print(F("URNA ELETRONICA"));

  // Indicador de etapa (2 bolinhas)
  for (int i = 1; i <= 2; i++) {
    int cx = 205 + (i - 1) * 18;
    if (i <= etapaVotacao) {
      tft.fillCircle(cx, 17, 5, ILI9341_YELLOW);
    } else {
      tft.drawCircle(cx, 17, 5, ILI9341_WHITE);
    }
  }

  // ---- Cargo ----
  tft.setTextSize(1);
  tft.setTextColor(COR_CINZA);
  tft.setCursor(15, 46);
  tft.print(F("SEU VOTO PARA"));
  tft.setCursor(LARGURA - 15 - 12 * 6, 46);
  tft.print(F("Cargo "));
  tft.print(etapaVotacao);
  tft.print(F(" de 2"));

  tft.setTextSize(3);
  tft.setTextColor(COR_TEXTO);
  tft.setCursor(15, 60);
  tft.print(nomesCargo[etapaVotacao]);

  tft.drawFastHLine(15, 92, LARGURA - 30, COR_CINZA_CLARO);

  // ---- Rotulo do numero ----
  tft.setTextSize(1);
  tft.setTextColor(COR_CINZA);
  tft.setCursor(15, 100);
  tft.print(F("NUMERO"));

  // ---- Botoes do rodape ----
  desenharBotao(9,   274, 70, 40, COR_BTN_BRANCO,   F("BRANCO"),   F("[A]"));
  desenharBotao(85,  274, 70, 40, COR_BTN_CORRIGE,  F("CORRIGE"),  F("[*]"));
  desenharBotao(161, 274, 70, 40, COR_BTN_CONFIRMA, F("CONFIRMA"), F("[#]"));

  atualizarCampos();
}

void atualizarCampos() {
  desenharCaixasNumero();
  desenharAreaCandidato();
}

void desenharCaixasNumero() {
  for (int i = 0; i < 2; i++) {
    int x = 15 + i * 47;
    int y = 112;

    tft.fillRoundRect(x, y, 40, 46, 4, ILI9341_WHITE);

    // Borda: azul (dupla) na caixa ativa, cinza nas demais
    bool ativa = (!votoBranco && i == (int)numeroDigitado.length());
    uint16_t corBorda = ativa ? COR_DESTAQUE : COR_CINZA;
    tft.drawRoundRect(x, y, 40, 46, 4, corBorda);
    if (ativa) tft.drawRoundRect(x + 1, y + 1, 38, 44, 4, corBorda);

    if (i < (int)numeroDigitado.length()) {
      tft.setTextSize(4);
      tft.setTextColor(COR_TEXTO);
      tft.setCursor(x + 8, y + 7);
      tft.print(numeroDigitado[i]);
    }
  }
}

void desenharAreaCandidato() {
  // Limpa a area
  tft.fillRect(10, 166, 220, 100, COR_FUNDO);

  // Voto em branco
  if (votoBranco) {
    textoCentral(F("VOTO EM BRANCO"), 208, 2, COR_TEXTO);
    return;
  }

  // Aguardando digitos
  if (numeroDigitado.length() < 2) {
    textoCentral(F("Digite o numero"), 205, 1, COR_CINZA);
    textoCentral(F("do candidato"), 218, 1, COR_CINZA);
    return;
  }

  // Numero completo: busca candidato
  Candidato* c = buscarCandidato(numeroDigitado);

  if (c != nullptr) {
    // Cartao do candidato
    tft.fillRoundRect(15, 170, 210, 92, 6, ILI9341_WHITE);
    tft.drawRoundRect(15, 170, 210, 92, 6, COR_CINZA);

    // "Foto" (silhueta)
    tft.fillRect(23, 178, 64, 76, COR_CINZA_CLARO);
    tft.fillCircle(55, 204, 13, COR_CINZA);
    tft.fillRoundRect(33, 222, 44, 32, 14, COR_CINZA);
    tft.fillRect(23, 254, 64, 1, COR_CINZA_CLARO); // acabamento
    tft.drawRect(23, 178, 64, 76, COR_CINZA);

    // Dados
    tft.setTextSize(1);
    tft.setTextColor(COR_CINZA);
    tft.setCursor(97, 180);
    tft.print(F("Candidato(a)"));

    imprimirNome(c->nome, 97, 194);

    tft.setTextSize(1);
    tft.setTextColor(COR_CINZA);
    tft.setCursor(97, 232);
    tft.print(F("Partido"));

    tft.setTextSize(2);
    tft.setTextColor(COR_TEXTO);
    tft.setCursor(97, 243);
    tft.print(c->partido);
  } else {
    textoCentral(F("NUMERO ERRADO"), 195, 2, ILI9341_RED);
    textoCentral(F("VOTO NULO"), 222, 2, ILI9341_RED);
  }
}

Candidato* buscarCandidato(const String& num) {
  for (int i = 0; i < totalCandidatos; i++) {
    if (num == candidatos[i].numero) {
      return &candidatos[i];
    }
  }
  return nullptr;
}

// ============================================================
//  FLUXO DE VOTACAO
// ============================================================
void confirmarVoto() {
  String voto = votoBranco ? String("BRANCO") : numeroDigitado;

  if (etapaVotacao == 1) {
    votoCargo1 = voto;
    etapaVotacao = 2;
    numeroDigitado = "";
    votoBranco = false;
    delay(300);
    desenharTelaVotacao();
  } else {
    votoCargo2 = voto;
    processarFimVotacao();
  }
}

void processarFimVotacao() {
  tft.fillScreen(COR_FUNDO);

  // FIM grande (3 letras x 48 px = 144 px)
  tft.setTextSize(8);
  tft.setTextColor(COR_TEXTO);
  tft.setCursor((LARGURA - 144) / 2, 90);
  tft.print(F("FIM"));

  // Faixa "VOTOU"
  tft.fillRoundRect(40, 185, 160, 40, 8, COR_BTN_CONFIRMA);
  tft.drawRoundRect(40, 185, 160, 40, 8, COR_VERDE_ESC);
  textoCentral(F("VOTOU"), 195, 3, COR_TEXTO);

  textoCentral(F("Voto computado com sucesso!"), 245, 1, COR_CINZA);

  // Som longo medido do audio real
  tocarSomUrna();

  // Registro na serial (aqui voce pode salvar na EEPROM ou no cartao SD)
  Serial.print(F("Vereador: "));
  Serial.print(votoCargo1);
  Serial.print(F(" | Prefeito: "));
  Serial.println(votoCargo2);

  delay(DUR_FIM + 1200);

  // Reseta para o proximo eleitor
  etapaVotacao = 1;
  numeroDigitado = "";
  votoBranco = false;
  votoCargo1 = "";
  votoCargo2 = "";
  desenharTelaVotacao();
}

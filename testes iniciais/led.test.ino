#define LED1_PIN 15
#define LED2_PIN 14

void setup() {
  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);

  // Mantém ambos desligados no boot
  digitalWrite(LED1_PIN, LOW);
  digitalWrite(LED2_PIN, LOW);

  delay(1000);  // aguarda boot completo

  // Liga os LEDs permanentemente
  digitalWrite(LED1_PIN, HIGH);
  digitalWrite(LED2_PIN, HIGH);
}

void loop() {
  // LEDs permanecem ligados
}

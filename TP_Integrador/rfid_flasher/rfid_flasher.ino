#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN 10
#define RST_PIN 9

MFRC522 rfid(SS_PIN, RST_PIN);

void setup() {
  Serial.begin(9600);

  SPI.begin();
  rfid.PCD_Init();

  Serial.println("Acerque una tarjeta RFID al lector...");
}

void loop() {
  // Verifica si hay una nueva tarjeta presente
  if (!rfid.PICC_IsNewCardPresent()) {
    return;
  }

  // Intenta leer la tarjeta
  if (!rfid.PICC_ReadCardSerial()) {
    return;
  }

  Serial.print("UID detectado: ");

  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) {
      Serial.print("0");
    }

    Serial.print(rfid.uid.uidByte[i], HEX);

    if (i < rfid.uid.size - 1) {
      Serial.print(" ");
    }
  }

  Serial.println();

  // Detiene la comunicación con la tarjeta actual
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();

  delay(1000);
}

#include "rfid_reader.h"
#include "config.h"
#include <MFRC522.h>
#include <SPI.h>

static MFRC522 mfrc522(RC522_SS_PIN, RC522_RST_PIN);

void initRfid() {
    SPI.begin();
    mfrc522.PCD_Init();
    delay(4);
    mfrc522.PCD_DumpVersionToSerial();
}

bool readUid(String& uid) {
    if (!mfrc522.PICC_IsNewCardPresent()) return false;

    Serial.println("[RFID] Tarjeta detectada, leyendo serial...");

    if (!mfrc522.PICC_ReadCardSerial()) {
        Serial.println("[RFID] WARN: PICC_ReadCardSerial falló.");
        return false;
    }

    char buf[15] = {};
    Serial.print("[RFID] UID bytes: ");
    for (byte i = 0; i < mfrc522.uid.size; i++) {
        Serial.printf("%02X ", mfrc522.uid.uidByte[i]);
        snprintf(buf + i * 2, 3, "%02X", mfrc522.uid.uidByte[i]);
    }
    Serial.println();
    uid = buf;

    MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
    Serial.printf("[RFID] UID: %s  tipo: %s\n",
                  uid.c_str(),
                  mfrc522.PICC_GetTypeName(piccType));

    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
    return true;
}

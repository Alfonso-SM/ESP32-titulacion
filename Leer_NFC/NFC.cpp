// NFC.cpp

#include "NFC.h"
uint8_t ndefBuf[128];
uint8_t recordBuf[128];
String tags[] = {"", "", ""};
UNIT_PN532  nfc(PN532_SS);

NFC::NFC() {
  // Constructor implementation
}

void NFC::init() {
  Serial.begin(115200);
  /*Configuracion nfc*/
  nfc.begin();
  uint32_t versiondata = nfc.getFirmwareVersion();
  while (! versiondata){
    Serial.print("Didn't find PN53x board\n");
    nfc.begin();
    versiondata = nfc.getFirmwareVersion();
  }
  
  nfc.SAMConfig();
}

bool NFC::READ_NFC() {
  bool success;
  uint8_t responseLength = 32;
  success = nfc.inListPassiveTarget();
  if (success) {
    uint8_t selectApdu[] = { 0x00, /* CLA */
                             0xA4, /* INS */
                             0x04, /* P1  */
                             0x00, /* P2  */
                             0x07, /* Length of AID  */
                             0xA0, 0x00, 0x00, 0x03, 0x27, 0x80, 0x70, /* AID defined on Android App */
                           };
    uint8_t response[2];
    success = nfc.inDataExchange(selectApdu, sizeof(selectApdu), response, &responseLength);
    if (success) {
     return printResponse(response, responseLength);
    }
    else {
      ChecarSiEsTarjeta();
    }
  }
}

bool NFC::ChecarSiEsTarjeta() {
  boolean success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };
  uint8_t uidLength;
  String check = "";
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength);
  if (success) {
    for (uint8_t i = 0; i < uidLength; i++)
    {
      check += String(uid[i], HEX);
    }
    check.toUpperCase();
    for (int i = 0; i < sizeof(tags); i++) {
      if ( check == tags[i] ){
        return true;
      }
    }
  }
  return false;
}

void NFC::tarjetas(String respuesta) {   // Tarjetas definidas desde Firebase
  String nfcCardsRead ;
  int i = respuesta.lastIndexOf("IdNfc 0");
  if (i > 0) {
    nfcCardsRead = respuesta.substring(i + 10, i + 18);
    tags[0] = nfcCardsRead;
  }
  i = 0;
  i = respuesta.lastIndexOf("IdNfc 1");
  if (i > 0) {
    nfcCardsRead = respuesta.substring(i + 10, i + 18);
    tags[1] = nfcCardsRead;
  }
  i = 0;
  i = respuesta.lastIndexOf("IdNfc 2");
  if (i > 0) {
    nfcCardsRead = respuesta.substring(i + 10, i + 18);
    tags[2] = nfcCardsRead;
  }
}


bool NFC::printResponse(uint8_t *response, uint8_t responseLength) {
  String respBuffer = "";
  for (int i = 0; i < responseLength; i++) {
    //if (response[i] < 0x10)
    // respBuffer = respBuffer + "0"; //Adds leading zeros if hex value is smaller than 0x10
    respBuffer += String(response[i], HEX);
  }

  if (respBuffer == "9898") {
    return true;
  }
  else {
    return false;
  }
}

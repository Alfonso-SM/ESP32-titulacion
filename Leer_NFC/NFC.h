// NFC.h

#ifndef NFC_h
#define NFC_h

#include <UNIT_PN532.h>
#include <SPI.h>         //Conexiones SPI del ESP32 de NFC
#define PN532_SCK  (18)
#define PN532_MOSI (23)
#define PN532_SS   (5)
#define PN532_MISO (19)


class NFC {
public:
  NFC();
  void init();
  bool READ_NFC();
  void tarjetas(String respuesta);
private:
  bool ChecarSiEsTarjeta();
  void Autorizado();
  void NoAutorizado();
  bool printResponse(uint8_t *response, uint8_t responseLength);
};

#endif
// GPS_Telcel.h

#ifndef GPS_Telcel_h
#define GPS_Telcel_h
/***********************************GPS y Telcel**********************************************/
// Set serial for AT commands
#define TINY_GSM_MODEM_SIM7000SSL  // Modelo usado para poder entrar a firebase
#define TINY_GSM_RX_BUFFER 1024
#define SerialMon Serial
#define SerialAT Serial1
/*SIM7000*/
#define uS_TO_S_FACTOR      1000000ULL
#define UART_BAUD           9600
#define PIN_TX              27
#define PIN_RX              26
#define PWR_PIN             4

#include <TinyGsmClient.h>
#include <ArduinoHttpClient.h>


class GPS_Telcel {
public:
  GPS_Telcel();
  void doSomething();
  void gpsPost();
  bool SendGPS(float lat, float lon);
  void setConnection();
  void ReadFromFirebase();
  void PostToFirebase(const char* method, const String & data);
  void CheckState();

private:
  void modemPowerOn();
};

#endif
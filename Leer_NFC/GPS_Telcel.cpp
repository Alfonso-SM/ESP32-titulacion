// GPS_Telcel.cpp

#include "GPS_Telcel.h"

#include "NFC.h"
#include "States.h"

// Firebase credentials
const char FIREBASE_HOST[]  = "car-key-a7d5e-default-rtdb.firebaseio.com";
const String FIREBASE_AUTH  = "5IGAXcHNdnpj4iO2d41i3OjiHPBta0MKngoe0ptX";
const String PathPrimary    = "/NFC/3278070";
const int SSL_PORT          = 443;
//*********** GPRS credentials And Firebase data  **********************************//
char apn[]  = "internet.itelcel.com";
char user[] = "webgprs";
char pass[] = "webgprs2002";
unsigned long previousMillis = 0;
long interval = 10000;
/*Variables para poder configurar el sim7000SSL*/
int counter, lastIndex, numberOfPieces = 24;
String pieces[24], input, url;
bool StatusOfGPS= false;
float lat;
float lon;

TinyGsm        modem(SerialAT);
TinyGsmClientSecure gsm_client_secure_modem(modem);
HttpClient http_client = HttpClient(gsm_client_secure_modem, FIREBASE_HOST, SSL_PORT);
NFC libNFC;
States ChangeState;

GPS_Telcel::GPS_Telcel() {
  // Constructor implementation
}

void GPS_Telcel::doSomething() {
  // Function implementation
}


void  GPS_Telcel::gpsPost() {
  modem.sendAT("+SGPIO=0,4,1,1");
  modem.enableGPS();
  StatusOfGPS = true ;
}

void GPS_Telcel::CheckState(){
  if(modem.getGPS(&lat, &lon) && StatusOfGPS){
    SendGPS(lat, lon);
    lat= NULL;
    lon = NULL;
  }
}

bool GPS_Telcel::SendGPS(float lat1, float lon1){
      SerialAT.write(Serial.read());
      Serial.println("Se obtuvo las cordenadas");
      //disableGPS();
      String gpsData = "";
      gpsData = "{";
      gpsData += "\"latitud\":" + String(lat1, 10) + ",";
      gpsData += "\"longitud\":" + String(lon1, 10) + ",";
      gpsData += "\"GPS\":" + String(0) + "";
      gpsData += "}";
      //PATCH   Update some of the keys for a defined path without replacing all of the data.
      PostToFirebase("PATCH", gpsData);
      StatusOfGPS = false;
}



void GPS_Telcel::setConnection() {    // Se crea la conexion a internet con el SIM7000
  /*Inicializar sim7000*/
  SerialAT.begin(UART_BAUD, SERIAL_8N1, PIN_RX, PIN_TX);
  modemPowerOn();
  modem.init();
  modem.sendAT("+SGPIO=0,4,1,0");
  modem.sendAT("+CFUN=0 ");
  delay(200);
  String res;
  res = modem.setNetworkMode(2);
  if (res != "1") {
    DBG("setNetworkMode  false ");
    return ;
  }
  delay(200);
  res = modem.setPreferredMode(2);
  if (res != "1") {
    DBG("setPreferredMode  false ");
    return ;
  }
  delay(200);
  modem.sendAT("+CFUN=1 ");
  if (modem.waitResponse(10000L) != 1) {
    DBG(" +CFUN=1  false ");
  }
  delay(200);

  SerialAT.println("AT+CGDCONT?");
  delay(500);
  if (SerialAT.available()) {
    input = SerialAT.readString();
    for (int i = 0; i < input.length(); i++) {
      if (input.substring(i, i + 1) == "\n") {
        pieces[counter] = input.substring(lastIndex, i);
        lastIndex = i + 1;
        counter++;
      }
      if (i == input.length() - 1) {
        pieces[counter] = input.substring(lastIndex, i);
      }
    }
    input = "";
    counter = 0;
    lastIndex = 0;
    for ( int y = 0; y < numberOfPieces; y++) {
      for ( int x = 0; x < pieces[y].length(); x++) {
        char c = pieces[y][x];  //gets one byte from buffer
        if (c == ',') {
          if (input.indexOf(": ") >= 0) {
            String data = input.substring((input.indexOf(": ") + 1));
            if ( data.toInt() > 0 && data.toInt() < 25) {
              modem.sendAT("+CGDCONT=" + String(data.toInt()) + ",\"IP\",\"" + String(apn) + "\",\"0.0.0.0\",0,0,0,0");
            }
            input = "";
            break;
          }
          // Reset for reuse
          input = "";
        } else {
          input += c;
        }
      }
    }
  } else {
    Serial.println("Failed to get PDP!");
  }
  if (!modem.waitForNetwork()) {
    delay(1000);
    return;
  }
  if (!modem.gprsConnect(apn, user, pass)) {
    delay(10000);
    return;
  }
  int csq = modem.getSignalQuality();
  Serial.println("Signal quality: " + String(csq));
  SerialAT.println("AT+CPSI?");     //Get connection type and band
  delay(500);
  if (SerialAT.available()) {
    String r = SerialAT.readString();
    Serial.println(r);
  }
  http_client.connect(FIREBASE_HOST, SSL_PORT);
  http_client.connectionKeepAlive();
  url = "";
  url += PathPrimary + ".json";
  url += "?auth=" + FIREBASE_AUTH;
  ReadFromFirebase();
}

void GPS_Telcel::PostToFirebase(const char* method, const String & data) {
  Serial.println("post : " + data);
  String contentType = "application/json";
  http_client.patch(url, contentType, data);
}

void GPS_Telcel::ReadFromFirebase() {
  String response;
  String gpsStatus = "0";
  String AperturaStatus = "0";
  String IgnicionStatusOn = "0";
  String IgnicionStatusOff = "0";
  http_client.get(url);
  response = http_client.responseBody();
  Serial.println(response);
  if (response.length() == 0) {
    return;
  }
  int i = response.lastIndexOf("GPS");
  gpsStatus = response.substring(i + 6, i + 7);
  if (gpsStatus == "1") {
    gpsPost();
  }
  i = 0;
  i = response.lastIndexOf("Estado Actual apertura");
  if (response.substring(i + 25, i + 30) == "Abrir") {
    ChangeState.OpenDoors();
    String ActualState = "";
    ActualState = "{";
    ActualState += "\"Estado Actual apertura\":" + String(0) + "";
    ActualState += "}";
    PostToFirebase("PATCH", ActualState);
  } else if (response.substring(i + 25, i + 31) == "Cerrar") {
    ChangeState.CloseDoors();
    String ActualState = "";
    ActualState = "{";
    ActualState += "\"Estado Actual apertura\":" + String(0) + "";
    ActualState += "}";
    PostToFirebase("PATCH", ActualState);
  }
  i = 0;
  i = response.lastIndexOf("Estado Actual Motor");
  IgnicionStatusOn = response.substring(i + 22, i + 29);
  IgnicionStatusOff = response.substring(i + 22, i + 28);
  if (IgnicionStatusOn == "Prender") {
    ChangeState.TurnOnCar();
    String ActualState = "";
    ActualState = "{";
    ActualState += "\"Estado Actual Motor\":" + String(0) + "";
    ActualState += "}";
    PostToFirebase("PATCH", ActualState);
  } else if (IgnicionStatusOff == "Apagar") {
    ChangeState.CloseDoors();
    String ActualState = "";
    ActualState = "{";
    ActualState += "\"Estado Actual Motor\":" + String(0) + "";
    ActualState += "}";
    PostToFirebase("PATCH", ActualState);
  }
  i = response.lastIndexOf("nfcCard");
  int j = response.lastIndexOf("},");
  String nfcCardsRead = response.substring(i + 10, j - 1);
  libNFC.tarjetas(nfcCardsRead);
}

void disableGPS() { // Se deshabilita el GPS con el Comando AT+SGPIO=0,4,1,0   El ultimo 0 nos indica que se apagara
  modem.sendAT("+SGPIO=0,4,1,0");
  if (modem.waitResponse(10000L) != 1)
    DBG(" SGPIO=0,4,1,0 false ");
  modem.disableGPS();
}

void GPS_Telcel::modemPowerOn() {  // Se prende el Sim7000 para que sea funcional este esta conectado al pin 4 del esp32
  pinMode(PWR_PIN, OUTPUT);
  digitalWrite(PWR_PIN, LOW);
  delay(1000);
  digitalWrite(PWR_PIN, HIGH);
}


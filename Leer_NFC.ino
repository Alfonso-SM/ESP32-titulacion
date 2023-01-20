#include <SPI.h>
#include <UNIT_PN532.h>
#include <Servo.h>
/* numero de pines*/
#define ChipNoAuth 80
#define ChipAuth 55
#define ChipPin  22
#define Lock 21
#define Unlock 0
#define Button 32
#define Acc1 14
#define Acc2 15
#define Run 13
#define Starter 12
//Conexiones SPI del ESP32
#define PN532_SCK  (18)
#define PN532_MOSI (23)
#define PN532_SS   (5)
#define PN532_MISO (19)
/*SIM7000*/
#define uS_TO_S_FACTOR      1000000ULL
#define UART_BAUD           9600
#define PIN_TX              27
#define PIN_RX              26
#define PWR_PIN             4
/***********************************GPS y Telcel**********************************************/
// Set serial for AT commands
#define TINY_GSM_MODEM_SIM7000SSL  // Modelo usado para poder entrar a firebase
#define TINY_GSM_RX_BUFFER 1024
#define SerialMon Serial
#define SerialAT Serial1
#include <TinyGsmClient.h>
#include <ArduinoHttpClient.h>
// Firebase credentials
const char FIREBASE_HOST[]  = "car-key-a7d5e-default-rtdb.firebaseio.com";
const String FIREBASE_AUTH  = "5IGAXcHNdnpj4iO2d41i3OjiHPBta0MKngoe0ptX";
const String PathPrimary    = "/NFC/3278070";
const int SSL_PORT          = 443;
//*********** GPRS credentials And Firebase data  **********************************//
char apn[]  = "internet.itelcel.com";
char user[] = "webgprs";
char pass[] = "webgprs2002";
TinyGsm        modem(SerialAT);
TinyGsmClientSecure gsm_client_secure_modem(modem);
HttpClient http_client = HttpClient(gsm_client_secure_modem, FIREBASE_HOST, SSL_PORT);
unsigned long previousMillis = 0;
long interval = 10000;
/**********************************************************************************************/
UNIT_PN532  nfc(PN532_SS);
Servo Chip;
uint8_t ndefBuf[128];
uint8_t recordBuf[128];
String tags[] = {"", "", ""};
bool autorizado = false;
bool aurotizadoCel = false;
bool DoorOpen = false;
bool RunStatus = false;
bool StartStatus = false;
bool StatusOpen = false;
bool prueba = false;
/*Timer cada un minuto para poder checar la base de datos*/
hw_timer_t * timer = NULL;
#define OneMin 60000000
void IRAM_ATTR FirabseCheckGPS(void);
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
volatile int interruptCounter;
volatile int interrupcionBoton = 0;

/*Variables para poder configurar el sim7000SSL*/
int counter, lastIndex, numberOfPieces = 24;
String pieces[24], input, url;

void setup()
{
  pines();
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
  /*Inicializar sim7000*/
  modemPowerOn();
  SerialAT.begin(UART_BAUD, SERIAL_8N1, PIN_RX, PIN_TX);
  setConnection();
  /*Interrupcion por tiempo para firbase*/
  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &FirabseCheckGPS, true);
  timerAlarmWrite(timer, OneMin, true);
  timerAlarmEnable(timer);
}

void loop()
{
  NFC();
  if (interruptCounter > 0) {
    portENTER_CRITICAL(&timerMux);
    interruptCounter = 0;
    portEXIT_CRITICAL(&timerMux);
    ReadFromFirebase();
  }
  if (interrupcionBoton == 1) {
    ChecarBoton();
  }
}

void IRAM_ATTR FirabseCheckGPS() {
  portENTER_CRITICAL_ISR(&timerMux);
  interruptCounter++;
  portEXIT_CRITICAL_ISR(&timerMux);
}

void printResponse(uint8_t *response, uint8_t responseLength) {
  String respBuffer = "";
  for (int i = 0; i < responseLength; i++) {
    //if (response[i] < 0x10)
    // respBuffer = respBuffer + "0"; //Adds leading zeros if hex value is smaller than 0x10
    respBuffer += String(response[i], HEX);
  }

  if (respBuffer == "9898") {
    Autorizado();
    autorizado = true;
    aurotizadoCel = true;

  }
  else {
    autorizado = false;
    NoAutorizado();
  }
  delay(1000);
}

void ChecarSiEsTarjeta() {
  boolean success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };
  uint8_t uidLength;
  String check = "";
  autorizado = false;
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength);
  if (success) {
    for (uint8_t i = 0; i < uidLength; i++)
    {
      check += String(uid[i], HEX);
    }
    check.toUpperCase();
    for (int i = 0; i < sizeof(tags); i++) {
      if ( check == tags[i] )
        autorizado = true;
    }
    if (autorizado)
      Autorizado();
    else {
      NoAutorizado();
      autorizado = false;
    }
    delay(1000);
  }
}

void Autorizado() {
  if (!DoorOpen) {  // Si se detecta autorizado y no esta abierto
    OpenDoors();
    timerAlarmDisable(timer);
  } else {
    Chip.write(ChipNoAuth);
    aurotizadoCel = false;
    timerWrite(timer, 0);
    timerAlarmEnable(timer);
    CloseDoors();
  }
}

void NoAutorizado() {
  Chip.write(ChipNoAuth);
  if (DoorOpen)
    CloseDoors();
}

void OpenDoors() {
  Chip.write(ChipAuth);
  delay(650);
  if (!RunStatus) {
    digitalWrite(Acc1 , LOW);
    digitalWrite(Acc2 , LOW);
    digitalWrite(Run , LOW);
    RunStatus = true;
  }
  digitalWrite(Unlock, LOW);
  delay(1500);
  digitalWrite(Unlock, HIGH);
  StatusOpen = true;
  DoorOpen = true;
}

void CloseDoors() {
  Chip.write(ChipNoAuth);
  digitalWrite(Lock, LOW);
  delay(200);
  digitalWrite(Lock, HIGH);
  DoorOpen = false;
  digitalWrite(Run, HIGH);
  digitalWrite( Acc1, HIGH);
  digitalWrite( Acc2, HIGH);
  StatusOpen = false;
  RunStatus = false;
  StartStatus = false;
  autorizado = false;
  gpsPost(); 
}

void IRAM_ATTR ChecarBotonInterrupcion() {
  interrupcionBoton++;
  if (interrupcionBoton > 1) {
    interrupcionBoton = 0;
  }
}


void ChecarBoton() {
  interrupcionBoton = 0;
  if (autorizado ) {
    if (!StartStatus) {
      if (RunStatus) {
        digitalWrite(Acc1, HIGH);
        digitalWrite(Starter, LOW);
        delay(400);
        digitalWrite(Starter, HIGH);
        digitalWrite(Acc1, LOW);
        StartStatus = true;
      } else {
        if (StatusOpen) {
          digitalWrite(Run, LOW);
          digitalWrite(Acc2, LOW);
          RunStatus = true;
        }
      }
    } else {
      StartStatus = false;
      RunStatus = false;
      digitalWrite(Run, HIGH);
      digitalWrite(Acc2, HIGH);
      prueba = false;
                                                                                                    //////////////////////////////////////////////////
    }
  }
}

void TurnOnCar() {
  autorizado = true;
  Chip.write(ChipAuth);
  delay(200);
  digitalWrite(Run, LOW);
  digitalWrite(Acc2, LOW);
  RunStatus = true;
  delay(1500);
  digitalWrite(Acc1, HIGH);
  digitalWrite(Starter, LOW);
  delay(400);
  digitalWrite(Starter, HIGH);
  digitalWrite(Acc1, LOW);
  StartStatus = true;
}

void tarjetas(String respuesta) {   // Tarjetas definidas desde Firebase
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

void pines() {
  pinMode( Starter , OUTPUT);
  pinMode( Run , OUTPUT);
  pinMode(Acc1 , OUTPUT);
  pinMode(Acc2 , OUTPUT);
  pinMode(Unlock , OUTPUT);
  pinMode( Lock , OUTPUT);
  pinMode(Button, INPUT);
  attachInterrupt(digitalPinToInterrupt(Button), ChecarBotonInterrupcion, RISING);
  digitalWrite(Run , HIGH);
  digitalWrite(Starter, HIGH);
  digitalWrite(Acc1 , HIGH);
  digitalWrite(Acc2 , HIGH);
  digitalWrite(Unlock , HIGH);
  digitalWrite(Lock , HIGH);
  Chip.attach(ChipPin);
  Chip.write(ChipNoAuth);
}

void NFC() {
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
      printResponse(response, responseLength);
    }
    else {
      ChecarSiEsTarjeta();
    }
  }
}


void enableGPS(void) {  // Se habilita el GPS con el Comando AT+SGPIO=0,4,1,1   El ultimo 1 nos indica que se prendera
  modem.sendAT("+SGPIO=0,4,1,1");
  if (modem.waitResponse(10000L) != 1)
    DBG(" SGPIO=0,4,1,1 false ");
  modem.enableGPS();
}

void disableGPS(void) { // Se deshabilita el GPS con el Comando AT+SGPIO=0,4,1,0   El ultimo 0 nos indica que se apagara
  modem.sendAT("+SGPIO=0,4,1,0");
  if (modem.waitResponse(10000L) != 1)
    DBG(" SGPIO=0,4,1,0 false ");
  modem.disableGPS();
}

void modemPowerOn() {  // Se prende el Sim7000 para que sea funcional este esta conectado al pin 4 del esp32
  pinMode(PWR_PIN, OUTPUT);
  digitalWrite(PWR_PIN, LOW);
  delay(1000);
  digitalWrite(PWR_PIN, HIGH);
}

void setConnection() {    // Se crea la conexion a internet con el SIM7000
  modem.init();
  modem.sendAT("+SGPIO=0,4,1,0");
  if (modem.waitResponse(10000L) != 1) {
    DBG(" SGPIO=0,4,1,0 false ");
  }
  modem.sendAT("+CFUN=0 ");
  if (modem.waitResponse(10000L) != 1) {
    DBG(" +CFUN=0  false ");
  }
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

void PostToFirebase(const char* method, const String & data) {
  Serial.println("post : " + data);
  String contentType = "application/json";
  http_client.patch(url, contentType, data);
}

void ReadFromFirebase() {
  String response;
  String gpsStatus = "0";
  String AperturaStatus = "0";
  String IgnicionStatusOn = "0";
  String IgnicionStatusOff = "0";
  http_client.get(url);
  response = http_client.responseBody();
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
    OpenDoors();
    String ActualState = "";
    ActualState = "{";
    ActualState += "\"Estado Actual apertura\":" + String(0) + "";
    ActualState += "}";
    PostToFirebase("PATCH", ActualState);
  } else if (response.substring(i + 25, i + 31) == "Cerrar") {
    CloseDoors();
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
    TurnOnCar();
    String ActualState = "";
    ActualState = "{";
    ActualState += "\"Estado Actual Motor\":" + String(0) + "";
    ActualState += "}";
    PostToFirebase("PATCH", ActualState);
  } else if (IgnicionStatusOff == "Apagar") {
    CloseDoors();
    String ActualState = "";
    ActualState = "{";
    ActualState += "\"Estado Actual Motor\":" + String(0) + "";
    ActualState += "}";
    PostToFirebase("PATCH", ActualState);
  }
  i = response.lastIndexOf("nfcCard");
  int j = response.lastIndexOf("},");
  String nfcCardsRead = response.substring(i + 10, j - 1);
  tarjetas(nfcCardsRead);
}

void gpsPost() {
  enableGPS();
  float lat;
  float lon;
  timerAlarmDisable(timer);  // deshabilitamos la interrupcion para leer firebase ya que se puede llegar a tardar mas de un minuto esta funcion
  while (1) {
    if (modem.getGPS(&lat, &lon)) {
      SerialAT.write(Serial.read());
      Serial.println("Se obtuvo las cordenadas");
      break;
    }
  }
  disableGPS();
  String gpsData = "";
  gpsData = "{";
  gpsData += "\"latitud\":" + String(lat, 10) + ",";
  gpsData += "\"longitud\":" + String(lon, 10) + ",";
  gpsData += "\"GPS\":" + String(0) + "";
  gpsData += "}";
  //PATCH   Update some of the keys for a defined path without replacing all of the data.
  PostToFirebase("PATCH", gpsData);
  timerWrite(timer, 0);
  timerAlarmEnable(timer);
}

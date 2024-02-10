#include "GPS_Telcel.h"
#include "States.h"
#include "NFC.h"


/**********************************************************************************************/
GPS_Telcel telcel;
States state;
NFC nfcLib;

/*Timer cada un minuto para poder checar la base de datos*/
hw_timer_t * timer = NULL;
#define OneMin 10000000
void IRAM_ATTR FirabseCheckGPS(void);
void IRAM_ATTR ChecarBotonInterrupcion(void);
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
volatile int interruptCounter;
volatile int interrupcionBoton = 0;



void setup()
{
  state.pines();
  nfcLib.init();
  telcel.setConnection();
  initInterr();
  attachInterrupt(digitalPinToInterrupt(Button), ChecarBotonInterrupcion, RISING);
}

void loop()
{
  if(nfcLib.READ_NFC()){
   state.OpenDoors();
   // timerAlarmDisable(timer);
   // telcel.gpsPost();
    interrupcionBoton = 0;
  }
  if (interruptCounter > 0) {
    nfcLib.READ_NFC();
    portENTER_CRITICAL(&timerMux);
    interruptCounter = 0;
    interrupcionBoton = 0;
    portEXIT_CRITICAL(&timerMux);
    telcel.ReadFromFirebase();
  }
  if (interrupcionBoton == 1) {
    state.ChecarBoton();
  }
  telcel.CheckState();
  
  interrupcionBoton = 0;
}


void initInterr(){
   /*Interrupcion por tiempo para firbase*/
  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &FirabseCheckGPS, true);
  timerAlarmWrite(timer, OneMin, true);
  timerAlarmEnable(timer);
}


void IRAM_ATTR FirabseCheckGPS() {
  portENTER_CRITICAL_ISR(&timerMux);
  interruptCounter++;
  portEXIT_CRITICAL_ISR(&timerMux);
}

void IRAM_ATTR ChecarBotonInterrupcion() {
  interrupcionBoton++;
  Serial.println("Interrupcion por boton : " + interrupcionBoton);
  if (interrupcionBoton > 1) {
    interrupcionBoton = 0;
  }
}




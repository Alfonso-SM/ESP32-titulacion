#include "GPS_Telcel.h"
#include "States.h"
#include "NFC.h"


/**********************************************************************************************/
GPS_Telcel telcel;
States state;
NFC nfcLib;

void IRAM_ATTR ChecarBotonInterrupcion(void);




void setup()
{
  state.pines();
  nfcLib.init();
  telcel.setConnection();
  attachInterrupt(digitalPinToInterrupt(Button), ChecarBotonInterrupcion, RISING);
}

void loop()
{
  if(nfcLib.READ_NFC()){
    state.OpenDoors();
    interrupcionBoton = 0;
  }
  telcel.ReadFromFirebase();
  telcel.CheckGPS();

}

void IRAM_ATTR ChecarBotonInterrupcion() {
  interrupcionBoton++;
  Serial.println("Interrupcion por boton : " + interrupcionBoton);
  state.OpenDoors();
}




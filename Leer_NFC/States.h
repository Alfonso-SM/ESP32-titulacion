// States.h

#ifndef States_h
#define States_h

#include "Arduino.h" // Include the Arduino core library
#include <ESP32_Servo.h>

/* numero de pines*/
#define ChipNoAuth 100
#define ChipAuth 10
#define ChipPin  22
#define Lock 21
#define Unlock 0
#define Button 32
#define Acc1 14
#define Acc2 15
#define Run 13
#define Starter 12

class States {
public:
  States();
  void OpenDoors();
  void CloseDoors();
  bool ChecarBoton();
  void TurnOnCar();
  void pines();
};

#endif
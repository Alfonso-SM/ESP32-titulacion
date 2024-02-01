// States.cpp

#include "States.h"

bool DoorOpen = false;
bool RunStatus = false;
bool StartStatus = false;
bool StatusOpen = false;
bool autorizado = false;

Servo Chip;


States::States() {
  // Constructor implementation
}


void States::OpenDoors() {
  if(DoorOpen){
    CloseDoors();
    return;
  }
  Chip.write(ChipAuth);
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

void States::CloseDoors() {
 // timerWrite(timer, 0);
 // timerAlarmEnable(timer);
  Chip.write(ChipNoAuth);
  digitalWrite(Lock, LOW);
  delay(200);
  digitalWrite(Lock, HIGH);
  digitalWrite(Run, HIGH);
  digitalWrite( Acc1, HIGH);
  digitalWrite( Acc2, HIGH);
  DoorOpen = false;
  StatusOpen = false;
  RunStatus = false;
  StartStatus = false;
  autorizado = false;
}

bool States::ChecarBoton() {
 // interrupcionBoton = 0;
  if (autorizado ) {
    if (!StartStatus) {
      if (RunStatus) {
        digitalWrite(Acc1, HIGH);
        digitalWrite(Starter, LOW);
        delay(500);
        digitalWrite(Starter, HIGH);
        digitalWrite(Acc1, LOW);
        StartStatus = true;
        delay(1500);
        return true;
        //timerAlarmDisable(timer);  // deshabilitamos la interrupcion para leer firebase ya que se puede llegar a tardar mas de un minuto esta funcion
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
      digitalWrite(Acc2, HIGH); //////////////////////////////////////////////////
    }
  }
  return false;
}

void States::TurnOnCar() {
  autorizado = true;
  Chip.write(ChipAuth);
  delay(200);
  digitalWrite(Run, LOW);
  digitalWrite(Acc2, LOW);
  RunStatus = true;
  delay(1500);
  digitalWrite(Acc1, HIGH);
  digitalWrite(Starter, LOW);
  delay(600);
  digitalWrite(Starter, HIGH);
  digitalWrite(Acc1, LOW);
  StartStatus = true;
}



void States::pines() {
  pinMode( Starter , OUTPUT);
  pinMode( Run , OUTPUT);
  pinMode(Acc1 , OUTPUT);
  pinMode(Acc2 , OUTPUT);
  pinMode(Unlock , OUTPUT);
  pinMode( Lock , OUTPUT);
  pinMode(Button, INPUT);
  digitalWrite(Run , HIGH);
  digitalWrite(Starter, HIGH);
  digitalWrite(Acc1 , HIGH);
  digitalWrite(Acc2 , HIGH);
  digitalWrite(Unlock , HIGH);
  digitalWrite(Lock , HIGH);
  Chip.attach(ChipPin);
  Chip.write(ChipNoAuth);
}

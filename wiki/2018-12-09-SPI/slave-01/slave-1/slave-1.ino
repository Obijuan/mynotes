// Biblioteca de manejo de SPI
#include <SPI.h>

// El pin 10 se usa para SS
const int slaveSelectPin = 10;

void setup() {   
  pinMode(slaveSelectPin, OUTPUT);
  digitalWrite(slaveSelectPin, HIGH); 

  //-- Inicializar SPI
  SPI.begin();
}

void loop() {

  for (;;) {
    //-- Activar SS
    digitalWrite(slaveSelectPin, LOW);

    //-- Transferir 2 bytes para probar
    //-- En la icezum alhambra solo se verán en los LEDs
    //-- el último
    SPI.transfer(0xf0);
    SPI.transfer(0x0F);

    //-- Desactivar SS
    digitalWrite(slaveSelectPin, HIGH);
    delay(500);  
  }
  

}

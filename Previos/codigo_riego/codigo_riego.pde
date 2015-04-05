*/ Modulo de Riego - Unidad SAC
*/ 



const int sensor1Pin = 2;     // Sensor (S1) de nivel de la bandeja.
const int sensor2Pin = 3;     // Sensor (S2) de nivel del deposito de agua.
const int relePin = 7;        // Rele que activa la bomba de agua.
const int ledRedPin =  4;     // Led rojo (deposito de agua vacio).
const int ledGreenPin =  5;   // Led verde (inicio del ciclo de riego).
const int ledBluePin =  6;    // Led azul (regando).


int sensor1State = 0;         // variables de lectura de los sensores de nivel.
int sensor2State = 0;

void setup() {
  
  pinMode(sensor1Pin, INPUT);    // Sensor (S1) se configura como pin digital de entrada.
  pinMode(sensor2Pin, INPUT);    // Sensor (S2) se configura como pin digital de entrada.
  pinMode(relePin, OUTPUT);      // Rele se configura como pin digital de salida.
  pinMode(ledRedPin, OUTPUT);    // Led rojo se configura como pin digital de salida.
  pinMode(ledGreenPin, OUTPUT);  // Led amarillo se configura como pin digital de salida.
  pinMode(ledBluePin, OUTPUT);   // Led verde se configura como pin digital de salida.
  


}

void loop() {
  
  
  sensor1State = digitalRead(sensor1Pin);   // Lectura del estado del sensor (S1).
  sensor2State = digitalRead(sensor2Pin);   // Lectura del estado del sensor (S2).
 

  if (sensor1State == HIGH) {             // Iniciar ciclo de riego

     digitalWrite(ledGreenPin, HIGH);     // Enciende el led verde. 
     delay (10000);                       // Espera 10 secs.
     digitalWrite(ledGreenPin, LOW);      // Apaga el led verde.
     digitalWrite(relePin, HIGH);         // Enciende la bomba de agua.
     digitalWrite(ledBluePin, HIGH);      // Enciende el led azul. 
     delay (15000);                       // Espera 15 secs.
     digitalWrite(relePin, LOW);          // Apaga la bomba de agua.
     digitalWrite(ledBluePin, LOW);       // Apaga el led azul.
     digitalWrite(ledGreenPin, HIGH);     // Enciende el led verde. 
     delay (15000);                       // Espera 15 secs.
  }
    
  
  if (sensor1State == LOW) {               // Finalizar ciclo de riego

  digitalWrite(relePin, LOW);              // Apaga la bomba de agua.
  digitalWrite(ledBluePin, LOW);           // Apaga el led azul.
  digitalWrite(ledGreenPin, LOW);          // Apaga el led verde.



  if (sensor2State == HIGH) {              // Sin agua en el deposito. Finalizar ciclo de riego.

     digitalWrite(ledRojoPin, HIGH);       // Enciende el led rojo 
     digitalWrite(relePin, LOW);           // Apaga la bomba de agua. 
     digitalWrite(ledBluePin, LOW);        // Apaga el led azul.


}


}





Condiciones: 

*Tras 20min regando stop-ciclo.
*efecto no rebote en los sensores.


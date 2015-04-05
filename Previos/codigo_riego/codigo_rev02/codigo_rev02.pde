


const int sensor1Pin = 2;                        // Sensor (S1) de nivel de la bandeja.
const int sensor2Pin = 3;                        // Sensor (S2) de nivel del deposito de agua.
const int relePin = 7;                           // Rele que activa la bomba de agua.
const int ledRedPin =  4;                        // Led rojo (deposito de agua vacio).
const int ledGreenPin =  5;                      // Led verde (inicio del ciclo de riego).
const int ledBluePin =  6;                       // Led azul (regando).
const long tMaxRiego = 20*60*1000;               // Tiempo máximo regando (20 minutos)
const int tError = 48*60*60*1000;                // Tiempo que el sistema se queda parado cuando hay error (48 horas)
const int tDescanso = 4*60*60*1000;              // Tiempo de "descanso" entre que la bandeja se queda sin agua y se empieza a regar (4 horas)
long tIniRiego = 0;                              // Momento en el que se empieza a regar
long tRegando = 0;                               // Tiempo que se lleva regando
int sensor1State = 0;                            // Variables de lectura de los sensores de nivel.
int sensor2State = 0;


void setup() {
 
  pinMode(sensor1Pin, INPUT);             // Sensor (S1) se configura como pin digital de entrada.
  pinMode(sensor2Pin, INPUT);             // Sensor (S2) se configura como pin digital de entrada.
  pinMode(relePin, OUTPUT);               // Rele se configura como pin digital de salida.
  pinMode(ledRedPin, OUTPUT);             // Led rojo se configura como pin digital de salida.
  pinMode(ledGreenPin, OUTPUT);           // Led amarillo se configura como pin digital de salida.
  pinMode(ledBluePin, OUTPUT);            // Led verde se configura como pin digital de salida.
 
}

void loop() {
    sensor1State = digitalRead(sensor1Pin);    // Lectura del estado del sensor (S1).
    sensor2State = digitalRead(sensor2Pin);    // Lectura del estado del sensor (S2).

    if (sensor1State == LOW) {                 // Hay agua en la bandeja: Finalizar ciclo de riego
        digitalWrite(relePin, LOW);            // Apaga la bomba de agua.
        digitalWrite(ledBluePin, LOW);         // Apaga el led azul.
        digitalWrite(ledGreenPin, LOW);        // Apaga el led verde.
        tRegando = 0;                          // resetear tiempo de riego
        delay (5000);                          // Espera 5 secs.       
    }
    else if (sensor2State == HIGH) {           // No hay agua en el deposito: Finalizar ciclo de riego.
        digitalWrite(ledRedPin, HIGH);         // Enciende el led rojo
        digitalWrite(relePin, LOW);            // Apaga la bomba de agua.
        digitalWrite(ledBluePin, LOW);         // Apaga el led azul.
        tRegando = 0;                          // Resetear tiempo de riego
        delay (5000);                          // Espera 5 secs.       
    } else {                                   // Hay agua en el deposito y no en la bandeja -> se puede regar     
        if (tRegando > tMaxRiego) {            // Se ha superado el tiempo máximo de riego -> ERROR
            digitalWrite(ledRedPin, HIGH);     // Enciende el led rojo
            delay(tError);                                 // dejarlo encendido
        } else {           
            if (tRegando == 0) {                           // Se empieza a regar
                digitalWrite(ledGreenPin, HIGH);           // Enciende el led verde.
                delay (tDescanso);                         // Espera a que se seque la tierra
                digitalWrite(ledGreenPin, LOW);            // Apaga el led verde.
                tIniRiego = millis();                      // Guardar tº de inicio del riego   
            }
             // Continuar el ciclo de riego
            digitalWrite(relePin, HIGH);                   // Enciende la bomba de agua.
            digitalWrite(ledBluePin, HIGH);                // Enciende el led azul.
            delay (15000);                                 // Espera 15 secs.
            digitalWrite(relePin, LOW);                    // Apaga la bomba de agua.
            digitalWrite(ledBluePin, LOW);                 // Apaga el led azul.
            digitalWrite(ledGreenPin, HIGH);               // Enciende el led verde.
            delay (15000);                                 // Espera 15 secs.
            digitalWrite(ledGreenPin, LOW);                // Apaga el led verde.
            tRegando = millis() - tIniRiego;               // actualiza tiempo que lleva regando
        }
    }
}



/*
  This is a test sketch for the Adafruit assembled Motor Shield for Arduino v2
  It won't work with v1.x motor shields! Only for the v2's with built in PWM
  control

  For use with the Adafruit Motor Shield v2
  ---->	http://www.adafruit.com/products/1438
*/

#include <Wire.h>
#include <Adafruit_MotorShield.h>
#include "utility/Adafruit_MS_PWMServoDriver.h"

// Create the motor shield object with the default I2C address
volatile Adafruit_MotorShield AFMS = Adafruit_MotorShield();
// Or, create it with a different I2C address (say for stacking)
// Adafruit_MotorShield AFMS = Adafruit_MotorShield(0x61);

// Select which 'port' M1, M2, M3 or M4. In this case, M1
volatile Adafruit_DCMotor *myMotorL = AFMS.getMotor(1);
// You can also make another motor on port M2
Adafruit_DCMotor *myMotorR = AFMS.getMotor(2);


//----------------------------
// VARIABLES GLOABLES
//----------------------------

// Variables encoder, finales de carrera y leds lado izquierdo
#define pinFinalCarreraMec1L 4
#define pinFinalCarreraMag1L 5
#define pinFinalCarreraMag2L 6
#define pinFinalCarreraMec2L 7
#define ledFinalMagnetico1L 8
#define ledFinalMagnetico2L 9
#define ledFinalMecanico1L 10
#define ledFinalMecanico2L 11
#define ledReadyL 12
#define ledReadyR 13

// Variables encoder, finales de carrera y leds lado derecho
#define pinFinalCarreraMec1R 28
#define pinFinalCarreraMag1R 24
#define pinFinalCarreraMag2R 26
#define pinFinalCarreraMec2R 22
#define ledFinalMagnetico1R 30
#define ledFinalMagnetico2R 32
#define ledFinalMecanico1R 34
#define ledFinalMecanico2R 36

//#define ledFinalMecanico 6
typedef enum {FINAL, EN_PROCESO} valor_t;
typedef enum {NO_AUTORIZADO_R, NO_AUTORIZADO_L,AUTORIZADO} carrera_t;
const int CAL = 2;
const int CBL = 3;
const int CAR = 18;
const int CBR = 19;
const int del = 100; // Valor predefinido del delay, para no trabajar en la zona del transitorio en el pin donde se activa la interrupción.
int counter = 0; // Debemos po  nerla como volatile para que sea consultada antes de ser usada porque puede estar siendo modificada por la interrupción
int umbral = 10; // establecemos un umbral para la llegada a la posicion determinada. Ojo con esta variable, ya que asegura que nuestro sistema no oscile

// Variables necesarias para el posicionamiento
volatile long posicionActualL = 0; // Posicion del motor1
volatile long posicionActualR = 0;
volatile long posicionRef = 0;    // Posicion de referencia a la que queremos ir.
volatile long posicionRefL = 0;
volatile long posicionRefR = 0;
// Variables para la comunicación serie
String inputString = "";         // a string to hold incoming data
boolean stringComplete = false;  // whether the string is complete

int estadoLed  = 0;

int STEP = 1; // Nos controla el número de pasos que realizamos de golpe


volatile boolean moviendonosL = false;
volatile boolean moviendonosR = false;
const int finCarrera =  12;   // PIN para el posible control fin de carrera
long velocidad = 50;

int valueAnterior = 0; // Sirve para saber en que estado estaba el final de carrera en la llamada anterior

//----------------------------
// INICIZALICION
//----------------------------

void setup() {

  //Serial.begin(115200);           // set up Serial library at 9600 bps
  Serial.begin(9600);
//  Serial.println("Adafruit Motorshield v2 - DC Motor test!");

  /* PINES DE ENTRADA*/
  // Pines encoders
  pinMode(CAL, INPUT);
  pinMode(CBL, INPUT);

  pinMode(CAR, INPUT);
  pinMode(CBR, INPUT);
  
  // Pines finales de carrera mecanicos
  pinMode(pinFinalCarreraMec1L, INPUT);
  pinMode(pinFinalCarreraMec2L, INPUT);
  pinMode(pinFinalCarreraMec1R, INPUT);
  pinMode(pinFinalCarreraMec2R, INPUT);

  // Pines finales de carrera magnméticos
  pinMode(pinFinalCarreraMag1L, INPUT);
  pinMode(pinFinalCarreraMag1L, INPUT);
  pinMode(pinFinalCarreraMag1R, INPUT);
  pinMode(pinFinalCarreraMag1R, INPUT);
  pinMode(13, OUTPUT);
  
  /*PINES DE SALIDA*/
  // Pines de salida para la visualización en leds de las activaciones
  pinMode(ledFinalMagnetico1L, OUTPUT);
  pinMode(ledFinalMagnetico2L, OUTPUT);
  pinMode(ledFinalMecanico1L, OUTPUT);
  pinMode(ledFinalMecanico2L, OUTPUT);
  pinMode(ledReadyL, OUTPUT);
  
  pinMode(ledFinalMagnetico1R, OUTPUT);
  pinMode(ledFinalMagnetico2R, OUTPUT);
  pinMode(ledFinalMecanico1R, OUTPUT);
  pinMode(ledFinalMecanico2R, OUTPUT);
  AFMS.begin();  // create with the default frequency 1.6KHz
  //AFMS.bGOegin(1000);  // OR with a different frequency, say 1KHz
  //Inicialización encoder
  counter = 0;
  attachInterrupt(0, inter_cal, CHANGE);
  attachInterrupt(1, inter_cbl, CHANGE);
  attachInterrupt(4, inter_car, CHANGE);
  attachInterrupt(5, inter_cbr, CHANGE);
  velocidad = 150;
  // Set the speed to start, from 0 (off) to 255 (max speed)
  myMotorL->setSpeed(velocidad);
  myMotorL->run(FORWARD);
  myMotorR->setSpeed(velocidad);
  myMotorR->run(FORWARD);
  // turn on motor
  myMotorL->run(RELEASE);
  myMotorR->run(RELEASE);
  apagar_ready(ledReadyR);
  apagar_ready(ledReadyL);

  // Valor por defecto de la velocidad de los motores
  
}


//----------------------------
// BUCLE SIN FIN
//----------------------------
void loop()

{
    //Serial.print(posicionActualR);
    //Serial.println(posicionActualL);
    carrera_t estado = AUTORIZADO; 
    carrera_t estadoR = AUTORIZADO;
 
    estado = lecturaEstado();
    estadoR = lecturaEstadoR();

  // Serial.println(velocidad);
   // Serial.println(estado);
  
  // Configuración en función del error entre la posición actual y la siguiente
//-------------------------------- Lado izquierdo----------------------------------
 
//Me muevo a la izquierda
  
  if ((posicionRefL > posicionActualL) && (estado != NO_AUTORIZADO_L) &&(abs(posicionActualL-posicionRefL) > umbral)) // Incrementamos posición angular
    {
    apagar_ready(ledReadyL);
    myMotorL->setSpeed(velocidad);
    myMotorL->run(FORWARD);
    

    //posicionActual = posicionActual + STEP;
    moviendonosL = true;

    // Serial.println(posicionRef);
//Me muevo a la derecha
    } else if ((posicionRefL < posicionActualL) && (estado != NO_AUTORIZADO_R) && (abs(posicionActualL-posicionRefL) > umbral))
    {
    myMotorL->setSpeed(velocidad);
    myMotorL->run(BACKWARD);
    apagar_ready(ledReadyL);

    //posicionAtual = posicionActual - STEP;
    moviendonosL = true;

    // Serial.println(posicionActual);

    }else // No hacemos nada Estamos en la posición que queremos
    {
    myMotorL -> run(RELEASE);
    moviendonosL = false;
    encender_ready(ledReadyL);
    }

  // Esto esta aquí por si hay que desactivar el motor después de llegar a la posición que toca
  if ((abs(posicionRefL-posicionActualL) < umbral) && (moviendonosL))
  {
    // RS.ENABLE(0);
    myMotorL -> run(RELEASE);
    moviendonosL = false;
    //Serial.println("Estoy");
  }
  leerFinalesCarrera();


//-------------------------------- Lado derecho ---------------------------------------
// Me muevo a la izquierda 
  if ((posicionRefR > posicionActualR) && (estadoR != NO_AUTORIZADO_L) && (abs(posicionActualR-posicionRefR) > umbral)) {
    apagar_ready(ledReadyR);
    myMotorR->setSpeed(velocidad);
    myMotorR->run(BACKWARD);
    

    //posicionActual = posicionActual + STEP;
    moviendonosR = true;

    // Serial.println(posicionActual);

// Me muevo a la derecha
  } else if ((posicionRefR < posicionActualR) && (estadoR != NO_AUTORIZADO_R) && (abs(posicionActualR-posicionRefR) > umbral))
  {
    myMotorR->setSpeed(velocidad);
    myMotorR->run(FORWARD);
    apagar_ready(ledReadyR);

    //posicionAtual = posicionActual - STEP;
    moviendonosR = true;

    // Serial.println(posicionActual);

  } else // No hacemos nada Estamos en la posición que queremos
  {
    myMotorR -> run(RELEASE);
    moviendonosR = false;
    encender_ready(ledReadyR);
  }

  // Esto esta aquí por si hay que desactivar el motor después de llegar a la posición que toca
  if ((abs(posicionRefR-posicionActualR)< umbral) && (moviendonosR))
  {
    // RS.ENABLE(0);
    myMotorR -> run(RELEASE);
    moviendonosR = false;
   // Serial.println("Estoy");
    
  }

  // int value = digitalRead(pinFinalCarreraMag1);
  // digitalWrite(6, value);
  // Serial.println(value);
  // delay(100); int valueMag1 = digitalRead(pinFinalCarreraMag1);
  leerFinalesCarrera();

}

//------------------------------------------------------
// ATENCIÓN A LA INTERRUPCIÓN RS232
//------------------------------------------------------

void serialEvent()
{
  // Serial.println("Hola");
  while (Serial.available())
  {
    // get the new byte:
    char inChar = (char)Serial.read();
    // add it to the inputString:
    inputString += inChar;
    // if the incoming character is a newline, set a flag
    // so the main loop can do something about it:
    if (inChar == '\n') {
      //  stringComplete = true;

      // Serial.print("Secuencia capturada:");
      // Serial.println(inputString);

      // Quitamos dos caracteres porque mandamos dos comandos de finalización. Habrá que ajustarlo en función de lo que mandemos
      int lastStringLength = inputString.length();
      String comandoLeido  = inputString.substring(0, lastStringLength - 2);
      int colonPosition = inputString.indexOf(':');  // Miramos si hay un :

      if (comandoLeido == "POSITION?")
      {
        int errorL = abs(posicionActualL - posicionRefL);
        int errorR = abs(posicionActualR - posicionRefR);
        int errorMax = max(errorL,errorR); // Calculamos el máximo 


        if(errorMax == errorL){
          errorMax <= umbral ? Serial.println(posicionRefL) : Serial.println(posicionActualL); 
        }else{
          errorMax <= umbral ? Serial.println(posicionRefR) : Serial.println(posicionActualR);
        }

        Serial.println(errorMax);
      }
      if (comandoLeido == "POSITION1?")
      {
        int errorL = abs(posicionActualL - posicionRefL);
        errorL <= umbral ? Serial.println(posicionRefL) : Serial.println(posicionActualL); 
      }
      if (comandoLeido == "POSITION2?")
      {
        int errorR = abs(posicionActualR - posicionRefR);
        errorR <= umbral ? Serial.println(posicionRefR) : Serial.println(posicionActualR);
      }
      if (comandoLeido == "VEL?")
      {
        // Serial.println("HOLA");
        Serial.println(velocidad);
      }
      else if (comandoLeido == "*IDN?")
      {
        Serial.println("ADAFRUIT - 200stepper - 2 motores");
      }

      else if (comandoLeido == "GOTORESET") // Resetea los dos motores al mismo tiempo
      {
        apagar_ready(ledReadyL);
        apagar_ready(ledReadyR);      
         while ((detectarFinalCarrera_mecanico(pinFinalCarreraMec1R) == EN_PROCESO)) {
          myMotorR ->setSpeed(255);
          myMotorR ->run(BACKWARD);
          if (detectarFinalCarrera_magnetico(pinFinalCarreraMag1R) == FINAL) {
           // iniciar(pinFinalCarreraMag1R);
            goto cansinoR;
          }
        }
       
        myMotorR -> run(FORWARD);
        while ((detectarFinalCarrera_magnetico(pinFinalCarreraMag1R) == EN_PROCESO)) {
          myMotorR->setSpeed(255);
          myMotorR->run(FORWARD);
        }
        
       // iniciar(pinFinalCarreraMag1R);
cansinoR: myMotorR -> run(RELEASE);
        moviendonosR = false;
       // Serial.println("Posicion de Reset alcanzada");
        posicionRefR = 0;
        posicionActualR = 0;

        while ((detectarFinalCarrera_mecanico(pinFinalCarreraMec1L) == EN_PROCESO)) {
          myMotorL->setSpeed(255);
          myMotorL->run(FORWARD);
          if (detectarFinalCarrera_magnetico(pinFinalCarreraMag1L) == FINAL) {
            //iniciar(pinFinalCarreraMag1L);
            goto cansino;
          }
        }
        myMotorL -> run(RELEASE);
        while ((detectarFinalCarrera_magnetico(pinFinalCarreraMag1L) == EN_PROCESO)) {
          myMotorL->setSpeed(255);
          myMotorL->run(BACKWARD);
        }
       // iniciar(pinFinalCarreraMag1L);
cansino: myMotorL -> run(RELEASE);
        moviendonosL = false;
        Serial.println("Posicion de Reset alcanzada");
        posicionRefL = 0;
        posicionActualL = 0;
   
      }
       else if (comandoLeido == "GOTORESET2")
      {
        gotoresetR();
      }
      else if (comandoLeido == "GOTORESET1")
      {
        gotoresetL();
      }
      else if (comandoLeido == "RESETVALUES")
      {
        posicionRefR = 0;
        posicionActualR = 0;
         posicionRefL = 0;
        posicionActualL = 0;
      }
        else if (comandoLeido == "RESETVALUES1")
      {
        posicionRefR = 0;
        posicionActualR = 0;
      }
       else if (comandoLeido == "RESETVALUES2")
      {
        posicionRefL = 0;
        posicionActualL = 0;
      }
      else if (colonPosition != -1)
      {
        comandoLeido = "";
        comandoLeido = inputString.substring(0, colonPosition);

        if (comandoLeido == "POSITION") // Cumple el comando
        {
          lastStringLength = inputString.length();
          String valor = inputString.substring(colonPosition + 1, lastStringLength);

          posicionRefR = valor.toInt(); // Actualizamos la posicion a la que tenemos que ir 
          posicionRefL = valor.toInt();
        }
        else if(comandoLeido == "POSITION1")
        {
          lastStringLength = inputString.length();
          String valor = inputString.substring(colonPosition + 1, lastStringLength);

          posicionRefL = valor.toInt(); // Actualizamos la posicion a la que tenemos que ir    
        }
        else if(comandoLeido == "POSITION2")
        {
          lastStringLength = inputString.length();
          String valor = inputString.substring(colonPosition + 1, lastStringLength);

          posicionRefR = valor.toInt(); // Actualizamos la posicion a la que tenemos que ir    
        }
        else if (comandoLeido == "VEL")
        {
          lastStringLength = inputString.length();
          String valorVelocidad = inputString.substring(colonPosition + 1, lastStringLength);
          velocidad = valorVelocidad.toInt();
          myMotorL->setSpeed(velocidad);  // Actualizamos la velocidad
          myMotorR->setSpeed(velocidad);
        }
      }

      inputString = ""; // Clear string for new input

    } // END de IF inCHAR

  } // END the WHILE

} // END the la function



// Función para atender a la interrupción en A en el encoder-------------------------------

void inter_cal() {
  int B = digitalRead(CBL);
  debounce(del);
  int A = digitalRead(CAL);

  if (A == HIGH) {
    if (B == LOW) {
      posicionActualL = posicionActualL + 1;
    } else
      posicionActualL = posicionActualL - 1;
  } else {
    if (B == HIGH) {
      posicionActualL = posicionActualL + 1;
    } else
      posicionActualL = posicionActualL - 1;
  }  
  leerFinalesCarrera();
}

void inter_car() {
  int B = digitalRead(CBR);
  debounce(del);
  int A = digitalRead(CAR);

  if (A == HIGH) {
    if (B == LOW) {
      posicionActualR = posicionActualR - 1;
    } else
      posicionActualR = posicionActualR + 1;
  } else {
    if (B == HIGH) {
      posicionActualR = posicionActualR - 1;
    } else
      posicionActualR = posicionActualR + 1;
  }
  leerFinalesCarrera();
}

// Función para atender a la interrupción en B del encoder-------------------------------

void inter_cbl() {
  int A = digitalRead(CAL);
  debounce(del);
  int B = digitalRead(CBL);

  if (B == HIGH) {
    if (A == LOW) {
      posicionActualL = posicionActualL - 1;
    } else
      posicionActualL = posicionActualL + 1;
  } else {
    if (A == HIGH) {
      posicionActualL = posicionActualL - 1;
    } else
      posicionActualL = posicionActualL + 1;
  }
  //Serial.println(counter);

  // Para comprobar que funcionan bien los finales de carrera magneticos
  leerFinalesCarrera();
  /*if (value!=valueAnterior)
    { valueAnterior = value;
    digitalWrite(6, value);
    Serial.println(value);
    }
  */

}

void inter_cbr() {
  int A = digitalRead(CAR);
  debounce(del);
  int B = digitalRead(CBR);

  if (B == HIGH) {
    if (A == LOW) {
      posicionActualR = posicionActualR + 1;
    } else
      posicionActualR = posicionActualR - 1;
  } else {
    if (A == HIGH) {
      posicionActualR = posicionActualR + 1;
    } else
      posicionActualR = posicionActualR - 1;
  }
  //Serial.println(counter);
    
  // Para comprobar que funcionan bien los finales de carrera magneticos
  leerFinalesCarrera();
  /*if (value!=valueAnterior)
    { valueAnterior = value;
    digitalWrite(6, value);
    Serial.println(value);
    }
  */

}
// Función para crear retardo dentro de las interrupciones (Arduino no permite usar delay ni millis)-------------

void debounce(int delay) {
  for (int i = 0; i < delay; i++) {
    /* can't use delay in the ISR so need to waste some time
       perfoming operations, this uses roughly 0.1ms on uno  */
    i = i + 0.0 + 0.0 - 0.0 + 3.0 - 3.0;
  }
}

// Función que detecta cuando se activa el final de carrera determinado a la entrada por flanco

valor_t detectarFinalCarrera_mecanico(int pin) {

  int actual = digitalRead(pin);
  valor_t res;

  if ((actual == LOW)) {
    res = FINAL;
  }
  else {
    res = EN_PROCESO;
  }
  return (res);

}
// Función que detecta cuando se activa un determinado final de carrera magnetico en flanco de bajada, ya que es activo a nivel bajo

valor_t detectarFinalCarrera_magnetico(int pin) {
  int actual = digitalRead(pin);
  valor_t res;

  if ((actual == LOW)) {
    res = FINAL;
  } else {
    res = EN_PROCESO;
  }
  return (res);
}

void iniciar(int pin) {

  switch (pin) {
    case pinFinalCarreraMag1L:
      //      Serial.println("Holi");
      while (detectarFinalCarrera_magnetico(pinFinalCarreraMag1L) == FINAL) {
        //        Serial.println("Holi");
        myMotorL->setSpeed(255);
        myMotorL->run(FORWARD);
        delay(500);
      }
      myMotorL->run(RELEASE);
      while (detectarFinalCarrera_magnetico(pinFinalCarreraMag1L) == EN_PROCESO) {
        myMotorL->setSpeed(255);
        myMotorL->run(BACKWARD);
      }
      myMotorL->run(RELEASE);
      Serial.println("Listo");
   case pinFinalCarreraMag1R:
      while (detectarFinalCarrera_magnetico(pinFinalCarreraMag1R) == FINAL) {  
        myMotorR->setSpeed(255);
        myMotorR->run(FORWARD);
        delay(500);
      }
      myMotorR->run(RELEASE);
      while (detectarFinalCarrera_magnetico(pinFinalCarreraMag1R) == EN_PROCESO) {
        myMotorR->setSpeed(255);
        myMotorR->run(BACKWARD);
      }
      myMotorR->run(RELEASE);
     // Serial.println("Listo"); 
      break;
    default:
      //Serial.println("Ups");
      break;
  }
}

void leerFinalesCarrera() {
  int valueMag1L = digitalRead(pinFinalCarreraMag1L);
  int valueMag2L = digitalRead(pinFinalCarreraMag2L);
  int valueMec1L = digitalRead(pinFinalCarreraMec1L);
  int valueMec2L = digitalRead(pinFinalCarreraMec2L);

  int valueMag1R = digitalRead(pinFinalCarreraMag1R);
  int valueMag2R = digitalRead(pinFinalCarreraMag2R);
  int valueMec1R = digitalRead(pinFinalCarreraMec1R);
  int valueMec2R = digitalRead(pinFinalCarreraMec2R);
  
  digitalWrite(ledFinalMagnetico1L, valueMag1L);
  digitalWrite(ledFinalMagnetico2L,  valueMag2L);
  digitalWrite(ledFinalMecanico1L, valueMec1L);
  digitalWrite(ledFinalMecanico2L, valueMec2L);

  digitalWrite(ledFinalMagnetico1R, valueMag1R);
  digitalWrite(ledFinalMagnetico2R,  valueMag2R);
  digitalWrite(ledFinalMecanico1R, valueMec1R);
  digitalWrite(ledFinalMecanico2R, valueMec2R);
}

void encender_ready(int led){
  digitalWrite(led,HIGH);
}

void apagar_ready(int led){
  digitalWrite(led, LOW);
}

carrera_t lecturaEstado(){
  carrera_t estado;
  if((detectarFinalCarrera_mecanico(pinFinalCarreraMec1L)== FINAL) || (detectarFinalCarrera_magnetico(pinFinalCarreraMag1L)== FINAL)){
    estado = NO_AUTORIZADO_L;
    return(estado);
  }else if((detectarFinalCarrera_mecanico(pinFinalCarreraMec2L)== FINAL) || (detectarFinalCarrera_magnetico(pinFinalCarreraMag2L)== FINAL)){
    estado = NO_AUTORIZADO_R;
    return(estado);  
  }else{
    estado = AUTORIZADO;
    return(estado);
  }
   return(estado);
}

carrera_t lecturaEstadoR(){
  carrera_t estado;
  if((detectarFinalCarrera_mecanico(pinFinalCarreraMec1R)== FINAL) || (detectarFinalCarrera_magnetico(pinFinalCarreraMag1R)== FINAL)){
    estado = NO_AUTORIZADO_L;
    return(estado);
  }else if((detectarFinalCarrera_mecanico(pinFinalCarreraMec2R)== FINAL) || (detectarFinalCarrera_magnetico(pinFinalCarreraMag2R)== FINAL)){
    estado = NO_AUTORIZADO_R;
    return(estado);  
  }else{
    estado = AUTORIZADO;
    return(estado);
  }
   return(estado);
}


void gotoresetR(){
        myMotorL -> run(RELEASE);
        moviendonosL = false;
        apagar_ready(ledReadyR);      
         while ((detectarFinalCarrera_mecanico(pinFinalCarreraMec1R) == EN_PROCESO)) {
          if (detectarFinalCarrera_magnetico(pinFinalCarreraMag1R) == FINAL) {
            //iniciar(pinFinalCarreraMag1R);
            goto finR;
          }
          myMotorR ->setSpeed(255);
          myMotorR ->run(BACKWARD);
        } 
        myMotorR -> run(FORWARD);
        while ((detectarFinalCarrera_magnetico(pinFinalCarreraMag1R) == EN_PROCESO)) {
          myMotorR->setSpeed(255);
          myMotorR->run(FORWARD);
        }
        
        //iniciar(pinFinalCarreraMag1R);
        finR: myMotorR -> run(RELEASE);
        moviendonosR = false;
        //Serial.println("Posicion de Reset alcanzada");
        posicionRefR = 0;
        posicionActualR = 0;
}

void gotoresetL(){
         myMotorR -> run(RELEASE);
         moviendonosR = false;
         apagar_ready(ledReadyL);      
         myMotorL ->run(FORWARD);
         while ((detectarFinalCarrera_mecanico(pinFinalCarreraMec1L) == EN_PROCESO)) {       
          if (detectarFinalCarrera_magnetico(pinFinalCarreraMag1L) == FINAL) {
            //iniciar(pinFinalCarreraMag1L);
            goto finL;
          }
        } 
        myMotorL -> run(FORWARD);
        while ((detectarFinalCarrera_magnetico(pinFinalCarreraMag1L) == EN_PROCESO)) {
          myMotorL->setSpeed(255);
        }
        
        //iniciar(pinFinalCarreraMag1L);
        finL: myMotorL -> run(RELEASE);
        moviendonosL = false;
        //Serial.println("Posicion de Reset alcanzada");
        posicionRefL = 0;
        posicionActualL = 0;   
}




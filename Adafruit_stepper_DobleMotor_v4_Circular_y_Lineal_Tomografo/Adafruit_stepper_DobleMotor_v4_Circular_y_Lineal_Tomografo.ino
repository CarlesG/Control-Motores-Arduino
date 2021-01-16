/* 
This is a test sketch for the Adafruit assembled Motor Shield for Arduino v2
It won't work with v1.x motor shields! Only for the v2's with built in PWM
control

For use with the Adafruit Motor Shield v2 
---->	http://www.adafruit.com/products/1438
*/
 

#include <Wire.h>

//----------------------------
// DECLARACIONES DE LOS MOTORES
//----------------------------

#include <Adafruit_MotorShield.h>
#include "utility/Adafruit_MS_PWMServoDriver.h"

// Create the motor shield object with the default I2C address
Adafruit_MotorShield AFMS = Adafruit_MotorShield(); 
// Or, create it with a different I2C address (say for stacking)
// Adafruit_MotorShield AFMS = Adafruit_MotorShield(0x61); 

// Connect a stepper motor with 200 steps per revolution (1.8 degree)
// to motor port #2 (M3 and M4)
Adafruit_StepperMotor *myMotor1 = AFMS.getStepper(200, 1); // Motor conectado a M1 y M2
Adafruit_StepperMotor *myMotor2 = AFMS.getStepper(200, 2); // Motor conectado a M3 y M4

#define SENTIDOMOTOR1 -1   // Si es positivo, incrementaremos positivamente en FORDWARD, si es negativo incrementaremos positivameent en BACKWARD
#define SENTIDOMOTOR2 -1  // Si es positivo, incrementaremos positivamente en FORDWARD, si es negativo incrementaremos positivameent en BACKWARD

//----------------------------
// DECLARACIONES DE LA MÁQUINA DE ESTADOS
//----------------------------

// Máquina de estados: Estados posibles
#define AUTOMATICO1Y2 0
#define AUTOMATICO1   1
#define AUTOMATICO2   2
#define PARADO        3 // No hace caso a nadie
#define MANUAL1Y2     4
#define MANUAL1       5
#define MANUAL2       6

volatile int maquinaEstado  = AUTOMATICO1Y2;

// Pausas para esperar a funcionar entre cambio de estados. Cada vez que se cambia de estado, se espera un tiempo para empezar a aplicar los cambios
#define       TIEMPO  1
#define       MINI_ESPERA  100 // Los milisegundos que tenemos que esperar hasta al
int           NESPERAS = round(TIEMPO*1000/MINI_ESPERA);
volatile int  nEsperas = 0; // Esta variabla la reseteará la atención de interrupción al cambio de estado



//----------------------------
// VARIABLES GLOBALES
//----------------------------

// Variables necesarias para el posicionamiento
volatile long posicionActual1 = 0; // Posicion del motor
volatile long posicionActual2 = 0; // Posicion del motor
volatile long posicionActual  = 0; // Posicion para devolver en caso que pregunten
volatile long posicionRef     = 0;    // Posicion de referencia a la que queremos ir. 
volatile long posicionRef1    = 0;    // Posicion de referencia a la que queremos ir. 
volatile long posicionRef2    = 0;    // Posicion de referencia a la que queremos ir. 
// volatile long offset1         = 1700;  // Offset respecto a la posición de reset del motor 1
// volatile long offset2         = 1900;  // Offset respecto a la posición de reset del motor 2
volatile long offset1         = 0;  // Offset respecto a la posición de reset del motor 1
volatile long offset2         = 0;  // Offset respecto a la posición de reset del motor 2


// Variables para la comunicación serie
String inputString = "";         // a string to hold incoming data
boolean stringComplete = false;  // whether the string is complete

int estadoLed  = 0;

int STEP = 1; // Nos controla el número de pasos que realizamos de golpe


boolean moviendonos = false; 
boolean primeraIteracion = true; // Es para controlar la primera iteración 

// const int finCarrera =  12;   // PIN para el posible control fin de carrera
long velocidad = 100; // Velocidad rpm

// Variables de control
float error1 = 0; // Error 1 entre la comanda y el sensor 1
float error2 = 0; // Error 2 entre la comanda y el sensor 1
const float errorUmbral = 0; // Esto es para determinar si ya hemos llegado a donde toca y no tenemos que movermos más.

// Variables del filtro

// Variables de final de carrera
#define finCarrera1 5 // Pin del final de carrera del motor 1. Activo a 5V
#define finCarrera2 13 // Pin del final de carrera del motor 2. Activo a 5V

//----------------------------
// INICIZALICION
//----------------------------

void setup() {
  Serial.begin(9600);           // set up Serial library at 9600 bps
  // Serial.println("Stepper test!");

    // --------------------------
    // Inicialización de los motores
    // --------------------------
    AFMS.begin();  // create with the default frequency 1.6KHz
    //AFMS.begin(1000);  // OR with a different frequency, say 1KHz

    // Inicializamos el motor a 10rpm
    myMotor1->setSpeed(velocidad);  // 10 rpm   
    myMotor2->setSpeed(velocidad);  // 10 rpm   

    // reserve 200 bytes for the inputString:
    inputString.reserve(200);   

    // Definimos los pines de final de carrera
    pinMode(finCarrera1, INPUT);
    pinMode(finCarrera2, INPUT);    
    
    
}


//----------------------------
// BUCLE SIN FIN
//----------------------------
void loop() 
{
  // Pasos en este loop
  // Paso 1 - Medir los angulos de los acelerómetros
  // Paso 2 - Si es la primera iteración, hacer que el angulo leido sea la comanda
  // Paso 3 - Calcular los error1 y error 2 entre la comanda y los angulos leidos
  // Paso 4 - Si error1>umbral movernos un paso
  // Paso 5 - Si error2>umbral movernos un paso
  

  //  Serial.println("Single coil steps");
  //  myMotor2->step(100, FORWARD, SINGLE); 
  // myMotor2->step(100, BACKWARD, SINGLE); 
  //
  //  Serial.println("Double coil steps");
  //  myMotor2->step(100, FORWARD, DOUBLE); 
  //  myMotor2->step(100, BACKWARD, DOUBLE);
  //  
  //  Serial.println("Interleave coil steps");
  //  myMotor2->step(100, FORWARD, INTERLEAVE); 
  //  myMotor2->step(100, BACKWARD, INTERLEAVE); 
  //  
  //  Serial.println("Microstep steps");
  //  myMotor2->step(50, FORWARD, MICROSTEP); 
  //  myMotor2->step(50, BACKWARD, MICROSTEP);
  
    // --------------------------
    // PASO 1 - Leemos los ángulos
    // --------------------------

    


    
    // ----------------------------------------
    // Primera iteración
    // ----------------------------------------
    // Nos sirve para controlar que es la primera vez que se ejecuta el bucle e inicializar la posición de referencia a la posición del sensor
    if (primeraIteracion==true)
    {
        primeraIteracion = false;        
        posicionRef = 0;
        posicionActual1 = posicionRef;
        posicionActual2 = posicionRef;
        
    }
    
    
      


    
    // Calculamos los errores
    error1 = posicionRef1 - posicionActual1 ;
    error2 = posicionRef2 - posicionActual2 ;

    // ----------------------------------------
    // Control Motor 1
    // ----------------------------------------
    // Si el error supera el umbral y el angulo leido está en el margen de control, nos movemos    
    
    // while (abs(error1)>errorUmbral)  // De esta forma gestionamos que se mueva primero un motor y después el otro
    if (abs(error1)>errorUmbral)     
    {      
      
        moviendonos = true;
        // if (posicionActual1 > posicionRef1) // El angulo leido 
        if (sign(error1*SENTIDOMOTOR1)==1)
        { 
           // if (digitalRead(finCarrera1)==0)
           // {  myMotor1 -> step(STEP, FORWARD  , SINGLE);  }
            
          myMotor1 -> step(STEP, FORWARD  , SINGLE); 
          
            posicionActual1 = posicionActual1 + SENTIDOMOTOR1*STEP;   
           
           // myMotor1->step(STEP, FORWARD  , DOUBLE);
           // myMotor1->step(STEP, FORWARD  , INTERLEAVE);           
          // myMotor1 -> step(STEP,FORWARD  , MICROSTEP);                     
                            
        } else 
        {
           // Solo lo movemos si el final de carrera no está a 1
            if (digitalRead(finCarrera1)==0)
            {  myMotor1->step(STEP, BACKWARD , SINGLE); }
            
            // myMotor1->step(STEP, BACKWARD , SINGLE);
              
            posicionActual1 = posicionActual1 - SENTIDOMOTOR1*STEP;                         
        }
      
        // Actualizamos el error al final        
        error1 =   posicionRef1 - posicionActual1;               
    
    // } // END de WHILE
    // moviendonos = false;
    // myMotor1-> release(); // Liberamos el motor   
    } else  // Hemos llegado a la posición o estamos fuera de margen
    {
       moviendonos = false;
       myMotor1-> release(); // Liberamos el motor 
    }

//      if (digitalRead(finCarrera1)==1)
//{ Serial.println("Final de carrera pulsado"); Serial.print("\t");
// }

            


  // ----------------------------------------
    // Control Motor 2
    // ----------------------------------------
    // Si el error supera el umbral y el angulo leido está en el margen de control, nos movemos    
    // while (abs(error2)>errorUmbral)  // De esta forma gestionamos que se mueva primero un motor y después el otro
    if (abs(error2)>errorUmbral)     
    {      
      
        moviendonos = true;        
        if (sign(error2*SENTIDOMOTOR2)==1)
        { 
            myMotor2->step(STEP, FORWARD, SINGLE);             
            posicionActual2 = posicionActual2 + SENTIDOMOTOR2*STEP;   
                      
        } else 
        {
            myMotor2->step(STEP, BACKWARD , SINGLE);           
            posicionActual2 = posicionActual2 - SENTIDOMOTOR2*STEP;           
        }

        // Actualizamos el error al final     
        error2 =  posicionRef2 - posicionActual2;

    // } // END de WHILE  
    //    moviendonos = false;
    // myMotor2-> release(); // Liberamos el motor    
    } else  // Hemos llegado a la posición o estamos fuera de margen
    {
       moviendonos = false;
       myMotor2-> release(); // Liberamos el motor     
    }

    
//  if (moviendonos==true)
//  {
//    Serial.print(posicionRef); Serial.print("\t");
//    Serial.print(posicionActual1); Serial.print("\t");
//    Serial.print(posicionActual2); Serial.print("\t");
//    Serial.println("\t");
//  }

}   

    


//------------------------------------------------------
// Llamada a la función para ir a la posición de reset.
//------------------------------------------------------

void gotoReset() {

    int valor;
    valor = digitalRead(finCarrera1);
    // Serial.print("Valor final de carrera 1"); Serial.print("\t");
    // Serial.println(valor); 
   
    valor = digitalRead(finCarrera2);
    // Serial.print("Valor final de carrera 2"); Serial.print("\t");
    // Serial.println(valor); 
    
//
//    // Lo sacamos de la posición de reset en caso de que estuviera
//    if (valor==1)
//    {
//      // Lo desplazamos un poco para sacarlo de la posible posición de reset
//      RS.CW_CCW(CCW); 
//      RS.CLK(1000);
//      posicionActual = posicionActual + 1000;
//    }
//
//    // Volvemos a leer el fin de carrera
//    valor = digitalRead(finCarrera);
//
//    // Buscamos la posición de reset
//     while (valor==0)
//     {   
//        valor = digitalRead(finCarrera);
//        RS.CW_CCW(CW); 
//        RS.CLK(STEP); 
//        posicionActual = posicionActual - STEP ;
//       
//     }
//
//        posicionRef = posicionActual;        
//        RS.ENABLE(0);
//             
} // END de la función gotoReset

void gotoReset1() {

    int valor;
    valor = digitalRead(finCarrera1);
//    Serial.print("Valor final de carrera 1"); Serial.print("\t");
//    Serial.println(valor); 
       
    // Lo sacamos de la posición de reset en caso de que estuviera
    if (valor==1)
    {
      // Lo desplazamos un poco para sacarlo de la posible posición de reset      
      // myMotor1->step(1000, BACKWARD  , SINGLE);                                                
      // posicionActual1 = posicionActual1 + 1000;
      
      while (valor==1) // Con esto lo que conseguimos es optimizar el proceso de reset.
      {   
          valor = digitalRead(finCarrera1);    
          myMotor1->step(STEP, FORWARD, SINGLE); 
          posicionActual1 = posicionActual1 + SENTIDOMOTOR1*STEP;  
          
      }      
    
    }

    // Volvemos a leer el fin de carrera
    valor = digitalRead(finCarrera1);

    // Buscamos la posición de reset
     while (valor==0)
     {   
        valor = digitalRead(finCarrera1);
    
        myMotor1->step(STEP, BACKWARD, SINGLE); 
        posicionActual1 = posicionActual1 - SENTIDOMOTOR1*STEP;  
                
     }

    // Ahora desplazamos el offset del motor
    if (sign(SENTIDOMOTOR1)*sign(offset1)==1)
        myMotor1 -> step(abs(offset1), FORWARD , SINGLE); 
    else
        myMotor1 -> step(abs(offset1), BACKWARD   , SINGLE); 
  
    posicionActual1 = posicionActual1 + SENTIDOMOTOR1*offset1;

    posicionRef1 = posicionActual1;        
    myMotor1 -> release(); // Liberamos el motor
             
} // END de la función gotoReset

void gotoReset2() {

    int valor;
       
    valor = digitalRead(finCarrera2);
    // Serial.print("Valor final de carrera 2"); Serial.print("\t");
    // Serial.println(valor); 
    

//    // Lo sacamos de la posición de reset en caso de que estuviera
    if (valor==1)
    {
      // Lo desplazamos un poco para sacarlo de la posible posición de reset
      
      // myMotor2->step(1000, BACKWARD  , SINGLE);                                                
      // posicionActual2 = posicionActual2 + 1000;

      while (valor==1) // Con esto lo que conseguimos es optimizar el proceso de reset.
      {   
          valor = digitalRead(finCarrera2);    
          myMotor2->step(STEP, BACKWARD, SINGLE); 
          posicionActual2 = posicionActual2 - SENTIDOMOTOR2*STEP;  
      }
         
    }
  //  BACKWARD
  //  FORWARD

    // Volvemos a leer el fin de carrera
    valor = digitalRead(finCarrera2);

    // Buscamos la posición de reset
     while (valor==0)
     {   
        valor = digitalRead(finCarrera2);
    
        myMotor2->step(STEP, FORWARD, SINGLE); 
        posicionActual2 = posicionActual2 + SENTIDOMOTOR2*STEP;  
                
     }


    if (sign(SENTIDOMOTOR2)*sign(offset2)==1)
          myMotor2 -> step(abs(offset2), FORWARD , SINGLE); 
      else
        myMotor2 -> step(abs(offset2), BACKWARD   , SINGLE); 


    // Ahora desplazamos el offset del motor
    // myMotor2->step(offset2, BACKWARD, SINGLE);
    // posicionActual2 = posicionActual2 - SENTIDOMOTOR2*offset2;
    posicionActual2 = posicionActual2 + SENTIDOMOTOR2*offset2;
    
    posicionRef2 = posicionActual2;        
    myMotor2 -> release(); // Liberamos el motor
             
} // END de la función gotoReset


//------------------------------------------------------
// FUNCIÓN PARA CAMBIAR DE MODO
//------------------------------------------------------
// Esta función ahora mismo no se utilizará porque nos apañamaremos con comandos. Se tendrá que activar cuando hagamos interrupciones por comando
void updateCambioEstado() 
{
    maquinaEstado++;
    if ((maquinaEstado>3) || (maquinaEstado<0)) // Si la maquina de estado se sale de margen, reajustamos
    { 
        maquinaEstado = AUTOMATICO1Y2;
    }
    
    // Nos ubicamos en la primera fila y columna del display
    // lcd.setCursor(0, 0);
    
    
    switch (maquinaEstado)
    {
      
      case AUTOMATICO1Y2:
           // lcd.print("AUTO  1-2");
           Serial.println("Automatico ->  1 y 2");   
           break;

      case AUTOMATICO1:
           // lcd.print("AUTO  1  ");       
           Serial.println("Automatico -> 1 ");   
           break;
      
      case AUTOMATICO2:
           // lcd.print("AUTO  2  ");          
           Serial.println("Automatico -> 2 ");   
           break;
           
      case PARADO:
          // lcd.print("STOPPED");
           break;
      
      case MANUAL1Y2:
           // lcd.print("MAN 1-2"); 
            Serial.println("Manual 1 y 2 - Modo no contemplado todavía");   
           break;
      
      case MANUAL1:
           // lcd.print("MAN 1  ");    
           Serial.println("Manual 1 - Modo no contemplado todavía");   
           break;
      
      case MANUAL2:
           // lcd.print("MAN 2  ");   
           Serial.println("Manual 2 - Modo no contemplado todavía");   
           break;

      
           
    }
    
    

    // encoderPushButton ++;
    // Serial.print("Botón apretado ");

    //  Serial.print("Encoder PushButton -------------- : ");
    // Serial.println(encoderPushButton);
     
     delay(50);    // Debouncing 
     nEsperas = 0; //  Lo reseteamos a 0 de forma que en el bucle principal no hará nada hasta que no superemos el tiempo total de espera
     

}



//------------------------------------------------------
// ATENCIÓN A LA INTERRUPCIÓN RS232
//------------------------------------------------------

void serialEvent() 
{
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
          String comandoLeido  = inputString.substring(0,lastStringLength-2);    
          int colonPosition = inputString.indexOf(':');  // Miramos si hay un :          
                
          if (comandoLeido == "POSITION?")
          {              
              if (abs(posicionActual1-posicionRef1)>abs(posicionActual2-posicionRef2))
                Serial.println(posicionActual1);              
              else
                Serial.println(posicionActual2);
              
                
          }
          else if (comandoLeido == "POSITION1?")
          {
            Serial.println(posicionActual1);              
          }
          else if (comandoLeido == "POSITION2?")
          {
            Serial.println(posicionActual2);              
          }
          else if (comandoLeido == "VEL?")
          {              
            // Serial.println("HOLA");
              Serial.println(velocidad);
          }
          else if (comandoLeido == "*IDN?")
          {
              // Serial.println("ADAFRUIT - 200stepper - Doble motor - v1");
              Serial.println("ADAFRUIT - 200stepper - 2 motores");
          }
          else if (comandoLeido == "OFFSET1?")
          {
            Serial.println(offset1);              
          }
          else if (comandoLeido == "OFFSET2?")
          {
            Serial.println(offset2);              
          }
          else if (comandoLeido == "GOTORESET")
          {
              // Llamada a la función para ir a la posición de reset
              // gotoReset();           
               gotoReset1();           
               gotoReset2();           
          } 
          else if (comandoLeido == "GOTORESET1")
          {
              // Llamada a la función para ir a la posición de reset
               gotoReset1();           
          } 
          else if (comandoLeido == "GOTORESET2")
          {
              // Llamada a la función para ir a la posición de reset
               gotoReset2();           
          } 

          else if (comandoLeido == "CAMBIOMODO")
          {
              // updateCambioEstado();
                 Serial.println("Deshabilitado el cambio de modo");
          }
          else if (comandoLeido == "RESETVALUES") 
          {
              posicionRef = 0;
              posicionRef1 = 0;
              posicionRef2 = 0;
              posicionActual1 = 0;
              posicionActual2 = 0;
          }
          else if (comandoLeido == "RESETVALUES1") 
          {
              posicionRef1 = 0;
              posicionActual1 = 0;              
          }
          else if (comandoLeido == "RESETVALUES2") 
          {
              posicionRef2 = 0;              
              posicionActual2 = 0;
          }          
          else if (colonPosition!=-1)
          {
              comandoLeido = "";
              comandoLeido = inputString.substring(0,colonPosition);

              lastStringLength = inputString.length();           
              String valor = inputString.substring(colonPosition+1,lastStringLength);
                  
              if (comandoLeido == "POSITION") // Cumple el comando
              {
                  // Esto manda a los dos motores la posición leida
           
                  posicionRef = valor.toInt(); // Actualizamos la posicion a la que tenemos que ir               
                  posicionRef1 = posicionRef;
                  posicionRef2 = posicionRef;    
              }
              else if (comandoLeido == "POSITION1") // Esto manda al motor1 la posición leida
              {
                  posicionRef = valor.toInt(); // Actualizamos la posicion a la que tenemos que ir               
                  posicionRef1 = posicionRef;                  
               }
              else if (comandoLeido == "POSITION2")     // Esto manda al motor2 la posición leida")
              {
                  posicionRef = valor.toInt(); // Actualizamos la posicion a la que tenemos que ir               
                  posicionRef2 = posicionRef;
              } 
              else if (comandoLeido == "OFFSET1")     // Esto manda al motor2 la posición leida")
              {
                 offset1 = valor.toInt(); // Actualizamos la posicion a la que tenemos que ir                                 
              } 
              else if (comandoLeido == "OFFSET2")     // Esto manda al motor2 la posición leida")
              {
                 offset2 = valor.toInt(); // Actualizamos la posicion a la que tenemos que ir                                 
              } 
              else if (comandoLeido == "VEL")
              {
                  // lastStringLength = inputString.length();           
                  // String valorVelocidad = inputString.substring(colonPosition+1,lastStringLength);
                  velocidad = valor.toInt();
                  Serial.println(velocidad);
                  myMotor1->setSpeed(velocidad);  // Actualizamos la velocidad
                  myMotor2->setSpeed(velocidad);  // Actualizamos la velocidad
                  
              }     
         }
                    
          inputString = ""; // Clear string for new input
  
      } // END de IF inCHAR
   
  } // END the WHILE

} // END the la function


//------------------------------------------------------
// FUNCIONES TRIGONOMETRICAS
//------------------------------------------------------

// -------------------------------------------
float asin(float c){
float out;
out= ((c+(c*c*c)/6+(3*c*c*c*c*c)/40+(5*c*c*c*c*c*c*c)/112+
(35*c*c*c*c*c*c*c*c*c)/1152 +(c*c*c*c*c*c*c*c*c*c*c*0.022)+
(c*c*c*c*c*c*c*c*c*c*c*c*c*.0173)+(c*c*c*c*c*c*c*c*c*c*c*c*c*c*c*.0139)+
(c*c*c*c*c*c*c*c*c*c*c*c*c*c*c*c*c*0.0115)+(c*c*c*c*c*c*c*c*c*c*c*c*c*c*c*c*c*c*c*0.01)
));

//asin
if(c>=.96 && c<.97){out=1.287+(3.82*(c-.96)); }
if(c>=.97 && c<.98){out=(1.325+4.5*(c-.97));} // arcsin
if(c>=.98 && c<.99){out=(1.37+6*(c-.98));}
if(c>=.99 && c<=1){out=(1.43+14*(c-.99));}
return out;}


// -------------------------------------------
float acos(float c)
{
    float out;
    out=asin(sqrt(1-c*c));
    return out;
}

// -------------------------------------------
float atan(float c)
{float out;
out=asin(c/(sqrt(1+c*c)));
return out;}


// -------------------------------------------
// Función signo
float sign(float x)
{
  if (x/abs(x)>=0)
    return 1;
  else
    return -1;
 
 }


/*
Basic operations. Demo for one board.  Aquí ponemos también los comandos para ser interpretados por el puerto serie
*/
 
// ROHM IC Motor Drivers
// please define one of the following motor driver ICs
// based on the EVK model version you are using.
// Please also visit
// http://www.rohm.com/web/eu/arduino-stepper-motor-shield
// to download the latest EVK manual version.
// Please also visit
// http://www.rohm.com/web/eu/search/parametric/-/search/Stepping%20Motor
// to download the latest datasheet version of the motor driver IC you are evaluating.

//#define BD63710AEFV  
//#define BD63715AEFV 
#define BD63720AEFV   // Seleccionamos el que toca
//#define BD6425EFV 
//#define BD63843EFV 
//#define BD63847EFV
 
#include <ROHM_Steppers.h> // Incluimos la librería

// initialize the ROHM Steppers libs for MASTER board 
// and NOT ACTIVE(OPEN), RESET, FULL_STEP, CW 
// half clock perid 1000 us by default


// Variables necesarias para el posicionamiento
long posicionActual = 0; // Posicion del motor
long posicionRef = 0;    // Posicion de referencia a la que queremos ir. 

// Variables para la comunicación serie
String inputString = "";         // a string to hold incoming data
boolean stringComplete = false;  // whether the string is complete

int estadoLed  = 0;

int STEP = 1;

boolean moviendonos; 

 const int finCarrera =  12;  

ROHM_Stepper RS(ONE);

//------------------------------------------------------
// Inicialización
//------------------------------------------------------

void setup()
{
    // Inicializamos el puerto serie
    // initialize serial:
    Serial.begin(9600);
  
    // reserve 200 bytes for the inputString:
    inputString.reserve(200);
  
	  
	  // change pin state
     RS.ENABLE(ACTIVE);  // OPEN->ACTIVE   
     RS.PS    (ACTIVE);  // RESET->ACTIVE
    delayMicroseconds(40); // wait 40us after PS->High as recommended in data sheet
	  
	  // ready to clock	

    //FULL_STEP CCW
     RS.MODE  (FULL_STEP);   
     RS.CW_CCW(CCW); 
     // RS.CLK(100);

    moviendonos = false;

// Lo deshabilitamos para que no consuma
    RS.ENABLE(0);

    // pinMode(13, OUTPUT);

    pinMode(finCarrera, INPUT);

}

//------------------------------------------------------
// BUCLE LOOP
//------------------------------------------------------

void loop() 
{

  // multiple clocks using CLK(N)

  
  if (posicionRef>posicionActual) // Incrementamos posición angular
  {
      
       RS.ENABLE(ACTIVE);  // OPEN->ACTIVE      

     
      RS.CW_CCW(CW); 
      RS.CLK(STEP);
      posicionActual = posicionActual + STEP;
    
       // Serial.println(posicionActual);
      // delay(100);
      
    moviendonos = true;
  } else if (posicionRef<posicionActual)
  {
      RS.ENABLE(ACTIVE);  // OPEN->ACTIVE   
     
     
      
      RS.CW_CCW(CCW); 
      RS.CLK(STEP); 
      posicionActual = posicionActual - STEP;
       // Serial.println(posicionActual);
       // delay(100);
       
      moviendonos = true;
 
  } else // No hacemos nada Estamos en la posición que queremos
  {
      
            
  }


  // NO PODEMOS GASTAR EL PIN 13 PORQUE CREO QUE LO GASTA LA LIBRERÍA DEL STEPPER MOTOR
  int moduloPosicionActual = abs(posicionActual);

//  digitalWrite(13, LOW);
//  delay(500);
//  digitalWrite(13, HIGH);
//  delay(500);

//  if ((moduloPosicionActual % 1000) == 0)
//  {  
//    
//    Serial.println(moduloPosicionActual);
//    Serial.println(estadoLed);
//    if (estadoLed == 1)
//    {   
//        Serial.println("APagamos");
//        digitalWrite(13, LOW); 
//        estadoLed = 0;
//    }
//    else if (estadoLed==0)
//    {
//        Serial.println("Encendemos");
//        digitalWrite(13, HIGH);    
//        estadoLed = 1 ;
//    }
//  }
  
//  delay(1000);
//  digitalWrite(13, LOW); // lO DEJAMOS APAGADO
  
  
  if ((posicionRef==posicionActual) && (moviendonos)) 
  {
    RS.ENABLE(0);
    
//    Serial.println("--------------");
//    Serial.print("Posicion alcanzada: ");
//    Serial.println(posicionActual);
//    Serial.println("--------------");
    moviendonos = false;
  } 
 
}
//------------------------------------------------------
// Llamada a la función para ir a la posición de reset.
//------------------------------------------------------

void gotoReset() {

      RS.ENABLE(ACTIVE);  // OPEN->ACTIVE            

    int valor;
    unsigned long previousMillis = millis();
        
    valor = digitalRead(finCarrera);

    posicionActual = 100;

  
    // Lo sacamos de la posición de reset en caso de que estuviera
    if (valor==1)
    {
      // Lo desplazamos un poco para sacarlo de la posible posición de reset
      RS.CW_CCW(CCW); 
      RS.CLK(1000);
      // posicionActual = posicionActual + 1000;
    }
    else
    {
      // Volvemos a leer el fin de carrera
      valor = digitalRead(finCarrera);
      // Buscamos la posición de reset
      while (valor==0)
      {
          valor = digitalRead(finCarrera);
          RS.CW_CCW(CCW); 
          RS.CLK(STEP); 
         
          if ((millis()-previousMillis)>500)
          {
              Serial.println(0); 
              previousMillis = millis();
          }         
       
        } // END de WHILE

        // Lo volvemos a sacar de la posición
        RS.CW_CCW(CCW);
        RS.CLK(1000);        
    
    } // END de ELSE
 
    // Volvemos a leer el valor
    valor = digitalRead(finCarrera);
    
    // Ahora lo volvemos a buscar en sentido antihorario
    while (valor==0)
    {
          valor = digitalRead(finCarrera);
          RS.CW_CCW(CW); 
          RS.CLK(STEP); 
         
          if ((millis()-previousMillis)>500)
          {
              Serial.println(0); 
              previousMillis = millis();
          }         
       
      } // END del segundo WHILE
  
  
    // Reseteamos todos los contadores a 0
    posicionRef = 0;
    posicionActual = 0;
        
    RS.ENABLE(0);
   
      Serial.println(1); // Indicamos que hemos llegado a reset
      
     
} // END de la función gotoReset



//------------------------------------------------------
// ATENCIÓN A LA INTERRUPCIÓN RS232
//------------------------------------------------------

void serialEvent() {
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read();
    // add it to the inputString:
    inputString += inChar;
    // if the incoming character is a newline, set a flag
    // so the main loop can do something about it:
     if (inChar == '\n') {
          //  stringComplete = true;
    
//           Serial.print("Secuencia capturada:");
//           Serial.println(inputString);


          // Quitamos dos caracteres porque mandamos dos comandos de finalización. Habrá que ajustarlo en función de lo que mandemos
          int lastStringLength = inputString.length();           
          String comandoLeido  = inputString.substring(0,lastStringLength-2);    
          int colonPosition = inputString.indexOf(':');  // Miramos si hay un :

    
          // int endPosition = inputString.indexOf('\n');
          // String comandoLeido = inputString.substring(0,endPosition);

           // Serial.print("Posicion Final: ");
           // Serial.println(lastStringLength);

          
          // Serial.print("Comando leido y eliminado el final de línea antes de buscar el COLON: ");
          // Serial.println(comandoLeido);

       
          if (comandoLeido == "POSITION?")
          {              
                  Serial.println(posicionActual);
          }
          else if (comandoLeido == "*IDN?")
          {

                Serial.println("DISPOSITIVO1 --");
          }
          else if (comandoLeido == "GOTORESET")
          {
            posicionActual = 100;
            // Llamada a la función para ir a la posición de reset
            gotoReset();
            
          } 
          else if (comandoLeido == "RESETVALUES") 
          {
            posicionRef = 0;
            posicionActual = 0;
          }
          else if (colonPosition!=-1)
          {
              comandoLeido = "";
              comandoLeido = inputString.substring(0,colonPosition);
          
         
              if (comandoLeido == "POSITION") // Cumple el comando
              {

                  lastStringLength = inputString.length();           
                  String valor = inputString.substring(colonPosition+1,lastStringLength);

             
                   posicionRef = valor.toInt(); // Actualizamos la posicion a la que tenemos que ir
     
           
              }
          
           }
          
          

          inputString = ""; // Clear string for new input
  
      } // END de IF inCHAR
   
  } // END the WHILE

} // END the la function



  

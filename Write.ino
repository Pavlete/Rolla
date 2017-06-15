#include <Wire.h>

enum CameraState {IDLE, SYNCHRONIZING, SYNCHRONIZED, CAPTURING}; 
 
CameraState myState = IDLE;

byte readI2CByte(byte reg)
{
  Wire.beginTransmission(0x21); 
  Wire.write(reg);  
  Wire.endTransmission();
  Wire.requestFrom(0x21, 1);
  while (Wire.available() == 0);  //block till u get something

  return Wire.read();
}

void writeI2CByte(byte reg, byte value)
{
  Wire.begin();
  Wire.beginTransmission(0x21);
  Wire.write(reg);  // software reset
  Wire.write(value);
  Wire.endTransmission();
  delay(500); // wait for reset to complete
}
 
void setup()
{
  //Pin de VSYNC
  pinMode(7, INPUT);
  
  //Botton disparador
  pinMode(5, INPUT);

  //Led
  pinMode(14, OUTPUT);
  digitalWrite(14, true);

  //PCLK
  pinMode(22, INPUT);
  
  //HREF
  pinMode(23, INPUT);

  //Datos
  DDRC = 0x00;

  //Configurar Timer  
  pinMode(11, OUTPUT);
  TCCR1A = ( (1 << COM1A0)); 
  TCCR1B = ((1 << WGM12) | (1 << CS10));
  TIMSK1 = 0;    
  OCR1A = 0;    
  
  Wire.begin();

  delay(1000); 
  Serial.begin(2000000);
  while (!Serial);             // Leonardo: wait for serial monitor
  Serial.println("\nI2C Scanner");  

  writeI2CByte(0x12, 0x80);

  myState = IDLE;

  for(int add = 0; add < 0xAF; add++)
  {
    Serial.print("Reg: 0x");
    Serial.print(add, HEX);
          
    Serial.print(" Val: 0x");
    Serial.print(readI2CByte(add), HEX);
    Serial.println();
  }
}

int mills = 0;
bool buttonStatus = false;
bool nightMode = false;
bool ledOn = false;
bool vsync = false;
bool href = false;
bool pclock = false;

byte linea[1280];
int lines = 0;
int rows = 0;
     
void loop()
{    
    switch(myState)
    {
      case IDLE:
      {
        //Serial.println(myState);
        bool old = buttonStatus;
        buttonStatus = digitalRead(5);

        if (old != buttonStatus && buttonStatus)
        {
          writeI2CByte(0x11, 0x87);
          myState = SYNCHRONIZING;
        }

        if(digitalRead(7) && !ledOn)
        {          
          digitalWrite(14, false);
          ledOn = true;
          
        }
        else if(!digitalRead(7) && ledOn)
        {
          digitalWrite(14, true);
          ledOn = false;
        }
        
      }
      break;
      
      case SYNCHRONIZING:
      {        
          //Serial.println(myState);
          bool valVsync = PINH & 16;
          
          if(valVsync && !vsync)
          {
            vsync = true;        
          }
          else if(!valVsync && vsync)
          {
            vsync = false;
            myState = SYNCHRONIZED;
            lines = 0;
          }
      }
      break;
      case SYNCHRONIZED:
      {
        //Serial.println(myState);
        bool valHref = PINA & 2;
          
        if(valHref && !href)
        {
          href = true;       
          rows = 0;
          lines++;  
          myState = CAPTURING;
        }        
        
        bool valVsync = PINH & 16;
        if(valVsync)
        {
          myState = IDLE;
          vsync = true;                    
          Serial.println(lines);
        }
      }
      break;
      case CAPTURING:
      {
        //Serial.println(myState);
        bool valPclock = PINA & 1;
          
        if(valPclock && !pclock)
        {
          pclock = true;        
          rows++;
        }
        else if(!valPclock && pclock)
        {
          pclock = false;
        }
                
        bool valHref = PINA & 2;
        if(!valHref && href)
        {
          myState = SYNCHRONIZED;
          href = false;
          //Serial.println(lines);
        }
      }
      break; 
    }
}

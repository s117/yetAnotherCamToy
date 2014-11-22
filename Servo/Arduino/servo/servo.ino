#include <Servo.h> 

#define MIDDLE_AXISX 90
#define MIDDLE_AXISY 90
Servo axisX;
Servo axisY;

int posX = MIDDLE_AXISX;
int posY = MIDDLE_AXISY;

unsigned char okX[3] = {'S','X','\0'};
unsigned char okY[3] = {'S','Y','\0'};
unsigned char failX[3] = {'F','X','\0'};
unsigned char failY[3] = {'F','Y','\0'};
unsigned char failU[3] = {'F','U','\0'};

void setup() 
{
  Serial.begin(115200);
  axisX.attach(9);
  axisY.attach(10);
  axisX.write(MIDDLE_AXISX);
  axisY.write(MIDDLE_AXISY);
  Serial.println("Control system ready...");
} 

void loop() 
{
  unsigned char axispart;
  unsigned char valpart;
  
  while(Serial.available() == 0);
  axispart = Serial.read();
  if(axispart == 'I')
    return;

  while(Serial.available() == 0);
  valpart = Serial.read();
  if(axispart == 'I')
    return;

  switch(axispart){
    case 'X':
      if((valpart >= 0)&&(valpart <= 200)){
        okX[2] = valpart;
        posX = valpart;
        Serial.write(okX,3);
      }else{
        Serial.write(failX,3);
      }
      break;
    case 'Y':
      if((valpart >= 0)&&(valpart <= 200)){
        okY[2] = valpart;
        posY = valpart;
        Serial.write(okY,3);
      }else{
        Serial.write(failY,3);
      }
      break;    
    default:
      Serial.write(failU,3);
      Serial.flush();
      break;
  }
  axisX.write(posX);
  axisY.write(posY);
}


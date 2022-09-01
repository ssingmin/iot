// First we include the libraries
#include <SoftwareSerial.h>

SoftwareSerial HC12(2, 3);

const int dry = 500;
const int met = 225;

void setup(void)
{
  // start serial port
  Serial.begin(9600);
  HC12.begin(9600);
  Serial.println("Dallas Temperature IC Control Library Demo");
}
void loop(void)
{
  float temp1 = temperature(0);
  float temp2 = temperature(1);
  float temp3 = temperature(2);
  float humid = humidity(3);

  Serial.print("temp1 : " + String(temp1));
  Serial.print(", temp2 : " + String(temp2));
  Serial.print(", temp3 : " + String(temp3));
  Serial.println(", humid : " + String(humid));

  String msg = String(temp1) + "," + String(temp2) + "," + String(temp3) +"," + String(humid) + "/";
  Serial.println("test");
  if (HC12.readStringUntil('/') == "request2")
  {
  
    Serial.println("send data");
    HC12.print(msg);
  }
}

float temperature(int pin_number) {
  int Vo;
  float R1 = 10000;
  float logR2, R2, T, Tc, Tf;
  float c1 = 1.009249522e-03, c2 = 2.378405444e-04, c3 = 2.019202697e-07;

  Vo = analogRead(pin_number);
  R2 = R1 * (1023.0 / (float)Vo - 1.0);
  logR2 = log(R2);
  T = (1.0 / (c1 + c2 * logR2 + c3 * logR2 * logR2 * logR2));
  // Tc = T - 273.15;
  Tc = T - 276.15;
  Tf = (Tc * 9.0) / 5.0 + 32.0;

  return Tc;
}

float humidity(int pin_number) {
  float val = analogRead(pin_number);
  Serial.println(val);
  float humid = map(val, met, dry, 100, 0); 
  
  return humid;
  
}

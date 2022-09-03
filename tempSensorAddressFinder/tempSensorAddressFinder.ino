#include <OneWire.h>
#include <DallasTemperature.h>

OneWire oneWire(5);
DallasTemperature sensors(&oneWire);
DeviceAddress temp;

void setup(void)
{
  Serial.begin(9600);

  sensors.begin();
  sensors.getAddress(temp,0);
}

void loop(void)
{ 
  for(int i=0;i<8;i++){
    Serial.print(" 0x");
    Serial.print(temp[i],HEX);
  }
  Serial.println();
  sensors.requestTemperatures();
  Serial.println(sensors.getTempC(temp));

  delay(5000);
}

#define Seconds 1000
#define Minutes 60000

//PIN
  #define PIN_THERMO 5
  #define PIN_TDS_DATA 9
  #define PIN_Pump 10
  #define PIN_Pump_Switch 16
  // OLED reserves PIN_2(SDA) and PIN_3(SCL)

//Water Thermo
  #include <OneWire.h>
  #include <DallasTemperature.h>
  #include <Math.h>
  OneWire oneWire(PIN_THERMO);
  DallasTemperature sensors(&oneWire);
  DeviceAddress temp0 = { 0x28, 0xFF, 0x64, 0x1E, 0x30, 0x5E, 0x66, 0xA6 }; //water
  DeviceAddress temp1 = { 0x28, 0xFF, 0x64, 0x1E, 0x30, 0x59, 0x11, 0xB3 }; //air

//TDS
  #define VREF 5.0 // analog reference voltage(Volt) of the ADC
  const int sample_count = 36;
  float rawData[sample_count];
  int rawDataIndex = 0;

//Display
  #include <SPI.h>
  #include <Wire.h>
  #include <Adafruit_GFX.h>
  #include <Adafruit_SSD1306.h>
  #define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
  #define SCREEN_WIDTH 128 // OLED display width, in pixels
  #define SCREEN_HEIGHT 64 // OLED display height, in pixels
  #define OLED_RESET -1   //   QT-PY / XIAO
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


//----------------------------------------------------------------------------------------------------
void setup() {
  //Thermo
    pinMode(PIN_THERMO, INPUT);
    sensors.begin();

  //TDS
    pinMode(PIN_TDS_DATA, INPUT);

  //Fertilizer
    pinMode(PIN_Pump, OUTPUT);
    pinMode(PIN_Pump_Switch, INPUT_PULLUP);

  //OLED
    delay(250);
    display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
    display.clearDisplay();

    display.setCursor(0, 30);
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(2);
    display.print("Booting"); display.display();

  //Serial
    Serial.begin(9600);
    delay(500);
    display.print("."); display.display();
    delay(500);
    display.print("."); display.display();
    delay(500);
    display.print("."); display.display();
    delay(500);

  Serial.println("Device Ready");


}


//----------------------------------------------------------------------------------------------------
void loop() {
  static unsigned long lastTime = 0;
  const long interval = 5000;
  unsigned long now = millis();

  if ( now - lastTime > interval ) {
    lastTime = now;

  //Thermo
    sensors.requestTemperatures();

  //TDS
    float voltage_raw = analogRead(PIN_TDS_DATA) * VREF / 1024.0;
    rawData[rawDataIndex++] = voltage_raw;
    if ( sample_count == rawDataIndex ) rawDataIndex = 0;

    float voltage_sum = 0;
    int data_count = 0;
    for (int i = 0; i < sample_count; i++) {
      voltage_sum += rawData[i];
      if ( rawData[i] > 0 ) data_count++;
    }
    float voltage = voltage_sum / data_count;

    float temperature = sensors.getTempCByIndex(0);
    float compensationCoefficient = 1.0 + 0.02 * (temperature-25.0); //temperature compensation formula: fFinalResult(25^C) = fFinalResult(current)/(1.0+0.02*(fTP-25.0));
    float compensationVolatge = voltage / compensationCoefficient; //temperature compensation
    float tdsValue = (133.42 * compensationVolatge * compensationVolatge * compensationVolatge - 255.86 * compensationVolatge * compensationVolatge + 857.39 * compensationVolatge) * 0.5; //convert voltage value to tds value
    float ecValue = tdsValue * 2 / 1000;

  //Fertilization
    const int ecThreshold = 2.0;
    static unsigned long lastTime_f = 0;
    const long interval_f = 30*Minutes;
    unsigned long now_f = millis();

    if (( digitalRead(PIN_Pump_Switch) == LOW ) ||
    (data_count == sample_count
    && ecValue < ecThreshold
    && now_f - lastTime_f > interval_f )) {
      lastTime_f = now_f;
      Serial.println();
      Serial.print("Fertilizer on @ EC:"); Serial.print(ecValue,1);
      fert_pump();
      for (int i = 0; i < sample_count; i++) rawData[i] = 0;
      rawDataIndex = 0;
    }
    if ( digitalRead(PIN_Pump_Switch) == LOW ) Serial.print("w/Switch");

  //Serial Print
    Serial.print("Temp: "); Serial.print("air_"); Serial.print(sensors.getTempCByIndex(1)); Serial.print("°C");
                            Serial.print(" water_"); Serial.print(sensors.getTempCByIndex(0)); Serial.print("°C");
    Serial.print(" | ");
    Serial.print("TDS: "); Serial.print(tdsValue,0); Serial.print("ppm");
    Serial.print(" | ");
    Serial.print("EC: "); Serial.print(ecValue,1); Serial.print("mS/cm");
    Serial.print(" | ");
    Serial.print("Level:"); Serial.print(ratio,0); Serial.print("%/"); Serial.print(water_quant,1); Serial.print("L");
    Serial.print(" | ");
    Serial.print(" DCount:");
    Serial.print(data_count);
    Serial.print("/");
    Serial.print(sample_count);
    Serial.print(" Interval:-");
    Serial.print((interval_f - (now_f - lastTime_f)) / 60000);
    Serial.println("min");

  //OLED Print
    display.setCursor(1, 1);
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);

    display.setTextSize(2); display.print("EC:");
    display.setTextSize(3); display.println(ecValue,1);

    display.setTextSize(1); display.print("TMP");
    display.setTextSize(2); display.print(sensors.getTempCByIndex(1),1); display.print(" "); display.print(sensors.getTempCByIndex(0),1);
    display.setTextSize(1); display.println("\n A:        W:\n");

    display.setTextSize(1); display.print("LV ");
    display.setTextSize(2); display.print(ratio,0); display.print(" "); display.print(water_quant,1);
    display.setTextSize(1); display.print("\n       %         L");

    //display.invertDisplay(true);
    display.display();
    //delay(100);
    //display.invertDisplay(false);

  }
}

void fert_pump() {
  const int PUMPING_DURATION = 10*Seconds;
    digitalWrite(PIN_Pump, HIGH);
    for ( int i = 0; i <= 3; i++) {
      delay(PUMPING_DURATION / 4);
      Serial.print(".");
    }
    delay(PUMPING_DURATION / 4);
    digitalWrite(PIN_Pump, LOW);
    // Serial.println("completed");
}

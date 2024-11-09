#include <Arduino.h>
#include "soc/rtc.h"
#include "HX711.h"

// 获取校准因子
const int LOADCELL_DOUT_PIN = 14; //HX711的DT引脚
const int LOADCELL_SCK_PIN = 12; //HX711的SCK引脚

HX711 scale;

void setup() {
  Serial.begin(115200); //波特率设置为115200
  // rtc_clk_cpu_freq_set(RTC_CPU_FREQ_80M);
  setCpuFrequencyMhz(RTC_CPU_FREQ_80M);

  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
}

void loop() {

  if (scale.is_ready()) {
    scale.set_scale();    
    Serial.println("Tare... remove any weights from the scale.");
    delay(3000);
    scale.tare();
    Serial.println("Tare done...");
    Serial.print("Place a known weight on the scale...");
    delay(3000);
    long reading = scale.get_units(10);
    Serial.print("Result: ");
    Serial.println(reading);
  } 
  else {
    Serial.println("HX711 not found.");
  }
  delay(1000);
}

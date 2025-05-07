/* Milliohmmeter driven by an ATTiny95.  Uses an ADS1115 four-input ADC chip with
   the inputs configured as dual differential sampling.

   A four wire Kelvin probe is used.  Two of the wires drive current through a known
   resistor, two are Kelvin voltage-monitoring probes used to measure the voltage across
   the unknown resistor. 

   dlf 5/7/2025
*/

#include <Arduino.h>
#include <TinyWireM.h>
#include <Adafruit_ADS1X15.h>
#include <LiquidCrystal_I2C.h> 

const float gainMultiplier = 0.1250F;  // For GAIN_ONE setting
const float calConstant = 1.000F;      // Calibrate againist known resistor
const float batteryLowLimit = 3.0F;    // Limit before we display "low battery"

#define BATTERY_MONITOR ADC2
 
// dual-differential ADC chip
Adafruit_ADS1115 ads;

//set the address for the I2C module, and the lines and characters per line.
LiquidCrystal_I2C lcd(0x27, 16, 2); 

// Take multiple readings and report the median
uint8_t numberOfReadings = 10;
 
// Prototypes
float calculateMedian(float* data, size_t size);
long readVcc();

// ##################################
// Setup
// ##################################
void setup() {

  lcd.init();
	lcd.backlight();

  ads.setGain(GAIN_ONE);                 // +/- 4.096V  1 bit = 0.125mV
  ads.setDataRate(RATE_ADS1115_32SPS);   // Slow down the sample rate to minimize noise (32ms for a sample)

  ads.begin();
}
 
 
// ##################################
// Loop
// ##################################
void loop() {
 
  // Array of results so we can pick the median later
  float resVal[numberOfReadings];

  for(uint8_t i=0; i<numberOfReadings; i++) { 

    // Measure the voltage across the current sense resistor
    int16_t adc01 = ads.readADC_Differential_0_1();

    // Measure the voltage across the unknown resistor
    int16_t adc23 = ads.readADC_Differential_2_3();

    // In millivolts
    float vCurRes = adc01 * gainMultiplier;
    float vSense = adc23 * gainMultiplier;

    float current= (vCurRes/10.0) * calConstant;
    resVal[i]= vSense / current;
  }

  // Find the median
  float resistor = calculateMedian(resVal, numberOfReadings);

  char str[8];
  dtostrf(resistor,3,3,str);

  lcd.clear(); 
  lcd.setCursor(0, 0); 

  if(resistor < 0) {
    lcd.print("Res Out Of Range"); 
  } else {
    lcd.print("Res="); 
    lcd.setCursor(4, 0); 
    lcd.print(str); 
    lcd.setCursor(12, 0); 
    lcd.print("ohms"); 
  }

  // Check the battery
  long vcc = readVcc();
  float voltage = vcc / 1000.0;

  if (voltage < batteryLowLimit) {
    lcd.setCursor(0, 1); 
    lcd.print("  Low Battery!");
  }

  delay(1000); 
}

// ##################################
// Functions
// ##################################

// #################
// Read the Vcc pin
// #################
long readVcc() {
  // Set up to read internal 1.1V reference
  ADMUX = _BV(MUX3) | _BV(MUX2); // Select internal 1.1V (0b1110)
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Start conversion
  while (bit_is_set(ADCSRA, ADSC)); // Wait until done

  uint16_t result = ADC;
  long vcc = (long)(1125300L) / result; // 1.1V * 1023 * 1000
  return vcc; // in millivolts
}

// ##############################################################
// Return the median value from an unordered array of floats
// ##############################################################
float calculateMedian(float* data, size_t size) {
  float temp[size];
  memcpy(temp, data, size * sizeof(float));

  // Simple insertion sort
  for (size_t i = 1; i < size; ++i) {
    float key = temp[i];
    int j = i - 1;
    while (j >= 0 && temp[j] > key) {
      temp[j + 1] = temp[j];
      j--;
    }
    temp[j + 1] = key;
  }
  
  // Calculate median
  if (size % 2 == 0) {
    return (temp[size / 2 - 1] + temp[size / 2]) / 2.0;
  } else {
    return temp[size / 2];
  }
}
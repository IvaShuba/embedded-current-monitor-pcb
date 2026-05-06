#include <Wire.h>
#include <Adafruit_MAX31865.h>
#include "rgb_lcd.h"
#include <sam.h>

// Physical Pin Mapping for SAMD21E / Arduino Zero
// Physical 19 -> PA14 (D2)
const int pin_CS  = 10; 
// Physical 17 -> PA12 (D22/MISO)
const int pin_SDI = 11; 
// Physical 20 -> PA15 (D24/MOSI)
const int pin_SDO = 12; 
// Physical 18 -> PA13 (D23/SCK)
const int pin_CLK = 13; 
// Physical 16 -> PA11 (D0)
const int pin_RDY = 0; 
bool LedOn = 0;

// Using Software SPI constructor to match your pin definitions
// Format: (cs, mosi, miso, clk)
Adafruit_MAX31865 thermo = Adafruit_MAX31865(pin_CS, pin_SDI, pin_SDO, pin_CLK);

rgb_lcd lcd;

// Calibration
#define RREF      430.0  
#define RNOMINAL  100.0  
#define Vref      3300

void adc_init() {
    // 1. ADC power
    PM->APBCMASK.reg |= PM_APBCMASK_ADC;

    // 2. (GCLK). on GCLK0 
    GCLK->CLKCTRL.reg = GCLK_CLKCTRL_ID_ADC |     
                        GCLK_CLKCTRL_GEN_GCLK0 |   
                        GCLK_CLKCTRL_CLKEN;        
    while (GCLK->STATUS.bit.SYNCBUSY);             

    // 3. Ref voltage  
    ADC->REFCTRL.reg = ADC_REFCTRL_REFSEL_INTVCC1; 

    // 4. Prescaler setup 
    ADC->CTRLB.reg = ADC_CTRLB_RESSEL_12BIT | 
                     ADC_CTRLB_PRESCALER_DIV64;

    // 5. Config PA02, PA04, PA05, PA06 on ADC (B)
    PORT->Group[0].PINCFG[2].bit.PMUXEN = 1; // PA02
    PORT->Group[0].PINCFG[4].bit.PMUXEN = 1; // PA04
    PORT->Group[0].PINCFG[5].bit.PMUXEN = 1; // PA05
    PORT->Group[0].PINCFG[6].bit.PMUXEN = 1; // PA06

    PORT->Group[0].PMUX[1].bit.PMUXE = 0x1;  // PA02 (PIN 2, PMUX1) -> Function B
    PORT->Group[0].PMUX[2].bit.PMUXE = 0x1;  // PA04 (PIN 4, PMUX2) -> Function B
    PORT->Group[0].PMUX[2].bit.PMUXO = 0x1;  // PA05 (PIN 5, PMUX2) -> Function B
    PORT->Group[0].PMUX[3].bit.PMUXE = 0x1;  // PA06 (PIN 6, PMUX3) -> Function B

    // 6. ADC enable
    ADC->CTRLA.bit.ENABLE = 1;
    while (ADC->STATUS.bit.SYNCBUSY);
}

uint16_t adc_read(uint8_t channel) {
    // PA02=AIN0, PA04=AIN4, PA05=AIN5, PA06=AIN6
    ADC->INPUTCTRL.bit.MUXPOS = channel;
    while (ADC->STATUS.bit.SYNCBUSY);

    // ADC start
    ADC->SWTRIG.bit.START = 1;
    
    // wait till finish
    while (!(ADC->INTFLAG.bit.RESRDY));

    // result
    return (uint16_t)ADC->RESULT.reg;
}

void setup() {
    // Starting SerialUSB for the native USB port
    SerialUSB.begin(115200);
   // Serial.begin(9600);


    lcd.begin(16, 2);
    lcd.setRGB(0, 255, 0); 
    lcd.print("Init MAX31865...");
    
    // Adjusted to 3-wire based on previous discussion, 
    // change to 4-wire if your hardware setup changed
    thermo.begin(MAX31865_4WIRE); 
    pinMode(2, OUTPUT);
    pinMode(pin_RDY, INPUT);
    delay(1000);
    lcd.clear();
    adc_init();
}

void loop() {
    uint16_t rtd = thermo.readRTD();
    float temperature = thermo.temperature(RNOMINAL, RREF);
    uint8_t fault = thermo.readFault();

    // Calculations for JSON output
    // 1. Calculate Voltage across PT100 in millivolts
    // Resistance = (RTD_Code * RREF) / 32768
    float resistance = (float)rtd * RREF / 32768.0;
    // Assuming standard 430 Ohm reference and ~400uA excitation current
    // V = I * R. In millivolts:
    float v_pt100_mv = resistance * 0.4; 

    // 2. Placeholder currents (Amps/milliAmps) 
    // These require external sensors to be real, currently static for the JSON format
    uint16_t raw_total = adc_read(ADC_INPUTCTRL_MUXPOS_PIN0_Val); // PA02
    uint16_t raw_lcd   = adc_read(ADC_INPUTCTRL_MUXPOS_PIN4_Val); // PA04
    uint16_t raw_uc    = adc_read(ADC_INPUTCTRL_MUXPOS_PIN5_Val); // PA05
    uint16_t raw_ext   = adc_read(ADC_INPUTCTRL_MUXPOS_PIN6_Val); // PA06

// 1. Converting raw to millivolts
float v_total = ((float)raw_total * Vref) / 4095.0;
float v_lcd   = ((float)raw_lcd   * Vref) / 4095.0;
float v_uc    = ((float)raw_uc    * Vref) / 4095.0;
float v_ext   = ((float)raw_ext   * Vref) / 4095.0;

// 2. Current (I = Vout / (Gain * Rshunt))
// Gain = 20.0

// I LCD (R 0.1 Ohm): I = V / (20 * 0.1) = V / 2
float i_lcd = (v_lcd / (20.0 * 0.1)); 

// I uC (R 1.0 Ohm): I = V / (20 * 1.0) = V / 20
float i_uc  = (v_uc / (20.0 * 1.0));

// I Extension board (R 1.0 Ohm): I = V / (20 * 1.0) = V / 20
float i_ext = (v_ext / (20.0 * 1.0));

// Total I (R 0.1 Ohm ): I = V / 2
float i_total = (v_total /  (20.0 * 0.1));



    

    // 1. LCD Display output
    lcd.setCursor(0, 0);
   /* if (fault) {
        lcd.print("Status: FAULT  ");
        lcd.setCursor(0, 1);
        lcd.print("Error: 0x"); lcd.print(fault, HEX);
    } else {*/
        lcd.print("it"); lcd.print(i_total, 2); lcd.print(" iL"); lcd.print(i_lcd, 2); lcd.print("   ");
        lcd.setCursor(0, 1);
        lcd.print("iC:"); lcd.print(i_uc, 2); lcd.print(" iE:"); lcd.print(i_ext, 2); lcd.print("   ");
    

    // 2. Manually construct and send the JSON string
    // Using SerialUSB to match JS data on Dashboeard
    SerialUSB.print("{\"temp\":");       SerialUSB.print(temperature, 1);
    SerialUSB.print(",\"temp_mv\":");    SerialUSB.print(v_pt100_mv, 1);
    SerialUSB.print(",\"i_total\":");    SerialUSB.print(i_total, 1); 
    SerialUSB.print(",\"i_lcd\":");      SerialUSB.print(i_lcd, 1);   
    SerialUSB.print(",\"i_uc\":");       SerialUSB.print(i_uc, 1);    
    SerialUSB.print(",\"i_ext\":");      SerialUSB.print(i_ext, 1);   
    SerialUSB.print(",\"v_sh_tot\":");    SerialUSB.print(v_total, 0); 
    SerialUSB.print(",\"v_sh_lcd\":");      SerialUSB.print(v_lcd, 0);   
    SerialUSB.print(",\"v_sh_uc\":");       SerialUSB.print(v_uc, 0);    
    SerialUSB.print(",\"v_sh_ext\":");      SerialUSB.print(v_ext, 0);   
    SerialUSB.println("}");


   
    if (fault) {
        thermo.clearFault();
    }
    /* //blink o LED 
    if (LedOn) {
        LedOn = 0;
        digitalWrite(2,LedOn);
    } else {
        LedOn = 1;
        digitalWrite(2,LedOn);
    }*/

    delay(1000); 


}
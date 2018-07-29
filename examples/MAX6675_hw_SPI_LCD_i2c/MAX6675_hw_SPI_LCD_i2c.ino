/***************************************************************************************************/
/* 
   Example for 12-bit MAX6675 K-Thermocouple to Digital Converter with Cold Junction Compensation

   written by : enjoyneering79
   sourse code: https://github.com/enjoyneering/MAX6675

   - MAX6675 power supply voltage is 3.0 - 5.5v
   - K-type thermocouples have an absolute accuracy of around ±2°C
   - Measurement tempereture range 0°C...+1024°C with 0.25°C resolution
   - Cold junction compensation range -20°C...+85°C
   - Keep k-type thermocouple cold junction & MAX6675 at the same temperature
   - Avoid placing heat-generating devices or components near the converter
     because this may produce errors
   - It is strongly recommended to add a 10nF/0.01mF ceramic surface-mount capacitor, placed across
     the T+ and T- pins, to filter noise on the thermocouple lines.

   This lcd uses I2C bus to communicate, specials pins are required to interface
   Board:                                    SDA                    SCL                    Level
   Uno, Mini, Pro, ATmega168, ATmega328..... A4                     A5                     5v
   Mega2560................................. 20                     21                     5v
   Due, SAM3X8E............................. 20                     21                     3.3v
   Leonardo, Micro, ATmega32U4.............. 2                      3                      5v
   Digistump, Trinket, ATtiny85............. 0/physical pin no.5    2/physical pin no.7    5v
   Blue Pill, STM32F103xxxx boards.......... PB7                    PB6                    3.3v/5v
   ESP8266 ESP-01........................... GPIO0/D5               GPIO2/D3               3.3v/5v
   NodeMCU 1.0, WeMos D1 Mini............... GPIO4/D2               GPIO5/D1               3.3v/5v
   ESP32.................................... GPIO21/D21             GPIO22/D22             3.3v

   This sensor uses SPI bus to communicate, specials pins are required to interface
   Board:                                    MOSI        MISO        SCLK         SS, don't use for CS   Level
   Uno, Mini, Pro, ATmega168, ATmega328..... 11          12          13           10                     5v
   Mega, Mega2560, ATmega1280, ATmega2560... 51          50          52           53                     5v
   Due, SAM3X8E............................. ICSP4       ICSP1       ICSP3        x                      3.3v
   Leonardo, ProMicro, ATmega32U4........... 16          14          15           x                      5v
   Blue Pill, STM32F103xxxx boards.......... PA17        PA6         PA5          PA4                    3v
   NodeMCU 1.0, WeMos D1 Mini............... GPIO13/D7   GPIO12/D6   GPIO14/D5    GPIO15/D8*             3v/5v
   ESP32.................................... GPIO23/D23  GPIO19/D19  GPIO18/D18   x                      3v

                                            *if GPIO2/D4 or GPIO0/D3 used for for CS, apply an external 25kOhm
                                             pullup-down resistor
   Frameworks & Libraries:
   ATtiny Core           - https://github.com/SpenceKonde/ATTinyCore
   ESP32 Core            - https://github.com/espressif/arduino-esp32
   ESP8266 Core          - https://github.com/esp8266/Arduino
   ESP8266 I2C lib fixed - https://github.com/enjoyneering/ESP8266-I2C-Driver
   STM32 Core            - https://github.com/rogerclarkmelbourne/Arduino_STM32

   GNU GPL license, all text above must be included in any redistribution, see link below for details:
   - https://www.gnu.org/licenses/licenses.html
*/
/***************************************************************************************************/
#include <SPI.h>
#include <Wire.h>               //for ESP8266 use bug free i2c driver https://github.com/enjoyneering/ESP8266-I2C-Driver
#include <MAX6675.h>
#include <LiquidCrystal_I2C.h>  //https://github.com/enjoyneering/LiquidCrystal_I2C

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#endif

#define LCD_ROWS           4    //qnt. of lcd rows
#define LCD_COLUMNS        20   //qnt. of lcd columns

#define LCD_DEGREE_SYMBOL  0xDF //degree symbol from lcd ROM, see p.9 of GDM2004D datasheet
#define LCD_SPACE_SYMBOL   0x20 //degree symbol from lcd ROM, see p.9 of GDM2004D datasheet

#define MAX_TEMPERATURE    45   //max temp, °C

const uint8_t iconTemperature[8] PROGMEM = {0x04, 0x0E, 0x0E, 0x0E, 0x0E, 0x1F, 0x1F, 0x0E}; //PROGMEM saves variable to flash & keeps dynamic memory free

float temperature = 0;

/*
MAX6675(cs)

cs - chip select
*/

MAX6675           myMAX6675(4); //for ESP8266 use D4
LiquidCrystal_I2C lcd(PCF8574_ADDR_A21_A11_A01, 4, 5, 6, 16, 11, 12, 13, 14, POSITIVE);


void setup()
{
  #if defined(ESP8266)
  WiFi.persistent(false);                                       //disable saving wifi config into SDK flash area
  WiFi.forceSleepBegin();                                       //disable swAP & station by calling "WiFi.mode(WIFI_OFF)" & put modem to sleep
  #endif

  Serial.begin(115200);

  /* LCD connection check */  
  while (lcd.begin(LCD_COLUMNS, LCD_ROWS, LCD_5x8DOTS) != true) //20x4 display, 5x8 pixels size
  {
    Serial.println(F("PCF8574 is not connected or lcd pins declaration is wrong. Only pins numbers: 4,5,6,16,11,12,13,14 are legal."));
    delay(5000);
  }

  lcd.print(F("PCF8574 is OK")); //(F()) saves string to flash & keeps dynamic memory
  delay(1000);

  lcd.clear();

  /* start MAX6675 */
  myMAX6675.begin();

  while (myMAX6675.getChipID() != MAX6675_ID)
  {
    lcd.setCursor(0, 0);
    lcd.print(F("MAX6675 error"));
    delay(5000);
  }

  lcd.clear();

  lcd.print(F("MAX6675 OK"));
  delay(2000);

  lcd.clear();

  /* load custom symbol to CGRAM */
  lcd.createChar(0, iconTemperature);

  /* prints static text */
  lcd.setCursor(0, 0);
  lcd.write(0);                                                   //print temperature icon

  if   (myMAX6675.detectThermocouple() == true) Serial.println(F("K-Thermocouple is connected to MAX6675 terminals 'T+' & 'T-'"));
  else                                          Serial.println(F("K-Thermocouple is broken, unplugged or 'T-' terminal is not grounded"));
}

void loop()
{
  temperature = myMAX6675.getTemperature();

  lcd.setCursor(2, 0);
  if (temperature != MAX6675_ERROR) lcd.print(temperature, 1);
  else                              lcd.print(F("xx"));           //thermocouple broken, unplugged or 'T-' terminal is not grounded
  lcd.write(LCD_DEGREE_SYMBOL);
  lcd.print(F("C"));
  lcd.write(LCD_SPACE_SYMBOL);

  lcd.printHorizontalGraph('T', 3, temperature, MAX_TEMPERATURE); //name of the bar, 3-rd row, current value, max. value

  delay(1000);
}

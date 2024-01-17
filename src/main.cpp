#include <Arduino.h>

#include <GyverOLED.h>
GyverOLED<SSD1306_128x64, OLED_NO_BUFFER> oled;

#define adc (analogRead(pin_A0) / 4.096 * 2.560) * 2.35

#define pin_R 13  // пин нагрузки
#define pin_A0 A1 // аналоговый пин

const uint16_t r_load = 3300;  // точно измеренное сопротивление нагрузки [mOhm]
float i_load;                  // ток нагрузки [А]
const uint8_t u_mosfeet = 6.5; // падение напряжения на открытом полевике [mV]
float u_bat;                   // напряжение батареи и каждой банки без нагрузки [mV]
float u_bat_r;                 // напряжение батареи и каждой банки под нагрузкой [mV]
float r_bat;                   // вн. сопротивление батареи и каждой банки [mOhm]

const uint8_t frame0_16x16[] PROGMEM = {
    0b00000000,
    0b11000000,
    0b11110000,
    0b00111000,
    0b00011000,
    0b00001100,
    0b00001100,
    0b00001100,
    0b00001100,
    0b00001100,
    0b00001100,
    0b00011000,
    0b00111000,
    0b11110000,
    0b11000000,
    0b00000000,
    0b00000000,
    0b10000011,
    0b10000111,
    0b11001110,
    0b11111100,
    0b01111000,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000,
    0b01111000,
    0b11111100,
    0b11001110,
    0b10000111,
    0b10000011,
    0b00000000,
};

void setup()
{
  Serial.begin(11520);
  oled.init();             // инициализация
  oled.clear();            // очистить дисплей (или буфер)

  pinMode(pin_R, OUTPUT);
  digitalWrite(pin_R, LOW); // MOSFEET отключение нагрузки

  analogReference(INTERNAL2V56); // INTERNAL1V024 INTERNAL1V25 INTERNAL2V048 INTERNAL2V56 INTERNAL4V096 DEFAULT
}

void loadCycle();
void impedance();
void led();

void loop()
{
  impedance();
  delay(500);
}

//*******************  Измерение под нагрузкой  **********************
void loadCycle()
{
  u_bat_r = u_bat = 0;
  for (uint8_t i = 0; i < 7; i++)
  {
    digitalWrite(pin_R, HIGH); // ПОДКЛЮЧЕНИЕ НАГРУЗКИ
    delay(50);
    u_bat_r += adc;
    digitalWrite(pin_R, LOW); // ОТКЛЮЧЕНИЕ НАГРУЗКИ
    delay(50);
    u_bat += adc;
  }
  u_bat_r = u_bat_r / 7;
  u_bat = u_bat / 7;
}

//***************** I M P E D A N C E **************************
void impedance()
{
  digitalWrite(pin_R, LOW);
  if (adc > 2500)
  {
    loadCycle();
    i_load = 1.132;
    r_bat = (u_bat - u_bat_r) / i_load;
  }
  else
  {
    u_bat = u_bat_r = i_load = r_bat = 0;
  }
  led();
}

void led()
{
  oled.setScale(2);

  oled.setCursorXY(0, 0);      // курсор в (X, Y)
  oled.print(u_bat / 1000, 2); //
  oled.print("v");

  oled.setCursorXY(70, 0);       // курсор в (X, Y)
  oled.print(u_bat_r / 1000, 2); //
  oled.print("v");

  oled.setCursorXY(35, 20); // курсор в (X, Y)
  oled.print(i_load, 2);    //
  oled.print("a  ");

  oled.setCursorXY(10, 45); // курсор в (X, Y)
  r_bat < 10 ? oled.print("00") : oled.print("");
  r_bat > 9 && r_bat < 100 ? oled.print("0") : oled.print("");
  oled.print(r_bat, 2); //

  oled.setCursorXY(85, 45);
  oled.print("m");

  oled.drawBitmap(100, 42, frame0_16x16, 16, 16);
}
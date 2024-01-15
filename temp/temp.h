
https://forum.alnado.ru/viewtopic.php?t=1533

// Измеритель внутреннего сопротивления Li-Ion аккумуляторов


// Берешь, делаешь необходимые замеры, записываешь показания и считаешь по формуле r=Е/I−R, 
// где Е - электродвижущая сила, равна напряжению на неподключенном ни к чему аккумуляторе,
//  I - сила тока, протекающая, через цепь, R - сопротивление нагрузочного резистора,
//   r - искомое внутреннее сопротивление. Лень считать вручную - можно поручить эту задачу экселю, сразу наглядно видно где какие значения.

#include "U8glib.h"
#include <Adafruit_ADS1015.h>

U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_DEV_0 | U8G_I2C_OPT_NO_ACK | U8G_I2C_OPT_FAST); // Fast I2C / TWI
Adafruit_ADS1115 ads;

const uint8_t OMEGA_BITMAP[] PROGMEM = {
  0x3c,   // 00111100
  0x42,   // 01000010
  0x81,   // 10000001
  0x81,   // 10000001
  0x81,   // 10000001         // Символ "Омега"
  0x81,   // 10000001
  0x42,   // 01000010
  0x24,   // 00100100
  0x24,   // 00100100
  0xe7    // 11100111
};
const uint16_t r_load = 21700;        // точно измеренное сопротивление нагрузки [mOhm]
float i_load;                         // ток нагрузки [А]
const uint8_t u_mosfeet = 10;         // падение напряжения на открытом полевике [mV]
float u_bat, u1, u2, u3;              // напряжение батареи и каждой банки без нагрузки [mV]
float u_bat_r, u1_r, u2_r, u3_r;      // напряжение батареи и каждой банки под нагрузкой [mV]
float r_bat, r1, r2, r3;              // вн. сопротивление батареи и каждой банки [mOhm]
uint8_t banks = 3;                    // количество банок в измеряемом аккумуляторе
uint8_t adc_num = 1;                  // текущий номер канала АЦП (1, 2, 3)
int16_t adc1, adc2, adc3;             // отсчеты АЦП без нагрузки
int16_t adc1_r, adc2_r, adc3_r;       // отсчеты АЦП под нагрузкой
const float k1 = 0.1875;              // 1 bit = 0.1875mV  вход 1 (НЕ ИЗМЕНЯТЬ!)
const float k2 = 0.375;               // 1 bit = 0.375mV (k1*2) вход 2 (подобрать при калибровке)
const float k3 = 0.5625;              // 1 bit = 0.5625mV (k1*3) вход3 (подобрать при калибровке)


//******************** D R A W  ********************************
// вывод данных на дисплей
void draw() {
  u8g.firstPage();
  do {
    u8g.drawStr( 52, 12, "V");
    u8g.drawStr(108, 12, "m");
    u8g.drawStr(108, 31, "m");
    u8g.drawStr(108, 47, "m");
    u8g.drawStr(108, 63, "m");
    u8g.drawBitmapP( 119,  2, 1, 10, OMEGA_BITMAP);
    u8g.drawBitmapP( 119, 21, 1, 10, OMEGA_BITMAP);
    u8g.drawBitmapP( 119, 37, 1, 10, OMEGA_BITMAP);
    u8g.drawBitmapP( 119, 53, 1, 10, OMEGA_BITMAP);
    u8g.drawStr(0, 31, "v3");
    u8g.drawStr(0, 47, "v2");
    u8g.drawStr(0, 63, "v1");
    u8g.setPrintPos(0, 12);
    u8g.print( u_bat / 1000.0, 3);
    u8g.setPrintPos(78, 12);
    u8g.print( r_bat, 0);
    u8g.setPrintPos(22, 31);
    u8g.print( u3 / 1000.0, 3);
    u8g.setPrintPos(22, 47);
    u8g.print( u2 / 1000.0, 3);
    u8g.setPrintPos(22, 63);
    u8g.print( u1 / 1000.0, 3);
    u8g.setPrintPos(78, 31);
    u8g.print( r3, 0);
    u8g.setPrintPos(78, 47);
    u8g.print( r2, 0);
    u8g.setPrintPos(78, 63);
    u8g.print( r1, 0);
  } while ( u8g.nextPage() );
}


//*******************  L O A D  C Y C L E  **********************
//в каждом нагрузочном цикле измеряется поочередно по одной банке
//для максимального равенства условий измерений
void loadCycle() {
  digitalWrite(2, HIGH);    // ПОДКЛЮЧЕНИЕ НАГРУЗКИ
  delay (2);
  switch (adc_num) {
    case 1:
      adc1_r = ads.readADC_SingleEnded(1);
      break;
    case 2:
      adc2_r = ads.readADC_SingleEnded(2);
      break;
    case 3:
      adc3_r = ads.readADC_SingleEnded(3);
      break;
  }
  digitalWrite(2, LOW);     // ОТКЛЮЧЕНИЕ НАГРУЗКИ
  adc_num++;
  if (adc_num > banks) adc_num = 1;
}


//***************** I M P E D A N C E **************************
void impedance() {
  digitalWrite(2, LOW);                 // отключение нагрузки
  adc1 = ads.readADC_SingleEnded(1);    // оцифровка
  adc2 = ads.readADC_SingleEnded(2);
  adc3 = ads.readADC_SingleEnded(3);

  if (abs(adc1 < 24)) banks = 0;        // определяем количество банок
  else if (abs(adc2 < 12)) banks = 1;   // порог обнаружения 4,5 mV
  else if (abs(adc3 < 8)) banks = 2;
  else banks = 3;

  switch (banks) {
    case 0:                              // ничего не подключено
      u1 = 0, u2 = 0, u3 = 0, u_bat = 0;
      r1 = 0, r2 = 0, r3 = 0, r_bat = 0;
      break;
    case 1:                              // одна банка
      u2 = 0, u3 = 0, r2 = 0, r3 = 0;
      u_bat = adc1 * k1;
      u1 = u_bat;
      loadCycle();
      u_bat_r = adc1_r * k1;
      i_load = (u_bat_r - u_mosfeet) / r_load;
      r_bat = (u_bat - u_bat_r) / i_load;
      r1 = r_bat;
      break;
    case 2:                               // две банки
      u3 = 0, r3 = 0;
      u_bat = adc2 * k2;
      u1 = adc1 * k1;
      u2 = u_bat - u1;
      loadCycle();
      u_bat_r = adc2_r * k2;
      u1_r = adc1_r * k1;
      u2_r = u_bat_r - u1_r;
      i_load = (u_bat_r - u_mosfeet) / r_load;
      r_bat = (u_bat - u_bat_r) / i_load;
      r1 = (u1 - u1_r) / i_load;
      r2 = r_bat - r1;
      break;
    case 3:                                // три банки
      u_bat = adc3 * k3;
      u1 = adc1 * k1;
      u2 = adc2 * k2 - u1;
      u3 = u_bat - u1 - u2;
      loadCycle();
      u_bat_r = adc3_r * k3;
      u1_r = adc1_r * k1;
      u2_r = adc2_r * k2 - u1_r;
      u3_r = u_bat_r - u1_r - u2_r;
      i_load = (u_bat_r - u_mosfeet) / r_load;
      r_bat = (u_bat - u_bat_r) / i_load;
      r1 = (u1 - u1_r) / i_load;
      r2 = (u2 - u2_r) / i_load;
      r3 = r_bat - r2 - r1;
      break;
  }
  draw();
  delay(200);
}


void setup() {
  ads.setGain(GAIN_TWOTHIRDS);    // 2/3x gain +/- 6.144V  1 bit = 0.1875mV (default)
  ads.begin();
  u8g.setColorIndex(1);           // pixel on
  u8g.setFont(u8g_font_unifont);
  pinMode(2, OUTPUT);
  digitalWrite(2, LOW);           // MOSFEET отключение нагрузки
  pinMode(3, OUTPUT);             // сброс дисплея
  digitalWrite(3, HIGH);
}

void loop() {
  impedance();
  //calibrate();
}

//================   C A L I B R A T E  ========================
// Для калибровки входных делителей временно закоментировать
// в void loop() impedance() и раскоментировать calibrate().
// Все три входа прибора соединить и подключить на первую банку.
// Подбором констант k2 и k3 добиться максимально близких показаний v2=v3=v1.
// v1 в данном случае является эталоном, по нему и подстраиваем
// k1 не изменять!
// После калибровки вернуть в void loop() impedance() и закоментировать calibrate()

void calibrate() {
  adc1 = ads.readADC_SingleEnded(1);  // оцифровка
  adc2 = ads.readADC_SingleEnded(2);
  adc3 = ads.readADC_SingleEnded(3);
  u1 = adc1 * k1;                     // вычисление напряжений
  u2 = adc2 * k2;
  u3 = adc3 * k3;
  u8g.firstPage();                    // вывод на дисплей
  do {
    u8g.drawStr(15, 12, "Calibration");
    u8g.drawStr(0, 31, "v3");
    u8g.drawStr(0, 47, "v2");
    u8g.drawStr(0, 63, "v1");
    u8g.setPrintPos(22, 31);
    u8g.print( u3, 3);
    u8g.setPrintPos(22, 47);
    u8g.print( u2, 3);
    u8g.setPrintPos(22, 63);
    u8g.print( u1, 3);
  } while ( u8g.nextPage() );
}
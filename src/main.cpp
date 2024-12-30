#include <Arduino.h>

// https://github.com/vindar/ILI9341_T4
// https://github.com/vindar/ILI9341_T4/blob/main/examples/dualDisplay/dualDisplay.ino
// https://vindar.github.io/tgx/html/md_docs_intro_api_2_d.html#subsec_text

#include <SPI.h>
// the screen driver library
#include <ILI9341_t3n.h>

#include <ili9341_t3n_font_Arial.h>

#define DEBUG

//
// IL9341 is WIRED TO SPI0
//
constexpr uint8_t PIN_SCK0 = 13;
constexpr uint8_t PIN_MISO0 = 12;
constexpr uint8_t PIN_MOSI0 = 11;
constexpr uint8_t PIN_CS0 = 10;
// if SPI0 is IL9341
constexpr uint8_t PIN_DC0 = 36;         // mandatory, can be any pin but using pin 10 (or 36 or 37 on T4.1) provides greater performance
constexpr uint8_t PIN_RESET0 = 6;       // optional (but recommended), can be any pin.
constexpr uint8_t PIN_BACKLIGHT0 = 255; // optional, set this only if the screen LED pin is connected directly to the Teensy.
constexpr uint8_t PIN_TOUCH_IRQ0 = 255; // optional, set this only if the touchscreen is connected on the same SPI bus
constexpr uint8_t PIN_TOUCH_CS0 = 255;  // optional, set this only if the touchscreen is connected on the same SPI bus

//
// AU IS WIRED TO SPI1
//
// The AU uses the following pins on the Br :
//    • cs = A2->Teensy pin 0(yellow)
//    • sck = A5 → Teensy pin 27(orange)
//    • miso = A9->Teensy pin 1(red)
//    • mosi = A6->Teensy pin 26(brown)
//    • data_rdy = A8->Teensy pin 2(OUT2)(black)
//    • GND->Teensy GND(green)

constexpr uint8_t PIN_SCK1 = 27;
constexpr uint8_t PIN_MISO1 = 1;     // mandatory  (if the display has no MISO line, set this to 255 but then VSync will be disabled)
constexpr uint8_t PIN_MOSI1 = 26;    // mandatory
constexpr uint8_t PIN_DC1 = 38;      // mandatory, can be any pin but using pin 0 (or 38 on T4.1) provides greater performance
constexpr uint8_t PIN_CS1 = 0;       // optional (but recommended), can be any pin.
constexpr uint8_t PIN_SDATA_RDY = 5; // AU DATA READY signal (IN2)

constexpr uint8_t PIN_RESET1 = 255;     // 29 optional (but recommended), can be any pin.
constexpr uint8_t PIN_BACKLIGHT1 = 255; // optional, set this only if the screen LED pin is connected directly to the Teensy.
constexpr uint8_t PIN_TOUCH_IRQ1 = 255; // optional. set this only if the touchscreen is connected on the same SPI bus
constexpr uint8_t PIN_TOUCH_CS1 = 255;  // optional. set this only if the touchscreen is connected on the same SPI bus

constexpr uint32_t SPI_SPEED = 24000000;

constexpr char *tqbf = "the quick brown fox jumps over the lazy dog0123456789THE QUICK BROWN FOX JUMPS OVER THE LAZY DOG\r\n\
the quick brown fox jumps over the lazy dog0123456789THE QUICK BROWN FOX JUMPS OVER THE LAZY DOG\r\n";

#define LANDSCAPE

constexpr uint8_t ROT_PORTRAIT = 1;
constexpr uint8_t ROT_LANDSCAPE = 0;

#ifdef PORTRAIT
// screen size in portrait mode
constexpr uint32_t LXP = 240;
constexpr uint32_t LYP = 320;
#endif
#ifdef LANDSCAPE
// screen size in landscape mode
constexpr uint32_t LXP = 320;
constexpr uint32_t LYP = 240;
#endif
// screen driver object
static ILI9341_t3n tft(PIN_CS0, PIN_DC0, PIN_RESET0, PIN_MOSI0, PIN_SCK0, PIN_MISO0); // for screen on SPI0

static void setup_spi_au();
static void setup_spi_il9341();
static void print_hex(uint8_t x);
static void wait_sdata();

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);
  for (int i = 0; i < 1000; i++)
  {
    delay(1);
    if (Serial.available())
    {
      break;
    }
  }
  setup_spi_il9341();
  setup_spi_au();
}

static unsigned char txbuf[256];
static unsigned char rxbuf[256];

void loop()
{
  // wait for data ready from slave
  wait_sdata();
  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(0, 0);
  memset(txbuf, 0xff, sizeof(txbuf));
  size_t count = strlen(tqbf);
  memcpy(txbuf, tqbf, count);
#ifdef DEBUG
  // show transmit buffer
  Serial.print("TX: ");
  for (size_t i = 0; i < count; i++)
  {
    print_hex(txbuf[i]);
  }
  Serial.println();
#endif
  tft.println("Start SPI transfer");
  // AU seems OK with Teensy 4.1 up to 16 MHz
  SPI1.beginTransaction(SPISettings((int)16000000, MSBFIRST, (uint8_t)SPI_MODE1));
  digitalWriteFast(PIN_CS1, LOW);
  auto st = micros();
  SPI1.transfer(txbuf, rxbuf, count);
  auto et = micros();
  digitalWriteFast(PIN_CS1, HIGH);
  SPI1.endTransaction();
  tft.println("Transfer complete");
  auto tt = et - st;
  auto pc = (float)tt / (float)count;
  tft.println("Elapsed: " + String(tt) + " micros, per char: " + String(pc) + " micros");
#ifdef DEBUG
  // show received data
  Serial.print("RX: ");
  for (size_t i = 0; i < count; i++)
  {
    print_hex(rxbuf[i]);
    if (rxbuf[i] != 0xff && txbuf[i] != 0x00)
    {
      tft.print(String((char)rxbuf[i]));
    }
  }
  tft.println(".");
  Serial.println();
#endif
}

static void setup_spi_au()
{
  tft.println("AU SPI1 setup");
  pinMode(PIN_SDATA_RDY, INPUT);
  pinMode(PIN_CS1, OUTPUT);
  digitalWrite(PIN_CS1, HIGH);
  if (SPI1.pinIsMISO(PIN_MISO1))
  {
    tft.print("AU: MISO=HW, ");
  }
  if (SPI1.pinIsMOSI(PIN_MOSI1))
  {
    tft.print("MOSI=HW, ");
  }
  if (SPI1.pinIsSCK(PIN_SCK1))
  {
    tft.print("SCK=HW, ");
  }
  if (SPI1.pinIsChipSelect(PIN_CS1))
  {
    tft.println("CS=HW");
  }
  // SPI1.setCS(PIN_CS1); // do not use
  SPI1.setMOSI(PIN_MOSI1);
  SPI1.setMISO(PIN_MISO1);
  SPI1.setSCK(PIN_SCK1);
  SPI1.begin();
  digitalWrite(PIN_CS1, HIGH);
  tft.println("AU SPI1 initialised");
}

static void setup_spi_il9341()
{
#ifdef DEBUG
  Serial.println("ILI9341 setup");
#endif
  // make sure backlight is on
  if (PIN_BACKLIGHT1 != 255)
  {
    pinMode(PIN_BACKLIGHT1, OUTPUT);
    // digitalWrite(PIN_BACKLIGHT1, HIGH);
    analogWrite(PIN_BACKLIGHT1, 128); // PWM backlight level 50 %
  }
  pinMode(PIN_CS0, OUTPUT);
  tft.begin(); // use default speed (write 30 MHz/ read 2 MHz)
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_WHITE);
  tft.setRotation(ROT_PORTRAIT);
  tft.setFont(Arial_10);
  // tft.setTextSize(2);
  tft.println("tft initialized on SPI 0");
}

static void print_hex(uint8_t x)
{
  if (x < 16)
  {
    Serial.print('0');
  }
  Serial.print(x, HEX);
}

static void wait_sdata()
{
  Serial.println("Waiting for AU data ready");
  tft.println("Waiting for AU SDATA");
  while (digitalRead(PIN_SDATA_RDY) == LOW)
  {
    delay(1);
  }
  Serial.println("AU data ready");
  tft.println("Got AU SDATA");
}

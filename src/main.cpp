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
constexpr uint8_t PIN_DC0 = 36; // mandatory, can be any pin but using pin 10 (or 36 or 37 on T4.1) provides greater performance
constexpr uint8_t PIN_RESET0 = 6; // optional (but recommended), can be any pin.
constexpr uint8_t PIN_BACKLIGHT0 = 255; // optional, set this only if the screen LED pin is connected directly to the Teensy.
constexpr uint8_t PIN_TOUCH_IRQ0 = 255; // optional, set this only if the touchscreen is connected on the same SPI bus
constexpr uint8_t PIN_TOUCH_CS0 = 255; // optional, set this only if the touchscreen is connected on the same SPI bus

//
// AU IS WIRED TO SPI1
//
// The AU uses the following pins on the Br :
//    • cs = A2->Teensy pin 0(yellow)
//    • sck = A5 → Teensy pin 27(orange)
//    • miso = A9->Teensy pin 1(red)
//    • mosi = A6->Teensy pin 26(brown)
//    • GND->Teensy GND(green)

constexpr uint8_t PIN_SCK1 = 27;
constexpr uint8_t PIN_MISO1 = 1;
constexpr uint8_t PIN_MOSI1 = 26;
constexpr uint8_t PIN_CS1 = 0;

constexpr uint8_t PIN_DC1 = 38;      // mandatory, can be any pin but using pin 0 (or 38 on T4.1) provides greater performance
constexpr uint8_t PIN_RESET1 = 255;     // 29 optional (but recommended), can be any pin.
constexpr uint8_t PIN_BACKLIGHT1 = 255; // optional, set this only if the screen LED pin is connected directly to the Teensy.
constexpr uint8_t PIN_TOUCH_IRQ1 = 255; // optional. set this only if the touchscreen is connected on the same SPI bus
constexpr uint8_t PIN_TOUCH_CS1 = 255;  // optional. set this only if the touchscreen is connected on the same SPI bus

constexpr uint32_t SPI_SPEED = 24000000;

const char* tqbf =
"the quick brown fox jumps over the lazy dog0123456789THE QUICK BROWN FOX JUMPS OVER THE LAZY DOG\r\n\
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
static bool spi_test(uint32_t spi_speed, uint32_t loop);
#ifdef DEBUG
static void print_hex(uint8_t x);
#endif

void setup() {
  Serial.begin(115200);
  for (int i = 0; i < 1000; i++) {
    delay(1);
    if (Serial.available()) {
      break;
    }
  }
  setup_spi_il9341();
  setup_spi_au();
}

static unsigned char txbuf[256];
static unsigned char rxbuf[256];

const uint32_t LOOPS = 4; // 4000
const uint32_t LOOP_DELAY = 100; // 0

void loop() {
  for (uint32_t speed = 12000000; speed <= 33000000; speed += 1000000) {
    tft.fillScreen(ILI9341_BLACK);
    tft.setCursor(0, 110);
    tft.setTextColor(ILI9341_WHITE);
    uint32_t MHz = speed / 1000000;
    tft.print("Test at " + String(MHz) + "MHz - ");
    bool ok = true;
    for (uint32_t i = 0; i < LOOPS; i++) {
      if (!(ok = spi_test(speed, i))) {
        tft.setTextColor(ILI9341_RED);
        tft.setCursor(80, 150);
        tft.println(String(MHz) + "MHz test failed");
        delay(2000);
        break;
      }
      delay(LOOP_DELAY);
    }
    if (ok) {
      tft.println(String(MHz) + "MHz test OK");
      delay(500);
    }
    else {
      break;
    }
  }
}

static bool spi_test(uint32_t spi_speed, uint32_t loop) {
  memset(txbuf, 0xff, sizeof(txbuf));
  size_t count = strlen(tqbf);
  memcpy(txbuf, tqbf, count);
#ifdef DEBUG
  // show transmit buffer
  Serial.print("TX" + String(loop) + ": ");
  for (size_t i = 0; i < count; i++) {
    print_hex(txbuf[i]);
  }
  Serial.println();
#endif
  //  AU seems OK with Teensy 4.1 up to 17MHz
  SPI1.beginTransaction(SPISettings((int)spi_speed, MSBFIRST, (uint8_t)SPI_MODE1));
  digitalWrite(PIN_CS1, LOW);
  SPI1.transfer(txbuf, rxbuf, count);
  digitalWrite(PIN_CS1, HIGH);
  SPI1.endTransaction();
#ifdef DEBUG
  // show received data
  Serial.print("RX" + String(loop) + ": ");
  for (size_t i = 0; i < count; i++) {
    print_hex(rxbuf[i]);
  }
  Serial.println();
#endif
  for (size_t i = 0; i < count - 1; ++i) {
    if (txbuf[i] != rxbuf[i + 1]) {
      String e = String(spi_speed / 1000000) + "MHz:  err at " + String(i) + " in loop " + String(loop);
      tft.setTextColor(ILI9341_ORANGE);
      tft.println(e + "\r\n");
#ifdef DEBUG
      Serial.println(e);
#endif
      return false;
    }
  }
  return true;
}
static void setup_spi_au() {
  tft.println("AU SPI1 setup");
  // pinMode(PIN_SDATA_RDY, INPUT);
  pinMode(PIN_CS1, OUTPUT);
  digitalWrite(PIN_CS1, HIGH);
  if (SPI1.pinIsMISO(PIN_MISO1)) {
    tft.print("AU: MISO=HW, ");
  }
  if (SPI1.pinIsMOSI(PIN_MOSI1)) {
    tft.print("MOSI=HW, ");
  }
  if (SPI1.pinIsSCK(PIN_SCK1)) {
    tft.print("SCK=HW, ");
  }
  if (SPI1.pinIsChipSelect(PIN_CS1)) {
    tft.println("CS=HW");
  }
  // SPI1.setCS(PIN_CS1); // do not use
  SPI1.setMOSI(PIN_MOSI1);
  SPI1.setMISO(PIN_MISO1);
  SPI1.setSCK(PIN_SCK1);
  digitalWrite(PIN_CS1, HIGH);
  SPI1.begin();
  tft.println("AU SPI1 initialised");
  delay(500);
}

static void setup_spi_il9341() {
#ifdef DEBUG
  Serial.println("ILI9341 setup");
#endif
  // make sure backlight is on
  if (PIN_BACKLIGHT0 != 255) {
    pinMode(PIN_BACKLIGHT0, OUTPUT);
    // digitalWrite(PIN_BACKLIGHT1, HIGH);
    analogWrite(PIN_BACKLIGHT0, 128); // PWM backlight level 50 %
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

#ifdef DEBUG
static void print_hex(uint8_t x) {
  if (x < 16) {
    Serial.print('0');
  }
  Serial.print(x, HEX);
}
#endif

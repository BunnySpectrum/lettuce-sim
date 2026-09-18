#include <Arduino.h>
#include <stdint.h>

extern const uint8_t digits[10];
extern const uint8_t scrollText[32];
extern const uint8_t promptText[33];

class KenbakUserInterface {
 public:
  KenbakUserInterface(uint16_t pin_strobe, uint16_t pin_clock, uint16_t pin_data)
      : pin_strobe_(pin_strobe), pin_clock_(pin_clock), pin_data_(pin_data) {}

  void send_command(uint8_t value) {
    digitalWrite(pin_strobe_, LOW);
    shiftOut(pin_data_, pin_clock_, LSBFIRST, value);
    digitalWrite(pin_strobe_, HIGH);
  }
  void reset() {
    send_command(0x40);  // set auto increment mode
    digitalWrite(pin_strobe_, LOW);
    shiftOut(pin_data_, pin_clock_, LSBFIRST, 0xc0);  // set starting address to 0
    for (uint8_t i = 0; i < 16; i++) {
      shiftOut(pin_data_, pin_clock_, LSBFIRST, 0x00);
    }
    digitalWrite(pin_strobe_, HIGH);
  }
  bool counting() {
    static uint8_t digit = 0;
    send_command(0x40);
    digitalWrite(pin_strobe_, LOW);
    shiftOut(pin_data_, pin_clock_, LSBFIRST, 0xc0);
    for (uint8_t position = 0; position < 8; position++) {
      shiftOut(pin_data_, pin_clock_, LSBFIRST, digits[digit]);
      shiftOut(pin_data_, pin_clock_, LSBFIRST, 0x00);
    }
    digitalWrite(pin_strobe_, HIGH);
    digit = ++digit % 10;
    return digit == 0;
  }

  bool scroll() {
    static uint8_t index = 0;
    uint8_t scrollLength = sizeof(scrollText);
    send_command(0x40);
    digitalWrite(pin_strobe_, LOW);
    shiftOut(pin_data_, pin_clock_, LSBFIRST, 0xc0);
    for (int i = 0; i < 8; i++) {
      uint8_t c = scrollText[(index + i) % scrollLength];
      shiftOut(pin_data_, pin_clock_, LSBFIRST, c);
      shiftOut(pin_data_, pin_clock_, LSBFIRST, c != 0 ? 1 : 0);
    }
    digitalWrite(pin_strobe_, HIGH);
    index = ++index % (scrollLength << 1);
    return index == 0;
  }
  void setLed(uint8_t value, uint8_t position) {
    pinMode(pin_data_, OUTPUT);
    send_command(0x44);
    digitalWrite(pin_strobe_, LOW);
    shiftOut(pin_data_, pin_clock_, LSBFIRST, 0xC1 + (position << 1));
    shiftOut(pin_data_, pin_clock_, LSBFIRST, value);
    digitalWrite(pin_strobe_, HIGH);
  }
  uint8_t readButtons(void) {
    uint8_t buttons = 0;
    digitalWrite(pin_strobe_, LOW);
    shiftOut(pin_data_, pin_clock_, LSBFIRST, 0x42);
    pinMode(pin_data_, INPUT);
    for (uint8_t i = 0; i < 4; i++) {
      uint8_t v = shiftIn(pin_data_, pin_clock_, LSBFIRST) << i;
      buttons |= v;
    }
    pinMode(pin_data_, OUTPUT);
    digitalWrite(pin_strobe_, HIGH);
    return buttons;
  }
  void buttons() {
    static uint8_t block = 0;
    uint8_t textStartPos = (block / 4) << 3;
    for (uint8_t position = 0; position < 8; position++) {
      send_command(0x44);
      digitalWrite(pin_strobe_, LOW);
      shiftOut(pin_data_, pin_clock_, LSBFIRST, 0xC0 + (position << 1));
      shiftOut(pin_data_, pin_clock_, LSBFIRST, promptText[textStartPos + position]);
      digitalWrite(pin_strobe_, HIGH);
    }
    block = (block + 1) % 16;
    uint8_t buttons = readButtons();
    for (uint8_t position = 0; position < 8; position++) {
      uint8_t mask = 0x1 << position;
      setLed(buttons & mask ? 1 : 0, position);
    }
  }

  void setup() {
    pinMode(pin_strobe_, OUTPUT);
    pinMode(pin_clock_, OUTPUT);
    pinMode(pin_data_, OUTPUT);
    send_command(0x88);
    reset();
  }

 private:
  uint16_t pin_strobe_;
  uint16_t pin_clock_;
  uint16_t pin_data_;
};
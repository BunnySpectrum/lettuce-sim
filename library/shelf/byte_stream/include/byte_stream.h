
#pragma once
#include <stddef.h>
#include <stdint.h>

class ByteStream {
 public:
  virtual void Configure(uint32_t baudrate) = 0;
  virtual uint8_t Available() const = 0;

  virtual size_t Write(const uint8_t* buffer, size_t size) const = 0;
  virtual size_t WriteChar(uint8_t value) const = 0;

  virtual size_t WriteStringC(const char* c_str) const = 0;

  virtual char ReadByte() const = 0;

  size_t WriteX(uint32_t value) const {
    bool started = false;
    for (uint32_t div = 1000000000; div >= 10; div /= 10) {
      if (value < div) {
        if (started) {
          WriteChar('0');
        }
        continue;
      }
      started = true;
      uint8_t rem = (value / div);
      WriteChar(rem + '0');
      value -= rem * div;
    }
    WriteChar(value + '0');
  }

  // Retain the generic 32-bit formatter so the implementations can be
  // compared on the MSP430.
  size_t WriteByteViaWriteX(uint8_t value) const { return WriteX(value); }

  size_t WriteByte(uint8_t value) const {
    size_t wrote = 0;

    // Avoid division and modulo: on the MSP430, integer promotion would make
    // those operations pull in the comparatively expensive division helpers.
    if (value >= 200) {
      wrote += WriteChar('2');
      value -= 200;
    } else if (value >= 100) {
      wrote += WriteChar('1');
      value -= 100;
    }

    if (wrote != 0 || value >= 10) {
      uint8_t tens = 0;
      while (value >= 10) {
        value -= 10;
        ++tens;
      }
      wrote += WriteChar('0' + tens);
    }

    wrote += WriteChar('0' + value);
    return wrote;
  }

  size_t WriteWord(uint16_t value) const { WriteX(value); }

  size_t WriteDWord(uint32_t value) const { WriteX(value); }

  virtual size_t WriteHex(uint8_t value) const {
    WriteChar(kNibbleToHex[(value & 0xF0) >> 4]);
    WriteChar(kNibbleToHex[value & 0xF]);
  }

  virtual size_t WriteOct(uint8_t value) const {
    WriteChar('0' + ((value & 0700) >> 6));
    WriteChar('0' + ((value & 0070) >> 3));
    WriteChar('0' + (value & 0007));
  }

 private:
  static const char kNibbleToHex[16];
};


#pragma once
#include <stddef.h>
#include <stdint.h>

const char kNibbleToHex[] = {'0', '1', '2', '3', '4', '5', '6', '7',
                             '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

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

  size_t WriteByte(uint8_t value) const { WriteX(value); }

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
};
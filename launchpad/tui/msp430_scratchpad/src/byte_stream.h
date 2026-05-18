
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
  virtual size_t Write(uint8_t value) const = 0;

  virtual size_t WriteStringC(const char* c_str) const = 0;
  virtual size_t WriteByte(uint8_t value) const = 0;
  virtual size_t WriteHex(uint8_t value) const {
    Write(kNibbleToHex[(value & 0xF0) >> 4]);
    Write(kNibbleToHex[value & 0xF]);
  }
  virtual size_t WriteWord(uint16_t value) const = 0;
  virtual size_t WriteDWord(uint32_t value) const = 0;

  virtual char ReadByte() const = 0;
};
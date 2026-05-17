
#pragma once
#include <stddef.h>
#include <stdint.h>

class ByteStream {
 public:
  virtual void Configure(uint32_t baudrate) = 0;
  virtual uint8_t Available() const = 0;

  virtual size_t Write(const uint8_t* buffer, size_t size) const = 0;
  virtual size_t Write(uint8_t value) const = 0;

  virtual size_t WriteStringC(const char* c_str) const = 0;
  virtual size_t WriteByte(uint8_t value) const = 0;
  virtual size_t WriteHex(uint8_t value) const = 0;
  virtual size_t WriteWord(uint16_t value) const = 0;
  virtual size_t WriteDWord(uint32_t value) const = 0;

  virtual char ReadByte() const = 0;
};
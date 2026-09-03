#include <Arduino.h>
#include <stdint.h>
#include "byte_stream.h"

class ByteStreamEnergia : public ByteStream {
 public:
  ByteStreamEnergia(uint8_t key) {
    if (key == 0) {
      stream_ = &Serial;
      // } else {
      // stream_ = &Serial1;
    }
  }
  void Configure(uint32_t baudrate) { stream_->begin(baudrate); }

  uint8_t Available() const { return stream_->available(); }

  char ReadByte() const { return stream_->read(); }

  size_t Write(const uint8_t* buffer, size_t size) const { return stream_->write(buffer, size); }

  size_t WriteStringC(const char* c_str) const {
    size_t wrote = 0;
    while(*c_str != '\0'){
      wrote += stream_->write(static_cast<uint8_t>(*c_str++));
    }
    return wrote;
  }

  size_t WriteChar(uint8_t value) const { return stream_->write(value); }

  //  private:
  HardwareSerial* stream_;
};

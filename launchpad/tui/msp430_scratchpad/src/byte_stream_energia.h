#include <Arduino.h>
#include <stdint.h>
#include "byte_stream.h"
#include "ee_composer.h"

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
  size_t Write(uint8_t value) const { return stream_->write(value); }

  size_t WriteStringC(const char* c_str) const { return stream_->print(c_str); }

  size_t WriteByte(uint8_t value) const { stream_->print(value); }
  size_t WriteWord(uint16_t value) const { stream_->print(value); }
  size_t WriteDWord(uint32_t value) const { stream_->print(value); }

  // int ParseStringToInt() const {
  //   return stream_->parseInt
  // }

 private:
  HardwareSerial* stream_;
};

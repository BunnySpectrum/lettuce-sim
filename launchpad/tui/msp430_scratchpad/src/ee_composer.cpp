#include "ee_composer.h"

#define ESC 0x1b
#define CSI '['  // 0x5B
EeComposer::EeComposer(const ByteStream* stream) : stream_(stream) {};

void EeComposer::MoveTo(const EePoint& point) const {
  stream_->WriteStringC("\x1b[");
  stream_->WriteByte(point.row());
  stream_->Write(';');
  stream_->WriteByte(point.col());
  stream_->Write('H');
}

void EeComposer::MoveDown(uint8_t rows) const {
  stream_->WriteStringC("\x1b[");
  stream_->WriteByte(rows);
  stream_->Write('B');
}
void EeComposer::MoveLeft(uint8_t columns) const {
  stream_->WriteStringC("\x1b[");
  stream_->WriteByte(columns);
  stream_->Write('D');
}
void EeComposer::MoveRight(uint8_t columns) const {
  stream_->WriteStringC("\x1b[");
  stream_->WriteByte(columns);
  stream_->Write('C');
}

void EeComposer::ClearScreen() const {
  stream_->WriteStringC("\x1b[2J");
}
void EeComposer::ClearChars(uint8_t len) const {
  if (len <= 0) {
    return;
  }
  stream_->Write(' ');
  if (len == 1) {
    return;
  }
  stream_->WriteStringC("\x1b[");
  stream_->WriteByte(len - 1);
  stream_->Write(0x62);
}
void EeComposer::ClearToEndOfLine() const {
  stream_->WriteStringC("\x1b[");
  stream_->WriteByte(0);
  stream_->Write(0x4B);
}

void EeComposer::ShowCursor(bool show) const {
  stream_->WriteStringC("\x1b[?25");
  if (show) {
    stream_->Write('h');
  } else {
    stream_->Write('l');
  }
}

void EeComposer::ComposeBar(bool is_top, uint8_t len) const {
  uint8_t left_corner, right_corner;
  if (is_top) {
    left_corner = '/';
    right_corner = '\\';
  } else {
    left_corner = '\\';
    right_corner = '/';
  }

  stream_->Write(left_corner);
  stream_->WriteStringC("-\x1b[");
  stream_->WriteByte(len - 3);
  stream_->Write(0x62);
  stream_->Write(right_corner);
}

void EeComposer::ComposeStringC(const char* text) const {
  stream_->WriteStringC(text);
}
#undef ESC
#undef CSI
#include "ee_composer.h"

#define ESC 0x1b
#define CSI '[' // 0x5B
EeComposer::EeComposer(const ByteStream *stream) : stream_(stream) {};

void EeComposer::MoveTo(const EePoint &point) const
{
  stream_->WriteStringC("\x1b[");
  stream_->WriteByte(point.row());
  stream_->WriteChar(';');
  stream_->WriteByte(point.col());
  stream_->WriteChar('H');
}

void EeComposer::MoveTo(uint8_t row, uint8_t col) const
{
  stream_->WriteStringC("\x1b[");
  stream_->WriteByte(row);
  stream_->WriteChar(';');
  stream_->WriteByte(col);
  stream_->WriteChar('H');
}

void EeComposer::MoveDown(uint8_t rows) const
{
  stream_->WriteStringC("\x1b[");
  stream_->WriteByte(rows);
  stream_->WriteChar('B');
}
void EeComposer::MoveLeft(uint8_t columns) const
{
  stream_->WriteStringC("\x1b[");
  stream_->WriteByte(columns);
  stream_->WriteChar('D');
}
void EeComposer::MoveRight(uint8_t columns) const
{
  stream_->WriteStringC("\x1b[");
  stream_->WriteByte(columns);
  stream_->WriteChar('C');
}

void EeComposer::ClearScreen() const
{
  stream_->WriteStringC("\x1b[2J");
}
void EeComposer::ClearChars(uint8_t len) const
{
  if (len <= 0)
  {
    return;
  }
  stream_->WriteChar(' ');
  if (len == 1)
  {
    return;
  }
  stream_->WriteStringC("\x1b[");
  stream_->WriteByte(len - 1);
  stream_->WriteChar(0x62);
}
void EeComposer::ClearToEndOfLine() const
{
  stream_->WriteStringC("\x1b[");
  stream_->WriteByte(0);
  stream_->WriteChar(0x4B);
}

void EeComposer::ShowCursor(bool show) const
{
  stream_->WriteStringC("\x1b[?25");
  if (show)
  {
    stream_->WriteChar('h');
  }
  else
  {
    stream_->WriteChar('l');
  }
}

void EeComposer::ComposeBar(bool is_top, uint8_t len) const
{
  uint8_t left_corner, right_corner;
  if (is_top)
  {
    left_corner = '/';
    right_corner = '\\';
  }
  else
  {
    left_corner = '\\';
    right_corner = '/';
  }

  stream_->WriteChar(left_corner);
  stream_->WriteStringC("-\x1b[");
  stream_->WriteByte(len - 3);
  stream_->WriteChar(0x62);
  stream_->WriteChar(right_corner);
}

void EeComposer::ComposeDiv(uint8_t len) const
{

  if (len > 3)
  {
    stream_->WriteChar('|');
    stream_->WriteStringC("-\x1b[");
    stream_->WriteByte(len - 3);
    stream_->WriteChar(0x62);
    stream_->WriteChar('|');
  }
  else
  {
    switch (len)
    {
    case 3:
      stream_->WriteStringC("|-|");
      break;
    case 2:
      stream_->WriteStringC("||");
      break;
    case 1:
      stream_->WriteChar('|');
      break;
    }
  }
}

size_t EeComposer::ComposeStringC(const char *text) const
{
  return stream_->WriteStringC(text);
}
#undef ESC
#undef CSI
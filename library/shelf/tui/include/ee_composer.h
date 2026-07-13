#pragma once

#include <stdint.h>
#include "byte_stream.h"
#include "ee_point.h"

class EeComposer {
 public:
  EeComposer(const ByteStream* stream);
  void MoveTo(const EePoint& point) const;
  void MoveTo(uint8_t row, uint8_t col) const;
  void MoveLeft(uint8_t columns) const;
  void MoveRight(uint8_t columns) const;
  void MoveDown(uint8_t rows) const;

  void ClearScreen() const;
  void ClearChars(uint8_t len) const;
  void ClearToEndOfLine() const;
  void ShowCursor(bool show) const;

  void ComposeDiv(uint8_t len) const;
  void ComposeBar(bool is_top, uint8_t len) const;
  void ComposeStringC(const char* text) const;

  void SetContext(EePoint origin, uint8_t col_count, uint8_t row_count);

  //  private:
  const ByteStream* stream_;
};

/* Trades
// CR and LR was +44B ROM, -4B room
// void CarriageReturn(const RenderContext* context) const;
// void LineFeed(RenderContext* context) const;
  
// void EeComposer::CarriageReturn(const RenderContext* context) const {
//   MoveTo(EePoint(context->origin.col(), context->active_row));
// }
// void EeComposer::LineFeed(RenderContext* context) const {
//   stream_->Write('\n');
//   context->active_row++;
// }

*/
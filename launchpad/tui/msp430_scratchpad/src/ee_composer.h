#pragma once

#include <stdint.h>
#include "byte_stream.h"
#include "ee_point.h"

struct RenderContext {
  // adding a constructor was +12B ROM, and -4B room
  // RenderContext(const EePoint& ctx_origin, uint8_t width, uint8_t height)
  //     : origin(ctx_origin), col_count(width), row_count(height), active_row(ctx_origin.row()) {}
  EePoint origin;
  uint8_t active_row;

  // adding col&row count was: +12B ROM, -2B room
  // uint8_t col_count;
  // uint8_t row_count;
};

class EeComposer {
 public:
  EeComposer(const ByteStream* stream);
  void MoveTo(const EePoint& point) const;
  void MoveLeft(uint8_t columns) const;
  void MoveDown(uint8_t rows) const;

  void ClearScreen() const;
  void ClearChars(uint8_t len) const;
  void ShowCursor(bool show) const;

  void ComposeBar(bool is_top, uint8_t len) const;
  void ComposeStringC(const char* text) const;

  void SetContext(EePoint origin, uint8_t col_count, uint8_t row_count);

  // CR and LR was +44B ROM, -4B room
  // void CarriageReturn(const RenderContext* context) const;
  // void LineFeed(RenderContext* context) const;

  //  private:
  const ByteStream* stream_;
};
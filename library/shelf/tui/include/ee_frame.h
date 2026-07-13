#pragma once
#include <stdint.h>
#include "ee_composer.h"
#include "ee_node.h"

#define CONTENT_OFFSET_COL 1
#define CONTENT_OFFSET_ROW 1
class EeFrame : public EeNode {
 public:
  EeFrame(EePoint origin, uint8_t width, uint8_t height, void (*app)())
      : EeNode(origin), width_(width), height_(height), app_(app) {};

  uint8_t width() const { return width_; }
  uint8_t height() const { return height_; }
  void Compose(const EeComposer& composer) {
    composer.MoveTo(origin());
    // composer.ComposeBar(true /*is_top*/, width());
    composer.ComposeDiv(width());

    // Move to content origin
    composer.MoveTo(origin());
    composer.MoveRight(CONTENT_OFFSET_COL);
    composer.MoveDown(CONTENT_OFFSET_ROW);
    app_();

    composer.MoveTo(origin());
    composer.MoveDown(height());
    // composer.ComposeBar(false /*is_top*/, width());
    composer.ComposeDiv(width());
  }

 private:
  uint8_t width_;
  uint8_t height_;
  void (*app_)();
};
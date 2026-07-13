#pragma once
#include <stdint.h>
#include "ee_composer.h"
#include "ee_node.h"
#include "locale.h"

class EeText : public EeNode {
 public:
  EeText(EePoint origin) : EeNode(origin), pre_clear_(0), post_clear_(0) {};

  void SetText(const char* text) {
    uint8_t new_width = TextWidth(text);
    uint8_t old_width = TextWidth(text_);
    if (new_width >= old_width) {
      pre_clear_ = 0;
      post_clear_ = 0;
    } else {
      pre_clear_ = 0;
      post_clear_ = old_width - new_width;
    }

    text_ = text;
  }
  void SetTextRaw(const char* text) { text_ = text; }
  void Compose(const EeComposer& composer) {

    composer.MoveTo(origin());
    if (pre_clear_ > 0) {
      composer.ClearChars(pre_clear_);
      composer.MoveLeft(1);
    }
    composer.ComposeStringC(text_);
    if (post_clear_ > 0) {
      composer.ClearChars(post_clear_);
      composer.MoveLeft(1);
    }

    pre_clear_ = 0;
    post_clear_ = 0;
  }

 private:
  const char* text_;
  uint8_t pre_clear_;
  uint8_t post_clear_;
};
// using max_width was: +8 RAM, +18 ROM
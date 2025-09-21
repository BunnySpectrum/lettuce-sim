#pragma once
#include <stdint.h>
#include "ee_composer.h"
#include "ee_node.h"
#include "locale.h"

struct FormatData {
  union {
    uint16_t word;
    uint8_t byte;
  } data;
  uint8_t tag;

  explicit FormatData(uint16_t word) : tag(2) { data.word = word; };
  explicit FormatData(uint8_t byte) : tag(1) { data.byte = byte; };
};

struct LogMessage {
  const char* const* text;
  FormatData* data;
};

class EeMessage : public EeNode {
 public:
  EeMessage(EePoint origin, uint8_t max_width, LogMessage* msg)
      : EeNode(origin), max_width_(max_width), pre_clear_(0), post_clear_(0), msg_(msg) {};

  uint8_t max_width() const { return max_width_; }
  void SetMessage(LogMessage* msg, uint8_t lang, uint8_t old_lang) {
    uint8_t new_width = TextWidth(msg->text[lang]);
    uint8_t old_width = TextWidth(msg_->text[old_lang]);
    if (new_width >= old_width) {
      pre_clear_ = 0;
      post_clear_ = 0;
    } else {
      pre_clear_ = 0;
      post_clear_ = old_width - new_width;
      switch (msg->data->tag) {
        case 1:
          post_clear_ += 3;
          break;
        case 2:
          post_clear_ += 5;
          break;
      }
      post_clear_ += 4;
    }

    msg_ = msg;
  }
  const uint8_t pre_clear() const { return pre_clear_; }
  const uint8_t post_clear() const { return post_clear_; }
  void Compose(const EeComposer& composer, uint8_t lang) {
    composer.MoveTo(origin());
    if (pre_clear() > 0) {
      composer.ClearChars(pre_clear());
      composer.MoveLeft(1);
    }
    composer.ComposeStringC(msg_->text[lang]);
    composer.stream_->Write(0x20);
    switch (msg_->data->tag) {
      case 1:
        composer.stream_->WriteByte(msg_->data->data.byte);
        break;
      case 2:
        composer.stream_->WriteWord(msg_->data->data.word);
        break;
    }
    if (post_clear() > 0) {
      composer.ClearChars(post_clear());
      composer.MoveLeft(1);
    }

    pre_clear_ = 0;
    post_clear_ = 0;
  }

 private:
  uint8_t max_width_;
  LogMessage* msg_;
  uint8_t pre_clear_;
  uint8_t post_clear_;
};
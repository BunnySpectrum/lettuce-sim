#pragma once
#include <stdint.h>
#include "ee_composer.h"
#include "ee_node.h"

#define CONTENT_OFFSET_COL 1
#define CONTENT_OFFSET_ROW 1
class EeFrame : public EeNode {
 public:
  EeFrame(EePoint origin, uint8_t width, uint8_t height,
          void (*app)(const EeComposer&, RenderContext* context))
      : EeNode(origin), width_(width), height_(height), app_(app) {};

  uint8_t width() const { return width_; }
  uint8_t height() const { return height_; }
  EePoint GetContentOrigin() { return origin() + EePoint(1 /*col*/, 1 /*row*/); }
  void Compose(const EeComposer& composer) {
    composer.MoveTo(origin());
    composer.ComposeBar(true /*is_top*/, width());

    // RenderContext context = {origin() + EePoint(1, 1), width_, height_, origin().row() + 1};
    RenderContext context = {origin() + EePoint(CONTENT_OFFSET_COL, CONTENT_OFFSET_ROW),
                             origin().row() + CONTENT_OFFSET_ROW};
    app_(composer, &context);

    composer.MoveTo(origin() + EePoint(0 /*col*/, height()));
    composer.ComposeBar(false /*is_top*/, width());
  }

 private:
  uint8_t width_;
  uint8_t height_;
  void (*app_)(const EeComposer& composer, RenderContext* context);
};
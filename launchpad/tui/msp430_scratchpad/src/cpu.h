#include <stdint.h>

class Kenbak {

 public:
  Kenbak() : step_(false), cursorAddress_(0) {}

  void toggle_step() { step_ ^= true; }
  void move_cursor_address(int16_t amount) { cursorAddress_ += amount; }

  uint8_t cursor_address() const { return cursorAddress_; }
  bool step() const { return step_; }

 private:
  bool step_;
  uint8_t cursorAddress_;
};
#include "app.h"


// void App::app_decode(const Kenbak& cpuState) {
void App::app_decode(const Kenbak& cpuState, const EeComposer& composer_) {

    if (!update_decode){
        return;
    }else{
        update_decode = false;
    }

    // composer_.MoveTo(kAppDecodeView.origin);
    composer_.MoveTo(kSwPoint);

  auto cursorData = cpuState.memory_read(cpuState.cursor_address());
  size_t wrote = 0;
  wrote += composer_.ComposeStringC("Decode: (");
  wrote += composer_.stream_->WriteOct(cpuState.cursor_address());
  wrote += composer_.ComposeStringC("): ");
  wrote += composer_.stream_->WriteOct(cursorData);

  composer_.MoveDown(1);
  composer_.MoveLeft(wrote);
  wrote = 0;
  if (Kenbak::is_add_sub_load_store(cursorData)) {
    composer_.ComposeStringC("AddSubLoadStore.");
  } else {
    wrote += composer_.ComposeStringC("???");
    composer_.ClearChars(kAppDecodeView.width - wrote);
  }

  // composer.MoveLeft(9 + 3 + 3 + 3);
  // composer.MoveDown(1);
  // if (last_input > 0x20) {
  //   composer.stream_->WriteChar(last_input);
  // }
}

// void App::app_controls() {
void App::app_controls(const EeComposer& composer_) {
    if (!update_controls){
        return;
    }else{
        update_controls = false;
    }

    // composer_.MoveTo(kAppControlsView.origin);
    composer_.MoveTo(kNePoint);
#define RN_CONTROL                             \
  do {                                         \
    composer_.MoveLeft(kAppControlsView.width); \
    composer_.MoveDown(1);                      \
  } while (0);

  composer_.ComposeStringC("PWR");
  RN_CONTROL

  composer_.stream_->WriteStringC("_01");
  RN_CONTROL

  composer_.stream_->WriteStringC("INP");
  RN_CONTROL

  composer_.stream_->WriteStringC("_C+");
  RN_CONTROL

  composer_.stream_->WriteStringC("ADD");
  RN_CONTROL

  composer_.stream_->WriteStringC("_S+");
  RN_CONTROL

  composer_.stream_->WriteStringC("_R+");
  RN_CONTROL

  composer_.stream_->WriteStringC("MEM");
  RN_CONTROL

  composer_.stream_->WriteStringC("_S+");
  RN_CONTROL

  composer_.stream_->WriteStringC("_R+");
  RN_CONTROL

  composer_.stream_->WriteStringC("RUN");
  RN_CONTROL

  composer_.stream_->WriteStringC("_Y+");
  RN_CONTROL

  composer_.stream_->WriteStringC("_N+");
  RN_CONTROL

  composer_.stream_->WriteStringC("_S_");
  RN_CONTROL

#undef RN_CONTROL
}


// void App::app_mem_draw_all(const Kenbak& cpuState) {
void App::app_mem_draw_all(const Kenbak& cpuState, const EeComposer& composer_) {
    if (!update_memory){
        return;
    }else{
        update_memory = false;
    }
    // composer_.MoveTo(kAppMemView.origin);
    composer_.MoveTo(kNwPoint);
  // Example to print w/ locale change
  // composer_.ComposeStringC(kTimeStrings[language]);
  // composer_.stream_->Write(0x20);
  // composer_.stream_->WriteDWord(uptime_ms);

  // int8_t delta = locale_string_delta(kTimeStrings, prev_language, language);
  // if (delta > 0) {
  //   composer_.ClearChars(delta);
  // }
  // composer_.ClearToEndOfLine();

  {  // Write column headings
    composer_.stream_->WriteStringC("    ");
    for (uint8_t col = 0; col < 16; col++) {
      composer_.stream_->WriteChar(' ');
      composer_.stream_->WriteOct(col);
    }
    // composer_.stream_->WriteChar('|');
    composer_.MoveLeft(kAppMemView.width);
    composer_.MoveDown(1);
  }
  for (uint8_t row = 0; row < 16; row++) {

    // write address
    // composer_.stream_->WriteHex(row * 0x10);
    composer_.stream_->WriteOct(row * 0x10);
    composer_.stream_->WriteChar(':');

    // Write the 0xF values
    for (uint8_t col = 0; col < 16; col++) {
      uint8_t value = cpuState.memory_read(row * 0x10 + col);
      // composer_.stream_->WriteChar(' ');
      composer_.MoveRight(1);
      composer_.stream_->WriteOct(value);
    }
    // composer_.stream_->WriteChar('|');

    {  // Moveto next row
      // baseline
      // composer_.stream_->WriteStringC("\x1b[");
      // composer_.stream_->WriteByte(row + 2 + 1 + (kNwPoint.row() - 1));
      // composer_.stream_->WriteChar(';');
      // composer_.stream_->WriteByte(kNwPoint.col());
      // composer_.stream_->WriteChar('H');

      // +8 stack
      // composer_.MoveTo(row + 2 + (kNwPoint.row() - 1), 1);

      // +6 stack
      // composer_.MoveTo(kNwPoint);
      // composer_.MoveDown(row + 2);

      // +4 stack
      composer_.MoveLeft(kAppMemView.width);
      composer_.MoveDown(1);
    }
  }
}

// void App::mem_view_update_addr(const Kenbak& cpuState, uint8_t addr){
void App::mem_view_update_addr(const Kenbak& cpuState, uint8_t addr, const EeComposer& composer_){
//   composer_.MoveTo(kAppMemView.origin);
  composer_.MoveTo(kNePoint);
  composer_.MoveDown(addr / 16 + 1);
  composer_.MoveRight(4 + 4 * (addr & 0xf) + 1);
  composer_.stream_->WriteOct(cpuState.memory_read(addr));
}

// void App::mem_view_cursor_set(uint8_t addr) {
void App::mem_view_cursor_set(uint8_t addr, const EeComposer& composer_) {
//   composer_.MoveTo(kAppMemView.origin);
  composer_.MoveTo(kNePoint);
  composer_.MoveDown(addr / 16 + 1);
  composer_.MoveRight(4 + 4 * (addr & 0xF));
  composer_.stream_->WriteChar('>');
}
void App::mem_view_cursor_clear(uint8_t addr, const EeComposer& composer_) {
//   composer_.MoveTo(kAppMemView.origin);
  composer_.MoveTo(kNePoint);
  composer_.MoveDown(addr / 16 + 1);
  composer_.MoveRight(4 + 4 * (addr & 0xf));
  composer_.stream_->WriteChar(' ');
}
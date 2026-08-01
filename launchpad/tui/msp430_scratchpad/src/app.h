#ifndef APP_H
#define APP_H

#include <stdint.h>
#include "ee_composer.h"
#include "cpu.h"
#include "view.h"

const EePoint kNwPoint = EePoint(1, 3);
// const struct AppView kAppControlsView = {kNwPoint, 3, 1 + 13};
const struct AppView kAppControlsView = {3, 1 + 13};

const EePoint kNePoint = EePoint(kAppControlsView.width + 2 /*col*/, 1 /*row*/);
// const struct AppView kAppMemView = {kNePoint, 16 * 4 + 4, 1 + 16};
const struct AppView kAppMemView = {16 * 4 + 4, 1 + 16};

const EePoint kSwPoint = EePoint(1, kAppMemView.height + 1);
// const struct AppView kAppDecodeView = {kSwPoint, 32, 3};
const struct AppView kAppDecodeView = {32, 3};

class App{
    public:
    // App(const EeComposer& composer) : composer_(composer){
    //     request_update_all();
    // };
    App(){
        request_update_all();
    };

// void app_decode(const Kenbak& cpuState);
void app_decode(const Kenbak& cpuState, const EeComposer& composer_);
void app_controls(const EeComposer& composer_);
void app_mem_draw_all(const Kenbak& cpuState, const EeComposer& composer_);
void mem_view_update_addr(const Kenbak& cpuState, uint8_t addr, const EeComposer& composer_);
void mem_view_cursor_set(uint8_t addr, const EeComposer& composer_);
void mem_view_cursor_clear(uint8_t addr, const EeComposer& composer_);

void request_update_decode(){
    update_decode = true;
}
void request_update_memory(){
    update_memory = true;
}
void request_update_controls(){
    update_controls = true;
}
void request_update_all(){
    update_controls = true;
    update_decode = true;
    update_memory = true;
}
bool needs_redraw(){
    return update_controls || update_decode || update_memory;
}

private:
    // const EeComposer& composer_;
    bool update_decode, update_memory, update_controls;

};



#endif 

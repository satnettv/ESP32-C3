#include "shared_stack.hpp"
#include "esp_expression_with_stack.h"

namespace shared_stack {

uint8_t stack[1024*8];
Mutex mtx;
Mutex mtx_my;

static std::function<void()> call;

static void _call_impl() {
    call();
}

void execute(std::function<void()> arg) {
    auto l = mtx_my.take_lock();
    call = arg;
    esp_execute_shared_stack_function(mtx.get_impl(), stack, sizeof(stack), _call_impl);
}

}

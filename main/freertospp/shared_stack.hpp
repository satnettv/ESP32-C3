#pragma once
#include "Mutex.hpp"
#include <functional>

namespace shared_stack {

extern uint8_t stack[1024*8];
extern Mutex mtx;
extern Mutex mtx_my;
void execute(std::function<void()> arg);

};

#pragma once

#include <iostream>

void printTimestamp();

#define LOG_LINE(level) printTimestamp(); std::cout << " " << __FILE__ << ":" << __LINE__
#define LOG_ERROR LOG_LINE(" ERROR ")
#define LOG_WARN LOG_LINE(" WARN ")
#define LOG_INFO LOG_LINE(" INFO ")
#define LOG_DEBUG LOG_LINE(" DEBUG ")
#define LOG_TRACE LOG_LINE(" TRACE ")

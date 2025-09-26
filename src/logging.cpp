#include "logging.h"

#include <chrono>
#include <iostream>
#include <iomanip>

void printTimestamp()
{
    auto now = std::chrono::system_clock::now();
    std::time_t currentTime_t = std::chrono::system_clock::to_time_t(now);
    std::tm *localTime = std::localtime(&currentTime_t);
    auto duration_since_epoch = now.time_since_epoch();
    auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(duration_since_epoch) % std::chrono::seconds(1);
    std::cout << std::put_time(localTime, "%Y-%m-%d %H:%M:%S")
              << "." << std::setfill('0') << std::setw(6) << microseconds.count();
}
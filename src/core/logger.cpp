#include "../include/logger.h"

// Define static members
std::mutex Logger::logMutex;
LogLevel Logger::minLevel = LogLevel::INFO; 
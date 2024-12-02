#include "Logger.h"
#include<iostream>

Logger& Logger::instance() {
    static Logger logger;
    return logger;
}

void Logger::setLogLevel(int logLevel) {
    logLevel_ = logLevel;
}

void Logger::log(std::string msg) {
    switch (logLevel_)
    {
    case INFO:
        std::cout << "[INFO]";
        break;
    case ERROR:
        std::cout << "[ERROR]";
        break;    
    case FATAL:
        std::cout << "[FATAL]";
        break;
    case DEBUG:
        std::cout << "[DEBUG]";
        break;
    default:
        break;
    }
    std::cout << " " << "time:" /* << time << */<< " " << msg;
}
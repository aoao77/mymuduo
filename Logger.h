#pragma once

#include<string>

enum LogLevel {
    INFO, 
    ERROR,
    FATAL,
    DEBUG,
};

#define LOG_INFO(logMsgFormat, ...) \
do {    \
    Logger& logger = Logger::instance(); \
    logger.setLogLevel(INFO); \
    char buf[1024] = {0}; \
    snprintf(buf, 1024, logMsgFormat, __VA_ARGS__); \
    logger.log(buf); \
} while(0)

#define LOG_ERROR(logMsgFormat, ...) \
do {    \
    Logger& logger = Logger::instance(); \
    logger.setLogLevel(ERROR); \
    char buf[1024] = {0}; \
    snprintf(buf, 1024, logMsgFormat, __VA_ARGS__); \
    logger.log(buf); \
} while(0)

#define LOG_FATAL(logMsgFormat, ...) \
do {    \
    Logger& logger = Logger::instance(); \
    logger.setLogLevel(FATAL); \
    char buf[1024] = {0}; \
    snprintf(buf, 1024, logMsgFormat, __VA_ARGS__); \
    logger.log(buf); \
    exit(-1); \
} while(0)

#define LOG_DEBUG(logMsgFormat, ...) \
do {    \
    Logger& logger = Logger::instance(); \
    logger.setLogLevel(DEBUG); \
    char buf[1024] = {0}; \
    snprintf(buf, 1024, logMsgFormat, __VA_ARGS__); \
    logger.log(buf); \
} while(0)

class Logger {
public:
    static Logger& instance();
    void setLogLevel(int logLevel);
    void log(std::string msg);
private:
    Logger(){}
    int logLevel_;
};
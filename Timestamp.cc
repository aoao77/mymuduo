#include "TimeStamp.h"
#include <time.h>

Timestamp::Timestamp() : microSecondsSinceEpoch_(0) {}

Timestamp::Timestamp(int64_t microSecondsSinceEpoch)
    : microSecondsSinceEpoch_(microSecondsSinceEpoch) {}

Timestamp Timestamp::now() {
    return Timestamp(time(NULL));
}

std::string Timestamp::toString() const{
    const time_t time_value = static_cast<time_t>(microSecondsSinceEpoch_);
    tm* tm_time = localtime(&time_value);
    char buf[128] = {0};
    snprintf(buf, 128, "%4d/%02d/%02d %02d:%02d:%02d",
    tm_time->tm_year + 1900,
    tm_time->tm_mon + 1,
    tm_time->tm_mday,
    tm_time->tm_hour,
    tm_time->tm_min,
    tm_time->tm_sec);
    return buf;
}
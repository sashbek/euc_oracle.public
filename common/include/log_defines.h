#pragma once

#define LOG_ENABLE
#ifdef LOG_ENABLE

#define LOG_INIT() \
    Serial.begin(115200); \
    delay(10)

#define LOG_F(format, ...) \
    Serial.printf(format, ##__VA_ARGS__)

#define LOG_LN(message) \
    Serial.println(message)

#define LOG_(message) \
    Serial.print(message)

#else

#define LOG_INIT()
#define LOG_F(format, ...)
#define LOG_LN(message)
#define LOG_(message)

#endif
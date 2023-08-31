#pragma once

#include "common.h"

class UserTime
{
private:
    /* data */
    uint32_t m_time;
    char *tz_location;
    char ip_address[20];
private:

public:
    UserTime(/* args */);
    ~UserTime();
    void startTasks(void);
    uint32_t getTime(void);
};

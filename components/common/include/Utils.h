#ifndef __USMART_UTILS_H__
#define __USMART_UTILS_H__

#include "common.h"
#include <string>
#include <vector>
using namespace std;
class Utils {
public:
    Utils() = default;
    static string GenID(int);
    static void DelayMs(int);
    static void Delay(int);
    static string GetMac();
    static string GetCurrentIP();
    static string GetCurrentGw();
    static string GetCurrentNetmask();
    static vector<string> Split(string str_in, const char* str_split);
};

#endif //__USMART_UTILS_H__
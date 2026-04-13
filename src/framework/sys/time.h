#ifndef _TIME_H
#define _TIME_H

#include <string>

class Time {
public:
    virtual std::string GetHourMinute() = 0;
    virtual void Update() = 0;

};

#endif //_TIME_H
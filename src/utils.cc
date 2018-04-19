#include "saturn/utils.h"

namespace saturn
{
void Timer::start()
{
    _t_start = clock::now();
    _t_stop = _t_start;
    _running = true;
}

void Timer::stop()
{
    if (_running) {
        _t_stop = clock::now();
        _running = false;
    }
}

long Timer::microseconds() const
{
    if (_running) {
        auto t = clock::now();
        return std::chrono::duration_cast<std::chrono::microseconds>(t - _t_start).count();
    }
    return std::chrono::duration_cast<std::chrono::microseconds>(_t_stop - _t_start).count();
}

double Timer::milliseconds() const
{
    return this->microseconds() / 1000.;
}

double Timer::seconds() const
{
    return this->microseconds() / 1000000.;
}

}    // namespace
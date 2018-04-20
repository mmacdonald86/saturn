#ifndef _SATURN_UTILS_H_
#define _SATURN_UTILS_H_

#include <chrono>

namespace saturn
{
class Timer
{
    using clock = std::chrono::high_resolution_clock;

  public:
    void start();

    void stop();

    long microseconds() const;

    double milliseconds() const;

    double seconds() const;

  private:
    bool _running = false;
    std::chrono::high_resolution_clock::time_point _t_start, _t_stop;
};

}    // namespace
#endif  // include guard
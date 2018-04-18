#ifndef _SATURN_COMMON_H_
#define _SATURN_COMMON_H_

#include <exception>
#include <string>
#include <vector>

namespace saturn
{

using size_t = std::size_t;

class SaturnError : public std::runtime_error {
  public:
    SaturnError(std::string const& msg)
        : std::runtime_error(msg)
    {
    }

    SaturnError(char const* msg)
        : std::runtime_error(msg)
    {
    }
};

enum class Status {
    ok = 0,
    warning = 10,
    error = 20,
};


}  // namespace
#endif  // include guard
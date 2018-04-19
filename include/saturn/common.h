#ifndef _SATURN_COMMON_H_
#define _SATURN_COMMON_H_

#include <stdexcept>
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

}  // namespace
#endif  // include guard
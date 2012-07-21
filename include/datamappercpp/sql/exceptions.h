#ifndef DATAMAPPERCPP_EXCEPTIONS_H__
#define DATAMAPPERCPP_EXCEPTIONS_H__

#include <string>
#include <stdexcept>

namespace dm {
namespace sql {

class MoreThanOneError : public std::runtime_error
{
public:
    MoreThanOneError(const std::string& msg)
        : std::runtime_error(msg)
    { }
};

} }

#endif /* EXCEPTIONS_H */

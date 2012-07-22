#ifndef DATAMAPPERCPP_EXCEPTIONS_H__
#define DATAMAPPERCPP_EXCEPTIONS_H__

#include <string>
#include <stdexcept>

namespace dm {
namespace sql {

class ErrorBase : public std::runtime_error
{
public:
    ErrorBase(const std::string& msg) :
        std::runtime_error(msg)
    { }

    virtual ~ErrorBase() throw()
    { }
};

class NotOneError : public ErrorBase
{
public:
    NotOneError(const std::string& msg) :
        ErrorBase(msg)
    { }
};

class DoesNotExistError : public ErrorBase
{
public:
    DoesNotExistError(const std::string& msg) :
        ErrorBase(msg)
    { }
};

} }

#endif /* EXCEPTIONS_H */

#ifndef DATAMAPPERCPP_FIELD_H__
#define DATAMAPPERCPP_FIELD_H__

#include <string>
#include <sstream>

#include <utilcpp/release_assert.h>

namespace dm
{

template <typename T>
struct Field
{
    const std::string& label;
    const std::string& options;

    Field(const std::string& l,
          const std::string& o = std::string()) :
        label(l), options(o)
    { }

    std::string typeDefinition() const
    {
        std::ostringstream ret;

        ret << getType();
        if (!options.empty())
            ret << " " << options;

        return ret.str();
    }

private:
    std::string getType() const
    {
        UTILCPP_RELEASE_ASSERT(false, "Unknown field type");
        return ""; // unreachable
    }
};

// TODO: note that this is specific for the SQL aspect
// Also, types are SQLite-specific.
template <>
std::string Field<int>::getType() const { return "INT"; }

// TODO: add CHECK( in { 0, 1 } )
template <>
std::string Field<bool>::getType() const { return "INT"; }

template <>
std::string Field<double>::getType() const { return "REAL"; }

template <>
std::string Field<const char*>::getType() const { return "TEXT"; }

template <>
std::string Field<std::string>::getType() const { return "TEXT"; }

}

#endif /* DATAMAPPERCPP_FIELD_H */

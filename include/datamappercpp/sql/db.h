#ifndef DATAMAPPERCPP_DB_H__
#define DATAMAPPERCPP_DB_H__

#include <dbccpp/dbccpp.h>

#include <string>

namespace dm {
namespace sql {

typedef dbc::PreparedStatement::ptr Statement;

void ConnectDatabase(const std::string& dbFileName)
{
    dbc::DbConnection::connect("sqlite", dbFileName);
}

void ExecuteStatement(const std::string& sql)
{
    dbc::DbConnection::instance().executeUpdate(sql);
}

Statement PrepareStatement(const std::string& sql)
{
    return dbc::DbConnection::instance().prepareStatement(sql);
}

} }

#endif /* DATAMAPPERCPP_DB_H */

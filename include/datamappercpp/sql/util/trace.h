#ifndef INSTALL_TRACE_H__
#define INSTALL_TRACE_H__

#include "../../../../lib/dbccpp/src/sqlite/SQLiteConnection.h"
#include <sqlite3.h>
#include <iostream>

namespace dm {
namespace sql {

static void trace(void*, const char* sql)
{
    std::cout << "SQL: " << sql << std::endl;
}

static void TraceSqlToStderr()
{
    dbc::DbConnection& db = dbc::DbConnection::instance();
    dbc::SQLiteConnection& sqlitedb = dynamic_cast<dbc::SQLiteConnection&>(db);
    sqlite3_trace(sqlitedb.handle(), trace, 0);
}

}
}

#endif /* INSTALL_TRACE_H */

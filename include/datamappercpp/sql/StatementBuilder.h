#ifndef DATAMAPPERCPP_SQLBUILDER_H__
#define DATAMAPPERCPP_SQLBUILDER_H__

#include <datamappercpp/Field.h>
#include <datamappercpp/sql/detail/FieldVisitors.h>

#include <utilcpp/release_assert.h>
#include <utilcpp/disable_copy.h>

#include <string>
#include <sstream>

// FIXME: ostringstream::exceptions(s.badbit | s.failbit);

namespace dm {
namespace sql {

template <class Mapping>
class StatementBuilder
{
public:
    static std::string CreateTableStatement()
    {
        std::ostringstream sql;

        // assume IF NOT EXISTS is useful
        sql << "CREATE TABLE IF NOT EXISTS " << Mapping::getLabel()
            // assume all entities have surrogate keys named 'id'
            << "(id INTEGER PRIMARY KEY AUTOINCREMENT,";

        FieldDeclarationBuilder fieldBuilder(sql);
        Mapping::accept(fieldBuilder);

        // replace last comma with ')'
        long pos = sql.tellp();
        sql.seekp(pos - 1);
        sql << ")";

        std::string customStatements(Mapping::customCreateStatements());
        if (!customStatements.empty())
            sql << ";" << customStatements;

        return sql.str();
    }

    static std::string InsertStatement()
    {
        std::ostringstream sql;

        sql << "INSERT INTO " << Mapping::getLabel() << " ";

        std::ostringstream columnLabels;
        std::ostringstream fieldPlaceholders;

        InsertStatementFieldBuilder fieldBuilder(columnLabels,
                                                 fieldPlaceholders);
        Mapping::accept(fieldBuilder);

        std::string s = columnLabels.str();
        s.erase(s.end() - 1); // remove last comma

        sql << "(" << s << ")";

        s = fieldPlaceholders.str();
        s.erase(s.end() - 1);

        sql << " VALUES (" << s << ")";

        return sql.str();
    }

    static std::string UpdateStatement()
    {
        std::ostringstream sql;

        sql << "UPDATE " << Mapping::getLabel() << " SET ";

        UpdateStatementFieldBuilder fieldBuilder(sql);
        Mapping::accept(fieldBuilder);

        long pos = sql.tellp();
        sql.seekp(pos - 1); // remove last comma

        sql << " WHERE id=?";

        return sql.str();
    }

    static std::string DeleteAllStatement()
    {
        std::ostringstream sql;

        sql << "DELETE FROM " << Mapping::getLabel();

        return sql.str();
    }

    static std::string DeleteByIdStatement()
    {
        std::ostringstream sql;

        sql << "DELETE FROM " << Mapping::getLabel()
            << " WHERE id=?";

        return sql.str();
    }

    static std::string SelectAllStatement()
    {
        std::ostringstream sql;

        sql << "SELECT * FROM " << Mapping::getLabel();

        return sql.str();
    }

    static std::string SelectByIdStatement()
    {
        return SelectByFieldStatement("id");
    }

    static std::string SelectByFieldStatement(const std::string field)
    {
        std::ostringstream sql;

        sql << "SELECT * FROM " << Mapping::getLabel()
            << " WHERE " << field << "=?";

        return sql.str();
    }

private:
    // disable instantiation to assure the class is only used via it's static
    // functions
    StatementBuilder();

};

} }

#endif /* DATAMAPPERCPP_SQLBUILDER_H__ */

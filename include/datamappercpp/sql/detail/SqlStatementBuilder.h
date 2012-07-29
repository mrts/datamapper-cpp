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

template <class Entity, class Mapping>
class SqlStatementBuilder
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
        Mapping::accept(fieldBuilder, _dummy_entity);

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
        Mapping::accept(fieldBuilder, _dummy_entity);

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
        Mapping::accept(fieldBuilder, _dummy_entity);

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
    SqlStatementBuilder();

    static Entity _dummy_entity;
};

template <class Entity, class Mapping>
Entity SqlStatementBuilder<Entity, Mapping>::_dummy_entity;

} }

#endif /* DATAMAPPERCPP_SQLBUILDER_H__ */

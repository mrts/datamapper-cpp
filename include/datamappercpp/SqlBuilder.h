#include <utilcpp/release_assert.h>
#include <utilcpp/disable_copy.h>

#include <string>
#include <sstream>


// 1
// FIXME: ostringstream::exceptions(s.badbit | s.failbit);

// 2
// namespace dm {
// namespace sql <- mark it as internal

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

// 3
/** Field visitors. */
class FieldDeclarationBuilder
{
    UTILCPP_DISABLE_COPY(FieldDeclarationBuilder)

public:
    FieldDeclarationBuilder(std::ostringstream& out) :
        _out(out)
    { }

    template <typename T>
    void addField(const Field<T>& field)
    {
        _out << field.label << " " << field.typeDefinition() << ",";
    }

private:
    std::ostringstream& _out;
};

class InsertStatementFieldBuilder
{
    UTILCPP_DISABLE_COPY(InsertStatementFieldBuilder)

public:
    InsertStatementFieldBuilder(std::ostringstream& columnLabels,
                                std::ostringstream& fieldPlaceholders) :
        _labels(columnLabels),
        _placeholders(fieldPlaceholders)
    { }

    template <typename T>
    void addField(const Field<T>& field)
    {
        _labels << field.label << ",";
        _placeholders << "?,";
    }

private:
    std::ostringstream& _labels;
    std::ostringstream& _placeholders;
};

class UpdateStatementFieldBuilder
{
    UTILCPP_DISABLE_COPY(UpdateStatementFieldBuilder)

public:
    UpdateStatementFieldBuilder(std::ostringstream& out) :
        _out(out)
    { }

    template <typename T>
    void addField(const Field<T>& field)
    {
        _out << field.label << "=?,";
    }

private:
    std::ostringstream& _out;
};

// 4
template <class Mapping>
class StatementBuilder
{
    // TODO: mark the class static
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

    static std::string LoadByIdStatement()
    {
        return LoadByFieldStatement("id");
    }

    static std::string LoadByFieldStatement(const std::string field)
    {
        std::ostringstream sql;

        sql << "SELECT * FROM " << Mapping::getLabel()
            << " WHERE " << field << "=?";

        return sql.str();
    }

};

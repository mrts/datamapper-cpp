#include <datamappercpp/Field.h>

#include <utilcpp/disable_copy.h>

#include <sstream>

namespace dm {
namespace sql {

class FieldDeclarationBuilder
{
    UTILCPP_DISABLE_COPY(FieldDeclarationBuilder)

public:
    FieldDeclarationBuilder(std::ostringstream& out) :
        _out(out)
    { }

    template <typename T>
    void visitField(const Field<T>& field, const T& )
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
    void visitField(const Field<T>& field, const T& )
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
    void visitField(const Field<T>& field, const T& )
    {
        _out << field.label << "=?,";
    }

private:
    std::ostringstream& _out;
};

} }

#include <datamappercpp/SqlBuilder.h>

#include <testcpp/testcpp.h>
#include <testcpp/StdOutView.h>

#include <iostream>

class PersonMapper
{
public:
    static std::string getLabel()
    { return "person"; }

    template <class Visitor>
    static void accept(Visitor& visitor)
    {
        visitor.addField(Field<std::string>("name", "UNIQUE NOT NULL"));
        visitor.addField(Field<int>("age"));
    }

    static std::string customCreateStatements()
    {
        return "CREATE INDEX IF NOT EXISTS person_name_idx "
            "ON person (name)";
    }
};

class TestDataMapperCpp : public Test::Suite
{
public:
    typedef void (TestDataMapperCpp::*TestMethod)();

    void test()
    {
        typedef StatementBuilder<PersonMapper> PersonSQL;

        Test::assertEqual<std::string>(
                "Create table statement is correct",
                PersonSQL::CreateTableStatement(),
                "CREATE TABLE IF NOT EXISTS person"
                "(id INTEGER PRIMARY KEY AUTOINCREMENT,"
                "name TEXT UNIQUE NOT NULL,"
                "age INT);"
                "CREATE INDEX IF NOT EXISTS person_name_idx ON person (name)");


        Test::assertEqual<std::string>(
                "Insert statement is correct",
                PersonSQL::InsertStatement(),
                "INSERT INTO person (name,age) VALUES (?,?)");

        Test::assertEqual<std::string>(
                "Update statement is correct",
                PersonSQL::UpdateStatement(),
                "UPDATE person SET name=?,age=? WHERE id=?");

        Test::assertEqual<std::string>(
                "Load by ID statement is correct",
                PersonSQL::LoadByIdStatement(),
                "SELECT * FROM person WHERE id=?");

        Test::assertEqual<std::string>(
                "Load by field statement is correct",
                PersonSQL::LoadByFieldStatement("age"),
                "SELECT * FROM person WHERE age=?");
    }
};

int main()
{
    Test::Controller &c = Test::Controller::instance();
    c.setObserver(Test::observer_transferable_ptr(new Test::ColoredStdOutView));
    c.addTestSuite("datamapper-cpp", Test::Suite::instance<TestDataMapperCpp>);

    return c.run();
}

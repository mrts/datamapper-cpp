#include <datamappercpp/sql/StatementBuilder.h>

#include <testcpp/testcpp.h>
#include <testcpp/StdOutView.h>

#include <iostream>

struct Person
{
    std::string name;
    int age;
};

class PersonMapping
{
public:
    static std::string getLabel()
    { return "person"; }

    template <class Visitor>
    static void accept(Visitor& visitor)
    {
        // or visitor << Field(...) ?
        visitor.addField(dm::Field<std::string>("name", "UNIQUE NOT NULL"));
        visitor.addField(dm::Field<int>("age"));
    }

    template <class Visitor>
    static void accept(Visitor& visitor, Person& p)
    {
        visitor.accessField("name", p.name);
        visitor.accessField("age",  p.age);
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
        testSqlStatementBuilding();
        // testSingleObjectLoading();
    }

    void testSqlStatementBuilding()
    {
        typedef dm::sql::StatementBuilder<PersonMapping> PersonSQL;

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

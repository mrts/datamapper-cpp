#include <datamappercpp/sql/StatementBuilder.h>
#include <datamappercpp/sql/Repository.h>
#include <datamappercpp/sql/db.h>

#include <testcpp/testcpp.h>
#include <testcpp/StdOutView.h>

#include <iostream>

/* Include <datamappercpp/sql/util/trace.h>
 * and call dm::sql::TraceSqlToStderr();
 * if you want to trace SQL statements as SQLite sees them.
 */

struct Person
{
    int id;
    std::string name;
    int age;
    double height;
};

class PersonMapping
{
public:
    static std::string getLabel()
    { return "person"; }

    template <class Visitor>
    static void accept(Visitor& visitor)
    {
        visitor.addField(dm::Field<std::string>("name", "UNIQUE NOT NULL"));
        visitor.addField(dm::Field<int>("age"));
        visitor.addField(dm::Field<double>("height"));
    }

    template <class Visitor>
    static void accept(Visitor& visitor, Person& p)
    {
        // NOTE THAT FIELD ORDER MUST CURRENTLY BE THE SAME AS IN addField
        // see note in ObjectFieldBinder::accessField() for improvement
        visitor.accessField(p.name);
        visitor.accessField(p.age);
        visitor.accessField(p.height);
    }

    static std::string customCreateStatements()
    {
        return "CREATE INDEX IF NOT EXISTS person_name_idx "
            "ON person (name)";
    }
};

class PersonRepository : public dm::sql::Repository<Person, PersonMapping>
{ };

class TestDataMapperCpp : public Test::Suite
{
public:
    typedef void (TestDataMapperCpp::*TestMethod)();

    TestDataMapperCpp()
    {
        dm::sql::ConnectDatabase("test.sqlite");
    }

    ~TestDataMapperCpp()
    {
        dm::sql::ExecuteStatement(std::string("DROP TABLE IF EXISTS ")
                + PersonMapping::getLabel());
    }

    void test()
    {
        testSqlStatementBuilding();
        testObjectSaving();
        testSingleObjectLoading();
        // TODO: test transactions
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
                "age INT,"
                "height REAL);"
                "CREATE INDEX IF NOT EXISTS person_name_idx ON person (name)");


        Test::assertEqual<std::string>(
                "Insert statement is correct",
                PersonSQL::InsertStatement(),
                "INSERT INTO person (name,age,height) VALUES (?,?,?)");

        Test::assertEqual<std::string>(
                "Update statement is correct",
                PersonSQL::UpdateStatement(),
                "UPDATE person SET name=?,age=?,height=? WHERE id=?");

        Test::assertEqual<std::string>(
                "Load by ID statement is correct",
                PersonSQL::LoadByIdStatement(),
                "SELECT * FROM person WHERE id=?");

        Test::assertEqual<std::string>(
                "Load by field statement is correct",
                PersonSQL::LoadByFieldStatement("age"),
                "SELECT * FROM person WHERE age=?");
    }

    void testObjectSaving()
    {
        PersonRepository::CreateTable();

        Person p = { -1, "Ervin", 38, 1.80 };
        PersonRepository::Save(p);
        Test::assertTrue(
                "Object ID is correctly set during saving",
                p.id == 1);

        std::vector<Person> ps;

        Person m = { -1, "Marvin", 24, 1.65 };
        ps.push_back(m);

        Person s = { -1, "Steve",  32, 2.10 };
        ps.push_back(s);

        PersonRepository::Save(ps);
        Test::assertTrue(
                "Multiple objects are correctly saved",
                ps[0].id == 2 && ps[1].id == 3);
    }

    void testSingleObjectLoading()
    {
        Person p = PersonRepository::Get(1);
        Test::assertTrue("Loading objects by ID works",
                p.name == "Ervin" && p.age == 38 && p.height == 1.80);

        p = PersonRepository::GetByField("name", "Marvin");
        Test::assertTrue("Loading objects by field works",
                p.name == "Marvin" && p.age == 24 && p.height == 1.65);

        p = PersonRepository::GetByQuery("SELECT * FROM person "
                "WHERE name LIKE '%eve'");
        Test::assertTrue("Loading objects by query works",
                p.name == "Steve" && p.age == 32 && p.height == 2.10);

        dm::sql::Statement statement = dm::sql::PrepareStatement(
                "SELECT * FROM person WHERE name LIKE '%vin'");
        p = PersonRepository::GetByQuery(statement, true);
        Test::assertTrue(
                "More than one result causes the first one to be returned "
                "if allow_many is true",
                p.name == "Ervin" && p.age == 38 && p.height == 1.80);

        Test::assertThrows<TestDataMapperCpp, TestMethod,
                           dm::sql::MoreThanOneError>(
                "More than one result causes MoreThanOneError exception "
                "if allow_many is false",
                *this, &TestDataMapperCpp::getPersonByQuery_ReturnsMany);
    }

    void getPersonByQuery_ReturnsMany()
    {
        PersonRepository::GetByQuery(
                "SELECT * FROM person WHERE name LIKE '%vin'");
    }

    /*
    void testUpdate()
    {
        p.age = 12;
        // would need a mock/fake here that would intercept and assure
        // that update was called instead of insert
        PersonRepository::Save(p);
    }
    */


};

int main()
{
    Test::Controller &c = Test::Controller::instance();
    c.setObserver(Test::observer_transferable_ptr(new Test::ColoredStdOutView));
    c.addTestSuite("datamapper-cpp", Test::Suite::instance<TestDataMapperCpp>);

    return c.run();
}

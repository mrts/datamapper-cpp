#include <datamappercpp/sql/Repository.h>
#include <datamappercpp/sql/db.h>

#include <datamappercpp/sql/detail/SqlStatementBuilder.h>

#include <utilcpp/disable_copy.h>

#include <testcpp/testcpp.h>
#include <testcpp/StdOutView.h>

#include <iostream>
#include <functional>

/* Include <datamappercpp/sql/util/trace.h>
 * and call dm::sql::TraceSqlToStderr();
 * if you want to trace SQL statements as SQLite sees them.
 */

struct Person
{
    typedef std::vector<Person> list;

    int id;
    std::string name;
    int age;
    double height;

    Person() :
        id(-1), name(), age(0), height(0.0)
    { }

    Person(int i, const std::string& n, int a, double h) :
        id(i), name(n), age(a), height(h)
    { }

    bool operator==(const Person& rhs) const
    {
        return id == rhs.id && name == rhs.name
               && age == rhs.age && height == rhs.height;
    }

    operator std::string() const
    {
        std::ostringstream out;
        out << "Person: {"
            << id << ","
            << name << ","
            << age << ","
            << height << "}";
        return out.str();
    }

};

void print_person(const Person& p)
{ std::cout << static_cast<std::string>(p) << std::endl; }

class PersonMapping
{
public:
    static std::string getLabel()
    { return "person"; }

    template <class Visitor>
    static void accept(Visitor& visitor, Person& p)
    {
        // Note that field order is important
        visitor.accessField(dm::Field<std::string>("name", "UNIQUE NOT NULL"),
                            p.name);
        visitor.accessField(dm::Field<int>("age"),
                            p.age);
        visitor.accessField(dm::Field<double>("height"),
                            p.height);
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
        testMultipleObjectLoading();
        testObjectDeletion();
        // TODO: test transactions
    }

    void testSqlStatementBuilding()
    {
        typedef dm::sql::SqlStatementBuilder<Person, PersonMapping> PersonSql;

        Test::assertEqual<std::string>(
                "Create table statement is correct",
                PersonSql::CreateTableStatement(),
                "CREATE TABLE IF NOT EXISTS person"
                "(id INTEGER PRIMARY KEY AUTOINCREMENT,"
                "name TEXT UNIQUE NOT NULL,"
                "age INT,"
                "height REAL);"
                "CREATE INDEX IF NOT EXISTS person_name_idx ON person (name)");

        Test::assertEqual<std::string>(
                "Insert statement is correct",
                PersonSql::InsertStatement(),
                "INSERT INTO person (name,age,height) VALUES (?,?,?)");

        Test::assertEqual<std::string>(
                "Update statement is correct",
                PersonSql::UpdateStatement(),
                "UPDATE person SET name=?,age=?,height=? WHERE id=?");

        Test::assertEqual<std::string>(
                "Select by ID statement is correct",
                PersonSql::SelectByIdStatement(),
                "SELECT * FROM person WHERE id=?");

        Test::assertEqual<std::string>(
                "Select by field statement is correct",
                PersonSql::SelectByFieldStatement("age"),
                "SELECT * FROM person WHERE age=?");
    }

    void testObjectSaving()
    {
        PersonRepository::CreateTable();

        Person p(-1, "Ervin", 38, 1.80);
        PersonRepository::Save(p);
        Test::assertTrue(
                "Object ID is correctly set during saving",
                p.id == 1);

        Person::list ps;

        ps.push_back(Person(-1, "Marvin", 24, 1.65));
        ps.push_back(Person(-1, "Steve",  32, 2.10));

        PersonRepository::Save(ps);
        Test::assertTrue(
                "Multiple objects are correctly saved",
                ps[0].id == 2 && ps[1].id == 3);
    }

    void testSingleObjectLoading()
    {
        Person p = PersonRepository::Get(1);
        Test::assertTrue("Get objects by ID works",
                p.name == "Ervin" && p.age == 38 && p.height == 1.80);

        p = PersonRepository::GetByField("name", "Marvin");
        Test::assertTrue("Get objects by field works",
                p.name == "Marvin" && p.age == 24 && p.height == 1.65);

        p = PersonRepository::GetByQuery("SELECT * FROM person "
                "WHERE name LIKE '%eve'");
        Test::assertTrue("Get objects by query works",
                p.name == "Steve" && p.age == 32 && p.height == 2.10);

        dm::sql::Statement statement = dm::sql::PrepareStatement(
                "SELECT * FROM person WHERE name LIKE '%vin'");
        p = PersonRepository::GetByQuery(statement, true);
        Test::assertTrue(
                "More than one result causes the first one to be returned "
                "if allow_many is true",
                p.name == "Ervin" && p.age == 38 && p.height == 1.80);

        Test::assertThrows<TestDataMapperCpp, TestMethod,
                           dm::sql::NotOneError>(
                "More than one result causes NotOneError exception "
                "if allow_many is false",
                *this, &TestDataMapperCpp::ifManyResults_ThenThrowsNotOneError);

        Test::assertThrows<TestDataMapperCpp, TestMethod,
                           dm::sql::DoesNotExistError>(
                "No results for given criteria causes "
                "DoesNotExistError exception",
                *this,
                &TestDataMapperCpp::ifDoesNotExist_ThenThrowsDoesNotExistError);
    }

    void testMultipleObjectLoading()
    {
        Person::list expected;
        expected.push_back(Person(1, "Ervin",  38, 1.80));
        expected.push_back(Person(2, "Marvin", 24, 1.65));
        expected.push_back(Person(3, "Steve",  32, 2.10));

        Person::list ps = PersonRepository::GetAll();

        Test::assertEqual<Person::list>("Get all objects works",
                ps, expected);

        expected[1].age = 32; // Marvin was lying about his age!
        PersonRepository::Save(expected[1]);

        ps = PersonRepository::GetManyByField("age", 32);
        expected.erase(expected.begin()); // remove Ervin, who's still 38

        Test::assertEqual<Person::list>("Get many objects by field works",
                ps, expected);

        expected.clear();
        expected.push_back(Person(1, "Ervin",  38, 1.80));
        expected.push_back(Person(2, "Marvin", 32, 1.65));

        ps = PersonRepository::GetManyByQuery("SELECT * FROM person "
                "WHERE name LIKE '%vin'");
        Test::assertEqual<Person::list>("Get many objects by query works",
                ps, expected);

        dm::sql::Statement statement = dm::sql::PrepareStatement(
                "SELECT * FROM person WHERE name LIKE '%vin'");
        ps = PersonRepository::GetManyByQuery(statement);
        Test::assertEqual<Person::list>("Get many objects by statement works",
                ps, expected);

        ps = PersonRepository::GetManyByField("age", 100);
        expected.clear();

        Test::assertEqual<Person::list>(
                "No results for given criteria returns empty list",
                ps, expected);
    }

    void testObjectUpdating()
    {

    }

    void testObjectDeletion()
    {
        PersonRepository::Delete(1); // Ervin is gone

        Person marvin(2, "Marvin", 24, 1.65);
        PersonRepository::Delete(marvin); // Marvin is gone too

        Test::assertTrue(
                "Object ID is invalidated during deletion",
                marvin.id == -1);

        Test::assertThrows<TestDataMapperCpp, TestMethod,
                           dm::sql::DoesNotExistError>(
                "Getting deleted objects causes DoesNotExistError exception",
                *this,
                &TestDataMapperCpp::ifIsDeleted_ThenThrowsDoesNotExistError);

        PersonRepository::DeleteAll();

        Person::list ps = PersonRepository::GetAll();
        Person::list expected;

        Test::assertEqual<Person::list>("Repository is empty after all "
                "objects have been deleted",
                ps, expected);
    }

    void ifDoesNotExist_ThenThrowsDoesNotExistError()
    {
        PersonRepository::Get(42);
    }

    void ifManyResults_ThenThrowsNotOneError()
    {
        PersonRepository::GetByQuery(
                "SELECT * FROM person WHERE name LIKE '%vin'");
    }

    void ifIsDeleted_ThenThrowsDoesNotExistError()
    {
        PersonRepository::Get(1);
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

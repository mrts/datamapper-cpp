#include <datamappercpp/sql/StatementBuilder.h>
#include <datamappercpp/sql/Transaction.h>
#include <datamappercpp/sql/db.h>
#include <datamappercpp/sql/exceptions.h>

#include <dbccpp/dbccpp.h>

#include <utilcpp/disable_copy.h>

#include <vector>

#if defined(__GXX_EXPERIMENTAL_CXX0X__) || (__cplusplus > 199711L)
  #include <functional>
  namespace dm
  {
      namespace stdutil = std;
  }
#else
  #include <boost/function.hpp>
  namespace dm
  {
      namespace stdutil = boost;
  }
#endif

namespace dm {
namespace sql {

class StatementFieldBinder
{
    UTILCPP_DISABLE_COPY(StatementFieldBinder)

public:
    StatementFieldBinder(dbc::PreparedStatement::ptr& statement) :
        _statement(statement)
    { }

    template <class FieldType>
    void accessField(FieldType& field)
    {
        // TODO: *_statement[label] = field
        *_statement << field;
    }

private:
    dbc::PreparedStatement::ptr& _statement;
};

class ObjectFieldBinder
{
    UTILCPP_DISABLE_COPY(ObjectFieldBinder)

public:
    ObjectFieldBinder(dbc::ResultSet::ptr& result) :
        _result(result),
        // index is 0-based, skip id, which is already set
        _counter(1)
    {}

    template <class FieldType>
    void accessField(FieldType& field)
    {
        // TODO: field = *_result[label]
        // and field << *_result;
        field = _result->get<FieldType>(_counter++);
    }

private:
    dbc::ResultSet::ptr& _result;
    unsigned int _counter;
};

/**
 * Repositories transfer domain entities and their collections to and from
 * database.
 *
 * Relies on Return Value Optimization and returns entities and collections by
 * copy.
 */
template <class Entity, class Mapping>
class Repository
{
public:
    static void CreateTable(bool enableTransaction = true)
    {
        Transaction transaction(enableTransaction);

        ExecuteStatement(StatementBuilder<Mapping>::CreateTableStatement());

        transaction.commit();
    }

    static void Save(std::vector<Entity>& entities,
                     bool enableTransaction = true)
    {
        Transaction transaction(enableTransaction);

        for (size_t i = 0; i < entities.size(); ++i)
            Save(entities[i], false);

        transaction.commit();
    }

    static void Save(Entity& entity, bool enableTransaction = true)
    {
        dbc::PreparedStatement::ptr statement;
        bool update = entity.id > 0;

        if (update)
        {
            prepareStatement(_updateEntityStatement,
                             &StatementBuilder<Mapping>::UpdateStatement);
            statement = _updateEntityStatement;
        }
        else
        {
            prepareStatement(_insertEntityStatement,
                             &StatementBuilder<Mapping>::InsertStatement);
            statement = _insertEntityStatement;
        }

        StatementFieldBinder fieldbinder(statement);
        Mapping::accept(fieldbinder, entity);

        if (update)
            // update needs to to have ID bound as well
            *statement << entity.id;

        Transaction transaction(enableTransaction);

        int howmany = statement->executeUpdate();
        if (howmany != 1)
        {
            std::ostringstream msg;
            msg << howmany << " rows affected during save instead of 1";
            throw MoreThanOneError(msg.str());
        }

        if (!update)
            // insert needs to set the object id after insert
            entity.id = statement->getLastInsertId();

        transaction.commit();
    }

    static Entity Get(int id)
    {
        // note that SELECTs are outside transactions and
        // mixing them with ongoing transactions may cause problems

        if (id < 1)
            throw std::invalid_argument("ID is less than 1");

        prepareStatement(_getEntityByIdStatement,
                         &StatementBuilder<Mapping>::LoadByIdStatement);
        *_getEntityByIdStatement << id;

        return GetByQueryImpl(_getEntityByIdStatement, false, id);
    }

    // TODO: note that value comes by copy, use enable_if
    template <typename Value>
    static Entity GetByField(const std::string& fieldname, Value value,
            bool allowMany = false)
    {
        Statement statement = PrepareStatement(
                StatementBuilder<Mapping>::LoadByFieldStatement(fieldname));
        *statement << value;
        return GetByQueryImpl(statement, allowMany, -1);
    }

    static Entity GetByQuery(const std::string& sql,
            bool allowMany = false)
    {
        Statement statement = PrepareStatement(sql);
        return GetByQueryImpl(statement, allowMany, -1);
    }

    static Entity GetByQuery(Statement& statement,
            bool allowMany = false)
    {
        return GetByQueryImpl(statement, allowMany, -1);
    }

    static void ResetStatements()
    {
        _insertEntityStatement.reset();
        _updateEntityStatement.reset();
        _getEntityByIdStatement.reset();
    }

private:
    Repository();

    static Statement _insertEntityStatement;
    static Statement _updateEntityStatement;
    static Statement _getEntityByIdStatement;

    inline static void prepareStatement(Statement& statement,
            stdutil::function<std::string (void)> createSqlStatement)
    {
        if (!statement)
        {
            statement = PrepareStatement(createSqlStatement());
        }
        else
        {
            statement->reset();
            statement->clear();
        }
    }

    inline static Entity GetByQueryImpl(Statement& statement,
            bool allowMany, int id)
    {
        Entity entity;

        dbc::ResultSet::ptr result(statement->executeQuery());
        result->next();

        // TODO: entity.id << *result;
        entity.id = result->get<int>(0);

        ObjectFieldBinder fieldbinder(result);
        Mapping::accept(fieldbinder, entity);

        if (!allowMany && result->next())
        {
            std::ostringstream msg;
            msg << "More than one result for ";
            if (id > 0)
                msg << "ID " << id;
            else
                msg << "query '" << statement->getSQL() << "'";
            throw MoreThanOneError(msg.str());
        }

        return entity;
    }

    /*

    vector<Entity> GetAll();
    vector<Entity> GetManyByQuery(PreparedStatement q);
    template <typename Value>
    vector<Entity> GetManyByField(string fieldname, Value value);

    void Delete(Entity e);
    */
};

template <class Entity, class Mapping>
dbc::PreparedStatement::ptr Repository<Entity, Mapping>::_insertEntityStatement;

template <class Entity, class Mapping>
dbc::PreparedStatement::ptr Repository<Entity, Mapping>::_updateEntityStatement;

template <class Entity, class Mapping>
dbc::PreparedStatement::ptr Repository<Entity, Mapping>::_getEntityByIdStatement;

} }

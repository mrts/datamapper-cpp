#include <datamappercpp/sql/Transaction.h>
#include <datamappercpp/sql/db.h>
#include <datamappercpp/sql/exceptions.h>

#include <datamappercpp/sql/detail/SqlStatementBuilder.h>

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

    template <typename T>
    void accessField(const Field<T>& , const T& field)
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
    ObjectFieldBinder(const dbc::ResultSet& result) :
        _result(result),
        // index is 0-based, skip id, which is already set
        _counter(1)
    {}

    template <typename T>
    void accessField(const Field<T>& , T& field)
    {
        // TODO: field = *_result[label]
        field = _result.get<T>(_counter++);
    }

private:
    const dbc::ResultSet& _result;
    unsigned int _counter;
};

/**
 * Repository transfers domain entities and their collections to and from
 * the database.
 *
 * Relies on Return Value Optimization and returns entities and collections by
 * copy.
 */
template <class Entity, class Mapping>
class Repository
{
public:
    typedef std::vector<Entity> Entities;
    typedef SqlStatementBuilder<Entity, Mapping> EntitySqlBuilder;

    static void CreateTable(bool enableTransaction = true)
    {
        Transaction transaction(enableTransaction);

        ExecuteStatement(EntitySqlBuilder::CreateTableStatement());

        transaction.commit();
    }

    static void Save(Entities& entities,
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
                             &EntitySqlBuilder::UpdateStatement);
            statement = _updateEntityStatement;
        }
        else
        {
            prepareStatement(_insertEntityStatement,
                             &EntitySqlBuilder::InsertStatement);
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
            msg << howmany << " rows affected while saving single entity "
                << "instead of 1";
            throw NotOneError(msg.str());
        }

        if (!update)
            // insert needs to set the object id after insert
            entity.id = statement->getLastInsertId();

        transaction.commit();
    }

    static void Delete(Entity& entity,
                bool enableTransaction = true,
                bool checkOneDeleted = true)
    {
        if (entity.id < 1)
            throw std::invalid_argument("Entity ID is less than 1");

        Delete(entity.id, enableTransaction, checkOneDeleted);

        entity.id = -1;
    }

    static void Delete(int id,
                bool enableTransaction = true,
                bool checkOneDeleted = true)
    {
        prepareStatement(_deleteEntityStatement,
                         &EntitySqlBuilder::DeleteByIdStatement);
        *_deleteEntityStatement << id;

        Transaction transaction(enableTransaction);

        int howmany = _deleteEntityStatement->executeUpdate();
        if (checkOneDeleted && howmany != 1)
        {
            std::ostringstream msg;
            msg << howmany << " rows affected while deleting single entity "
                << "instead of 1";
            throw NotOneError(msg.str());
        }

        transaction.commit();
    }

    static void DeleteAll(bool enableTransaction = true)
    {
        Transaction transaction(enableTransaction);

        ExecuteStatement(EntitySqlBuilder::DeleteAllStatement());

        transaction.commit();
    }

    static Entity Get(int id)
    {
        // note that SELECTs are outside transactions and
        // mixing them with ongoing transactions may cause problems

        if (id < 1)
            throw std::invalid_argument("ID is less than 1");

        prepareStatement(_getEntityByIdStatement,
                         &EntitySqlBuilder::SelectByIdStatement);
        *_getEntityByIdStatement << id;

        return GetByQueryImpl(_getEntityByIdStatement, false, id);
    }

    template <typename Value>
    static Entity GetByField(const std::string& fieldname, const Value& value,
            bool allowMany = false)
    {
        Statement statement = PrepareStatement(
                EntitySqlBuilder::SelectByFieldStatement(fieldname));
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

    static Entities GetAll()
    {
        prepareStatement(_getAllEntitiesStatement,
                         &EntitySqlBuilder::SelectAllStatement);

        return GetManyByQuery(_getAllEntitiesStatement);
    }

    template <typename Value>
    static Entities GetManyByField(const std::string& fieldname, Value value)
    {
        Statement statement = PrepareStatement(
                EntitySqlBuilder::SelectByFieldStatement(fieldname));
        *statement << value;

        return GetManyByQuery(statement);
    }

    static Entities GetManyByQuery(const std::string& sql)
    {
        Statement statement = PrepareStatement(sql);
        return GetManyByQuery(statement);
    }

    static Entities GetManyByQuery(Statement& statement)
    {
        Entities entities;
        dbc::ResultSet::ptr result(statement->executeQuery());

        while (result->next())
        {
            Entity entity;
            entity.id = (*result)[0];

            ObjectFieldBinder fieldbinder(*result);
            Mapping::accept(fieldbinder, entity);

            entities.push_back(entity);
        }

        return entities;
    }

    static void ResetStatements()
    {
        // Free the resources associated with prepared statments.
        // The statements are phoenix singletons that spring to life
        // as needed.
        _insertEntityStatement.reset();
        _updateEntityStatement.reset();
        _deleteEntityStatement.reset();
        _getEntityByIdStatement.reset();
        _getAllEntitiesStatement.reset();
    }

private:
    Repository();

    static Statement _insertEntityStatement;
    static Statement _updateEntityStatement;
    static Statement _deleteEntityStatement;
    static Statement _getEntityByIdStatement;
    static Statement _getAllEntitiesStatement;

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

        try
        {
            // TODO: entity.id << *result;
            entity.id = (*result)[0];
        }
        catch (const dbc::NoResultsError& e)
        {
            std::ostringstream msg;
            msg << "No " << Mapping::getLabel() << " exists for query '"
                << statement->getSQL() << "'";
            throw DoesNotExistError(msg.str());
        }

        ObjectFieldBinder fieldbinder(*result);
        Mapping::accept(fieldbinder, entity);

        if (!allowMany && result->next())
        {
            std::ostringstream msg;
            msg << "More than one result for ";
            if (id > 0)
                msg << "ID " << id;
            else
                msg << "query '" << statement->getSQL() << "'";
            throw NotOneError(msg.str());
        }

        return entity;
    }

};

template <class Entity, class Mapping>
dbc::PreparedStatement::ptr Repository<Entity, Mapping>::_insertEntityStatement;

template <class Entity, class Mapping>
dbc::PreparedStatement::ptr Repository<Entity, Mapping>::_updateEntityStatement;

template <class Entity, class Mapping>
dbc::PreparedStatement::ptr Repository<Entity, Mapping>::_deleteEntityStatement;

template <class Entity, class Mapping>
dbc::PreparedStatement::ptr Repository<Entity, Mapping>::_getEntityByIdStatement;

template <class Entity, class Mapping>
dbc::PreparedStatement::ptr Repository<Entity, Mapping>::_getAllEntitiesStatement;

} }

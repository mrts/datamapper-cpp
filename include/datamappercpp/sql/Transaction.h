#ifndef DATAMAPPERCPP_TRANSACTION_H__
#define DATAMAPPERCPP_TRANSACTION_H__

#include <datamappercpp/sql/db.h>

namespace dm {
namespace sql {

class Transaction
{
public:
    Transaction(bool enabled = true) :
        _is_enabled(enabled),
        _is_completed(false)
    {
        if (_is_enabled)
            // TODO: default isolation level
            ExecuteStatement("BEGIN TRANSACTION");
    }

    void commit()
    {
        if (_is_enabled && !_is_completed)
        {
            do_commit();
            _is_completed = true;
        }
    }

    void rollback()
    {
        if (_is_enabled && !_is_completed)
        {
            do_rollback();
            _is_completed = true;
        }
    }

    ~Transaction()
    {
        // TODO: beware of the silent rollback
        if (_is_enabled && !_is_completed)
            do_rollback();
    }

private:
    void do_commit()
    { ExecuteStatement("COMMIT TRANSACTION"); }

    void do_rollback()
    { ExecuteStatement("ROLLBACK TRANSACTION"); }

    bool _is_enabled;
    bool _is_completed;
};

} }

#endif /* TRANSACTION_H */

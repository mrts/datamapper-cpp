# datamapper-cpp

*datamapper-cpp* is an object-relational mapper for C++ and SQLite. It is a thin layer on top of
[dbc-cpp](https://github.com/mrts/datamapper-cpp), a lightweight database access library
for C++ with JDBC-like *ResultSet* interface. 

## Features

- Object-database mapping, get objects from database and save objects to database
- Objects of normal C++ classes work, no need to inherit from anything
- Developer specifies mapping by providing object field to DB column mapping in a mapping class
- Repository class that receives the domain object and the mapping as template parameters
  - takes care of SQL statement generation
  - takes care of binding fields to SQL statements
  - takes care of filling fields from resultsets
  - manages transactions
  
It relies on the following [dbc-cpp](https://github.com/mrts/datamapper-cpp) low-level database operations:
- query execution
- prepared statements:
  - compiled SQL statement objects, parameter binding
- result sets:
  - iteration over the results, fetching values
- dealing with datatypes

And finally, dbc-cpp relies on SQLite, the architecture diagram is as follows:

```
+----------------+
| datamapper-cpp |
+----------------+
|    dbc-cpp     |
+----------------+
|     SQLite     |
+----------------+
```

## API examples

See [tests](https://github.com/mrts/datamapper-cpp/blob/master/test/src/main.cpp) for more details:

```c++
// Create database table.
PersonRepository::CreateTable();

// Save a single object to database.
Person p(-1, "Ervin", 38, 1.80);
PersonRepository::Save(p);

// Save multiple objects.
Person::list ps;
ps.push_back(Person(-1, "Marvin", 24, 1.65));
ps.push_back(Person(-1, "Steve",  32, 2.10));
PersonRepository::Save(ps);

// Get single object from database.
Person p = PersonRepository::Get(1);
p = PersonRepository::GetByField("name", "Marvin");
p = PersonRepository::GetByQuery("SELECT * FROM person WHERE name LIKE '%vin'");

// Get multiple objects from database.
Person::list ps = PersonRepository::GetAll();
ps = PersonRepository::GetManyByField("age", 32);
ps = PersonRepository::GetManyByQuery("SELECT * FROM person WHERE name LIKE '%vin'");

// Delete data from database.
PersonRepository::Delete(1);
Person marvin(2, "Marvin", 24, 1.65);
PersonRepository::Delete(marvin);
```

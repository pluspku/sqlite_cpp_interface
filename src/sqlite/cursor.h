extern "C" {
#include "sqlite3.h"
}

#include <string>
#include <vector>
#include <iostream>

namespace sqlite {
  class SQLiteException: public std::exception {
    public:
      SQLiteException(std::string msg):_message(msg){}
      const char * what () const throw () {
        return _message.c_str();
      }
      virtual ~SQLiteException() throw() {}
    private:
      std::string _message;
  };

  class Value {
    public:
      inline int vint() const;
      inline double vfloat() const;
      inline std::string vtext() const;
      inline int dtype() const { return _dtype; };

    private:
      Value(sqlite3_value * value):_value(value){
        _dtype = ::sqlite3_value_numeric_type(value);
      }
      friend class Row;
      int _dtype;
      sqlite3_value * _value;
  };
  std::ostream & operator<< (std::ostream & os, const Value & value);

  class Row {
    public:
      Value operator[](int index) const;
      //Value operator[](const std::string index) const;
      ~Row();
    private:
      int _nCols;
      sqlite3_value ** _values;
      Row(sqlite3_stmt* stmt);
      friend class Cursor;
  };

  class Rows: public std::vector<Row*> {
    public:
      virtual ~Rows() {
      for (Rows::iterator it = begin(); it!=end(); it++) {
        delete (*it);
      }
    }
  };

  class Binding {

  };

  class Cursor {
    public:
      Cursor * execute(std::string query, Binding * binding = NULL);
      Row * fetchone();
      Rows fetchall();
    private:
      Cursor(sqlite3* conn):_conn(conn),_stmt(NULL){};
      sqlite3 * _conn;
      sqlite3_stmt * _stmt;
      friend class Connection;
      void step();
      void finish();
  };

  class Connection {
    public:
      Connection(std::string db, int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);
      ~Connection();
      void close();
      Cursor cursor();
    private:
      sqlite3 * _conn;
  };
}


#include "cursor.h"

namespace sqlite {
  void handle_error(int rcode, sqlite3* conn) {
    if (rcode != SQLITE_OK)
      throw SQLiteException(::sqlite3_errmsg(conn));
  }

  Connection::Connection(std::string db, int flags) {
    const int rcode = ::sqlite3_open_v2(db.c_str(), &_conn, flags, NULL);
    handle_error(rcode, _conn);
  }

  void Connection::close() {
    if (_conn) {
      const int rcode = ::sqlite3_close(_conn);
      handle_error(rcode, _conn);
    }
    _conn = NULL;
  }

  Connection::~Connection() {
    close();
  }

  Cursor Connection::cursor() {
    return Cursor(_conn);
  }

  Cursor * Cursor::execute(std::string query, Binding * binding) {
    if (_stmt)
      throw SQLiteException("Unfinished statement");
    const char * buffer = query.c_str();
    const int rcode = ::sqlite3_prepare_v2(_conn, buffer, query.length(), &_stmt, NULL);
    handle_error(rcode, _conn);
    if (::sqlite3_column_count(_stmt) == 0) {
      ::sqlite3_step(_stmt);
      finish();
    }
    return this;
  }

  void Cursor::finish() {
    const int rcode = ::sqlite3_finalize(_stmt);
    handle_error(rcode, _conn);
    _stmt = NULL;
  }

  Rows Cursor::fetchall() {
    Rows rows;
    while (true) {
      const int rcode = ::sqlite3_step(_stmt);
      switch (rcode) {
        case SQLITE_DONE:
          finish();
          return rows;
        case SQLITE_ROW:
          rows.push_back(new Row(_stmt));
          break;
        default:
          throw SQLiteException(std::string("Unhandled status: ") + std::to_string(rcode));
      }
    }
  }

  Row::Row(sqlite3_stmt * stmt) {
    _nCols = ::sqlite3_column_count(stmt);
    _values = new sqlite3_value*[_nCols];
    for (int i=0; i < _nCols; i++){
      _values[i] = ::sqlite3_value_dup(::sqlite3_column_value(stmt, i));
    }
  }

  Row::~Row() {
    for (int i=0; i < _nCols; i++) {
      ::sqlite3_value_free(_values[i]);
    }
    delete[] _values;
  }

  Value Row::operator[](int index) const {
    if (index >= _nCols) {
      throw SQLiteException("Column index out of range");
    }
    return Value(_values[index]);
  }

  inline int Value::vint() const {
    return ::sqlite3_value_int64(_value);
  }

  inline double Value::vfloat() const {
    return ::sqlite3_value_double(_value);
  }

  inline std::string Value::vtext() const {
    return std::string(reinterpret_cast<const char *>(::sqlite3_value_text(_value)));
  }

  std::ostream & operator<< (std::ostream & os, const Value & value) {
    switch (value.dtype()) {
      case SQLITE_INTEGER:
        os << value.vint();
        break;
      case SQLITE_FLOAT:
        os << value.vfloat();
        break;
      case SQLITE_TEXT:
        os << value.vtext();
        break;
      case SQLITE_NULL:
        os << "NULL";
        break;
      default:
        throw SQLiteException("Notimplemented dtype");
    }
    return os;
  }
}


#include <PostgreSQL.hpp>

using std::string;
using std::to_string;

namespace PostgreSQL {

void Server::connect(string host, string user, string password, string db, int
    timeout) {
  string conninfo = "host=" + host +
      " user=" + user +
      " password=" + password +
      " dbname=" + db +
      " connect_timeout=" + to_string(timeout);
  conn.reset(PQconnectdb(conninfo.c_str()));
}

void Server::disconnect() {
  if (conn != nullptr) {
    PQfinish(conn.release());
  }
}

int Server::insert(const string& absolute_table, const string& column_list,
    const string& value_list, const string& on_conflict) {
  m_error.clear();
  auto s = "insert into " + absolute_table + " (" + column_list + ") values (" +
      value_list + ")";
  if (!on_conflict.empty()) {
    s += " on conflict " + on_conflict;
  }
  s += ";";
  auto result = PQexec(conn.get(), s.c_str());
  if (PQresultStatus(result) == PGRES_COMMAND_OK) {
    return 0;
  }
  m_error = PQresultErrorMessage(result);
  return -1;
}

} // end namespace PostgreSQL

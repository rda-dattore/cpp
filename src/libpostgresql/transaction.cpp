#include <PostgreSQL.hpp>
#include <myexception.hpp>

using std::string;
using std::to_string;

namespace PostgreSQL {

int Transaction::commit() {
  if (is_started) {
    if (m_server->command("commit") < 0) {
      m_error = "Commit error: '" + m_server->error() + "'";
      return -1;
    }
    is_started = false;
    return 0;
  }
  m_error = "Transaction not started.";
  return -1;
}

void Transaction::get_lock(long long lock_id, size_t timeout_seconds) {
  if (m_server->command("set local lock_timeout = '" + to_string(
      timeout_seconds) + "s'; select pg_advisory_xact_lock(" + to_string(
      lock_id) + ")") < 0) {
    throw my::BadOperation_Error("Unable to obtain lock: '" + m_server->error()
        + "'");
  }
}

int Transaction::rollback() {
  if (is_started) {
    if (m_server->command("rollback") < 0) {
      m_error = m_server->error();
      return -1;
    }
    is_started = false;
    return 0;
  }
  m_error = "Transaction not started.";
  return -1;
}

void Transaction::start(Server& server) {
  if (is_started) {
    throw my::BadOperation_Error("Transaction already started.");
  }
  m_server = &server;
  if (m_server->command("start transaction") < 0) {
    throw my::BadOperation_Error(m_server->error());
  }
  is_started = true;
}

} // end namespace PostgreSQL

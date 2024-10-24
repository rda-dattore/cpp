#include <PostgreSQL.hpp>
#include <strutils.hpp>
#include <myexception.hpp>

using std::make_pair;
using std::pair;
using std::stoll;
using std::string;
using std::to_string;
using std::vector;
using strutils::append;
using strutils::replace_all;
using strutils::split;
using strutils::substitute;
using strutils::trim;

namespace PostgreSQL {

int Server::command(string command, string *result) {
  m_error.clear();
  auto r = PQexec(conn.get(), command.c_str());
  auto status = PQresultStatus(r);
  if (status == PGRES_COMMAND_OK || status == PGRES_TUPLES_OK) {
    if (result != nullptr) {
      result->clear();
      auto num_rows = PQntuples(r);
      auto num_fields = PQnfields(r);
      for (auto n = 0; n < num_rows; ++n) {
        string row = "|";
        for (auto m = 0; m < num_fields; ++m) {
          row += string(PQgetvalue(r, n, m), PQgetlength(r, n, m)) + "|";
        }
        append(*result, row, "\n");
      }
    }
    return 0;
  }
  m_error = PQresultErrorMessage(r);
  if (m_error.find("ERROR:  duplicate key value violates unique constraint") ==
      0) {
    return -2;
  }
  return -1;
}

static void custom_receiver(void *arg, const PGresult *res) {
  // do nothing - which suppresses notices output
}

void Server::connect(string host, string user, string password, string db, int
    timeout) {
  string conninfo = "host=" + host +
      " user=" + user +
      " password=" + password +
      " dbname=" + db +
      " connect_timeout=" + to_string(timeout);
  conn.reset(PQconnectdb(conninfo.c_str()));
  if (PQstatus(conn.get()) != CONNECTION_OK) {
    m_error = PQerrorMessage(conn.get());
  }
  PQsetNoticeReceiver(conn.get(), custom_receiver, nullptr);
}

int Server::create_index(string absolute_table, string index_name, string
    column_list) {
  auto p = split_tablename(absolute_table);
  return command("create index " + index_name + " on \"" + p.first + "\"." + p.
      second + " (" + column_list + ")");
}

int Server::_delete(string absolute_table, string where_conditions) {
  auto p = split_tablename(absolute_table);
  string s = "delete from " + postgres_ready(p.first) + "." + p.second;
  if (!where_conditions.empty()) {
    s += " where " + where_conditions;
  }
  s += ";";
  return command(s);
}

void Server::disconnect() {
  if (conn != nullptr) {
    PQfinish(conn.release());
  }
}

int Server::drop_index(string schema_name, string index_name) {
  return command("drop index \"" + schema_name + "\"." + index_name);
}

int Server::duplicate_table(string absolute_table_source, string
    absolute_table_target) {
  auto p_tgt = split_tablename(absolute_table_target);
  auto p_src = split_tablename(absolute_table_source);
  int status = command("create table " + postgres_ready(p_tgt.first) + "." +
      p_tgt.second + " (like " + postgres_ready(p_src.first) + "." + p_src.
      second + " including defaults)");
  if (status != 0) {
    return status;
  }
  string result;
  status = command("select indexname, indexdef from pg_indexes where "
      "schemaname = '" + p_src.first + "' and tablename = '" + p_src.second +
      "'", &result);
  if (status != 0) {
    return status;
  }
  auto sp = split(result, "\n");
  for (const auto& line : sp) {
    auto sp2 = split(line, "|");
    auto new_name = sp2[1];
    if (new_name.find(p_src.second) == 0) {
      new_name = new_name.substr(p_src.second.length());
    } else {
      replace_all(new_name, "dsnnnn_inventory_p", "");
      replace_all(new_name, "duid_inventory_p", "");
      replace_all(new_name, "dsnnnn_inventory_lati_loni_b", "");
      replace_all(new_name, "duid_inventory_lati_loni_b", "");
      replace_all(new_name, "dsnnnn_inventory_lati_loni", "");
      replace_all(new_name, "duid_inventory_lati_loni", "");
      replace_all(new_name, "dsnnnn_point_times_decade", "");
      replace_all(new_name, "duid_point_times_decade", "");
      replace_all(new_name, "dsnnnn_time_series_times_decade", "");
    }
    if (p_tgt.second.find("ds") == 0 && p_tgt.second[6] == '_' && new_name.
        find("dsnnnn_") == 0) {
      new_name = p_tgt.second.substr(0, 6) + new_name.substr(6);
    } else if (p_tgt.second[0] == 'd' && p_tgt.second[7] == '_' && new_name.
        find("duid_") == 0) {
      new_name = p_tgt.second.substr(0, 7) + new_name.substr(4);
    } else {
      new_name = p_tgt.second + new_name;
    }
    auto is_primary_key = false;
    if (new_name.find("_pkey") != string::npos) {
      is_primary_key = true;
    }
    auto sp3 = split(sp2[2], " ON ");
    if (sp3.size() != 2) {
      m_error = "unable to parse indexdef '" + sp2[2] + "'";
      return -1;
    }
    if (is_primary_key) {
      replace_all(sp3[0], sp2[1], new_name + "_001");
    } else {
      replace_all(sp3[0], sp2[1], new_name);
    }
    replace_all(sp3[1], p_src.first, p_tgt.first);
    replace_all(sp3[1], p_src.second, p_tgt.second);
    status = command(sp3[0] + " ON " + sp3[1]);
    if (status != 0) {
      return status;
    }
    if (is_primary_key) {
      status = command("alter table only " + postgres_ready(p_tgt.first) + "." +
          p_tgt.second + " add constraint " + new_name + " primary key using "
          "index " + new_name + "_001");
      if (status != 0) {
        return status;
      }
    }
  }

  // check for sequences in the template
  status = command("select column_name, column_default from "
      "information_schema.columns where table_schema = '" + p_src.first + "' "
      "and table_name = '" + p_src.second + "'", &result);
  if (status != 0) {
    return status;
  }
  sp = split(result, "\n");
  for (const auto& line : sp) {
    auto sp2 = split(line, "|");
    if (sp2[2].find("nextval(") != string::npos) {
      auto seq = "\"" + p_tgt.first + "\"." + p_tgt.second + "_" + sp2[1] +
          "_seq";
      status = command("create sequence " + seq);
      if (status != 0) {
        return status;
      }
      status = command("alter table \"" + p_tgt.first + "\"." + p_tgt.second +
          " alter column " + sp2[1] + " set default nextval('" + seq + "')");
      if (status != 0) {
        return status;
      }
    }
  }
  return 0;
}

vector<string> Server::get_index_names(string absolute_table) {
  auto p = split_tablename(absolute_table);
  vector<string> v;
  string result;
  if (command("select indexname from pg_indexes where schemaname = '" + p.first
      + "' and tablename = '" + p.second + "'", &result) == 0) {
    auto sp = split(result, "\n");
    for (auto& part : sp) {
      trim(part);
      if (part != "indexname" && part[0] != '-' and part.find("rows)") ==
          string::npos) {
        v.emplace_back(substitute(part, "|", ""));
      }
    }
  }
  return v;
}

int Server::insert(const string& absolute_table, const string&
    row_specification, const string& on_conflict) {
  return insert(absolute_table, "", row_specification, on_conflict);
}

int Server::insert(const string& absolute_table, const string& column_list,
    const string& value_list, const string& on_conflict) {
  auto p = split_tablename(absolute_table);

  // if absolute_table is a view, get the underlying table name so that the
  //   'on conflict' clause will work
  LocalQuery q("select table_schema, table_name from information_schema."
      "view_table_usage where view_schema = '" + p.first + "' and view_name = '"
      + p.second + "'");
  if (q.submit(*this) == 0 && q.num_rows() == 1) {
    Row row;
    q.fetch_row(row);
    p = std::make_pair(row[0], row[1]);
  }
  auto s = "insert into " + postgres_ready(p.first) + "." + p.second;
  if (!column_list.empty()) {
    s += " (" + column_list + ")";
  }
  s += " values (" + value_list + ")";
  if (!on_conflict.empty()) {
    s += " on conflict " + on_conflict;
  }
  s += ";";
  return command(s);
}

vector<string> Server::schema_names() {
  vector<string> v; // return value
  string result;
  auto status = command("select distinct schemaname from pg_tables", &result);
  if (status == 0) {
    auto sp = split(result, "\n");
    for (const auto& line : sp) {
      v.emplace_back(substitute(line, "|", ""));
    }
  }
  return v;
}

int Server::update(string absolute_table, string column_name_value_pair, string
    where_conditions) {
  if (column_name_value_pair.empty()) {
    return -1;
  }
  auto p = split_tablename(absolute_table);
  auto s = "update " + postgres_ready(p.first) + "." + p.second + " set " +
      column_name_value_pair;
  if (!where_conditions.empty()) {
    s += " where " + where_conditions;
  }
  s += ";";
  return command(s);
}

int Server::update(string absolute_table, vector<string>&
    column_name_value_pairs, string where_conditions) {
  if (column_name_value_pairs.empty()) {
    return -1;
  }
  auto p = split_tablename(absolute_table);
  auto it = column_name_value_pairs.begin();
  string s = "update " + postgres_ready(p.first) + "." + p.second + " set " +
      *it;
  auto end = column_name_value_pairs.end();
  for (++it; it != end; ++it) {
    append(s, *it, ", ");
  }
  if (!where_conditions.empty()) {
    s += " where " + where_conditions;
  }
  s += ";";
  return command(s);
}

} // end namespace PostgreSQL

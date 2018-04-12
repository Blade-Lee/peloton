//===----------------------------------------------------------------------===//
//
//                         Peloton
//
// index_selection_util.h
//
// Identification: src/include/brain/index_selection_util.h
//
// Copyright (c) 2015-2018, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <string.h>
#include <set>
#include <sstream>
#include <vector>
#include "catalog/index_catalog.h"
#include "parser/sql_statement.h"

namespace peloton {
namespace brain {

using namespace parser;

// Represents a hypothetical index
class IndexObject {
 public:
  oid_t db_oid;
  oid_t table_oid;
  std::set<oid_t> column_oids;
  IndexConstraintType type;

  IndexObject(){};

  IndexObject(oid_t db_oid, oid_t table_oid, oid_t col_oid)
      : db_oid(db_oid), table_oid(table_oid) {
    column_oids.insert(col_oid);
  }

  IndexObject(oid_t db_oid, oid_t table_oid, std::vector<oid_t> &col_oids)
      : db_oid(db_oid), table_oid(table_oid) {
    for (auto col : col_oids) column_oids.insert(col);
  }

  // To string for performing hash.
  const std::string toString() const;

  bool operator==(const IndexObject &obj) const;

  bool IsCompatible(std::shared_ptr<IndexObject> index) const;
  IndexObject Merge(std::shared_ptr<IndexObject> index);
};

struct IndexObjectHasher {
  size_t operator()(const IndexObject &obj) const {
    return std::hash<std::string>()(obj.toString());
  }
};

// Represents a set of hypothetical indexes - An index configuration.
class IndexConfiguration {
 public:
  IndexConfiguration();
  IndexConfiguration(std::set<std::shared_ptr<IndexObject>> index_obj_set) {
    indexes_ = index_obj_set;
  };
  void Add(IndexConfiguration &config);
  void Merge(IndexConfiguration &config);
  void AddIndexObject(std::shared_ptr<IndexObject> index_info);
  void RemoveIndexObject(std::shared_ptr<IndexObject> index_info);

  size_t GetIndexCount() const;
  const std::set<std::shared_ptr<IndexObject>> &GetIndexes() const;
  const std::string ToString() const;
  bool operator==(const IndexConfiguration &obj) const;
  IndexConfiguration operator-(const IndexConfiguration &obj);

 private:
  // The set of hypothetical indexes in the configuration
  std::set<std::shared_ptr<IndexObject>> indexes_;
};

// Represents a workload of SQL queries
class Workload {
 private:
  std::vector<SQLStatement *> sql_queries_;

 public:
  Workload() {}
  Workload(SQLStatement *query) : sql_queries_({query}) {}
  void AddQuery(SQLStatement *query) { sql_queries_.push_back(query); }
  const std::vector<SQLStatement *> &GetQueries() { return sql_queries_; }
  size_t Size() { return sql_queries_.size(); }
};

class IndexObjectPool {
 public:
  IndexObjectPool();
  std::shared_ptr<IndexObject> GetIndexObject(IndexObject &obj);
  std::shared_ptr<IndexObject> PutIndexObject(IndexObject &obj);

 private:
  std::unordered_map<IndexObject, std::shared_ptr<IndexObject>,
                     IndexObjectHasher>
      map_;
};

}  // namespace brain
}  // namespace peloton

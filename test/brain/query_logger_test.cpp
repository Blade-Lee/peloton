//===----------------------------------------------------------------------===//
//
//                         Peloton
//
// query_logger_test.cpp
//
// Identification: test/brain/query_logger_test.cpp
//
// Copyright (c) 2015-16, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include <memory>

#include "catalog/catalog.h"
#include "common/harness.h"
#include "concurrency/transaction_manager_factory.h"
#include "sql/testing_sql_util.h"
#include "settings/settings_manager.h"
#include "parser/pg_query.h"
#include "threadpool/brain_thread_pool.h"

using std::vector;
using std::string;

namespace peloton {
namespace test {

class QueryLoggerTests : public PelotonTest {
 protected:
  virtual void SetUp() override {
    settings::SettingsManager::SetBool(settings::SettingId::brain, true);
    PelotonInit::Initialize();

    // query to check that logging is done
    select_query =
        "SELECT query_string, fingerprint FROM pg_catalog.pg_query_history;";
    select_query_fingerprint =
        pg_query_fingerprint(select_query.c_str()).hexdigest;
    wait_time = 2;
  }

  virtual void TearDown() override { PelotonInit::Shutdown(); }

  // Executes the given query and then checks if the queries that are executed
  // till now are actually logged
  void TestSimpleUtil(string const &test_query,
                      vector<std::string> &expected_result) {
    string test_query_fingerprint =
        pg_query_fingerprint(test_query.c_str()).hexdigest;
    expected_result.push_back(test_query + "|" + test_query_fingerprint);
    TestingSQLUtil::ExecuteSQLQuery(test_query.c_str());

    // give some time to actually log this query
    sleep(wait_time);

    TestingSQLUtil::ExecuteSQLQueryAndCheckResult(select_query.c_str(),
                                                  expected_result, true);

    // the select query we used will also be logged for next time
    expected_result.push_back(select_query + "|" + select_query_fingerprint);
  }

  // Executes the given query and then checks if the queries that are executed
  // till now are actually logged only when the transaction commits. Otherwise
  // stores to queries for checking this later when commit happens.
  void TestTransactionUtil(string const &test_query,
                           vector<std::string> &expected_result,
                           bool committed) {
    static vector<std::string> temporary_expected_result;
    string test_query_fingerprint =
        pg_query_fingerprint(test_query.c_str()).hexdigest;
    temporary_expected_result.push_back(test_query + "|" +
                                        test_query_fingerprint);
    TestingSQLUtil::ExecuteSQLQuery(test_query.c_str());

    // give some time to actually log this query
    sleep(wait_time);

    // only check once the the transaction has committed
    if (committed) {
      expected_result.insert(expected_result.end(),
                             temporary_expected_result.begin(),
                             temporary_expected_result.end());
      temporary_expected_result.clear();
      TestingSQLUtil::ExecuteSQLQueryAndCheckResult(select_query.c_str(),
                                                    expected_result, true);
      // the select query we used will also be logged for next time
      expected_result.push_back(select_query + "|" + select_query_fingerprint);
    }
  }

 protected:
  string select_query;  // fixed query to check the queries logged in the table
  string select_query_fingerprint;  // fingerprint for the fixed query
  int wait_time;  // time to wait in seconds for the query to log into the table
};

// Testing the functionality of query logging
TEST_F(QueryLoggerTests, QueriesTest) {
  vector<std::string> expected_result;  // used to store the expected result

  // create the table and do some inserts and check
  TestSimpleUtil("CREATE TABLE test(a INT);", expected_result);
  TestSimpleUtil("INSERT INTO test VALUES (1);", expected_result);
  TestSimpleUtil("INSERT INTO test VALUES (2);", expected_result);

  // check if the queries are logged only when the transaction actually commits
  TestTransactionUtil("BEGIN;", expected_result, false);
  TestTransactionUtil("INSERT INTO test VALUES (1);", expected_result, false);
  TestTransactionUtil("COMMIT;", expected_result, true);

  // final check to see if everything is ok
  TestSimpleUtil(select_query, expected_result);
}

}  // namespace test
}  // namespace peloton

//===----------------------------------------------------------------------===//
//
//                         Peloton
//
// index_selection.h
//
// Identification: src/include/brain/index_selection.h
//
// Copyright (c) 2015-2018, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include "brain/index_selection_context.h"
#include "brain/index_selection_util.h"
#include "catalog/index_catalog.h"
#include "expression/tuple_value_expression.h"
#include "parser/sql_statement.h"

namespace peloton {
namespace brain {

/**
 * @brief Comparator for set of (Index Configuration, Cost)
 */
struct IndexConfigComparator {
  IndexConfigComparator(Workload &workload) { this->w = &workload; }
  bool operator()(const std::pair<IndexConfiguration, double> &s1,
                  const std::pair<IndexConfiguration, double> &s2) {
    // Order by cost. If cost is same, then by the number of indexes
    // Unless the configuration is exactly the same, get some ordering
    return ((s1.second < s2.second) ||
            (s1.first.GetIndexCount() < s2.first.GetIndexCount()) ||
            (s1.first.ToString() < s2.first.ToString()));
  }

  Workload *w;
};

//===--------------------------------------------------------------------===//
// IndexSelection
//===--------------------------------------------------------------------===//

class IndexSelection {
 public:
  /**
   * @brief Constructor
   */
  IndexSelection(Workload &query_set, size_t max_index_cols,
                 size_t enumeration_threshold, size_t num_indexes);

  /**
   * @brief The main external API for the Index Prediction Tool
   * @returns The best possible Index Congurations for the workload
   */
  void GetBestIndexes(IndexConfiguration &final_indexes);

  /**
   * @brief Gets the indexable columns of a given query
   */
  void GetAdmissibleIndexes(parser::SQLStatement *query,
                            IndexConfiguration &indexes);

  /**
   * @brief GenerateCandidateIndexes.
   * If the admissible config set is empty, generate
   * the single-column (admissible) indexes for each query from the provided
   * queries and prune the useless ones. This becomes candidate index set. If
   * not empty, prune the useless indexes from the candidate set for the given
   * workload.
   *
   * @param candidate_config - new candidate index to be pruned.
   * @param admissible_config - admissible index set of the queries
   * @param workload - queries
   */
  void GenerateCandidateIndexes(IndexConfiguration &candidate_config,
                                IndexConfiguration &admissible_config,
                                Workload &workload);

  /**
   * @brief gets the top k cheapest indexes for the workload
   *
   * @param indexes - the indexes in the workload
   * @param top_indexes - the top k cheapest indexes in the workload are
   * returned through this parameter
   * @param workload - the given workload
   * @param k - the number of indexes to return
   */
  void Enumerate(IndexConfiguration &indexes, IndexConfiguration &top_indexes,
                 Workload &workload, size_t k);

  /**
   * @brief generate multi-column indexes from the single column indexes by
   * doing a cross product.
   */
  void GenerateMultiColumnIndexes(IndexConfiguration &config,
                                  IndexConfiguration &single_column_indexes,
                                  IndexConfiguration &result);

 private:
  /**
   * @brief PruneUselessIndexes
   * Delete the indexes from the configuration which do not help at least one of
   * the queries in the workload
   *
   * @param config - index set
   * @param workload - queries
   * @param pruned_config - result configuration
   */
  void PruneUselessIndexes(IndexConfiguration &config, Workload &workload, IndexConfiguration &pruned_config);

  /**
   * @brief Gets the cost of an index configuration for a given workload
   * directly from the memo table. Assumes ComputeCost is called.
   * TODO (Priyatham): This function can be removed now since the requirement
   * for the comparator to be a const has been eliminated by me.
   */
  double GetCost(IndexConfiguration &config, Workload &workload) const;

  /**
   * @brief Gets the cost of an index configuration for a given workload. It
   * would call the What-If API appropriately and stores the results in the memo
   * table
   */
  double ComputeCost(IndexConfiguration &config, Workload &workload);

  // Configuration Enumeration related
  /**
   * @brief Gets the cheapest indexes through naive exhaustive enumeration by
   * generating all possible subsets of size <= m where m is a tunable parameter
   */
  void ExhaustiveEnumeration(IndexConfiguration &indexes,
                             IndexConfiguration &top_indexes,
                             Workload &workload);

  /**
   * @brief Gets the remaining cheapest indexes through greedy search
   */
  void GreedySearch(IndexConfiguration &indexes,
                    IndexConfiguration &remaining_indexes, Workload &workload,
                    size_t num_indexes);

  // Admissible index selection related
  /**
   * @brief Helper to parse the order where in the SQL statements such as
   * select, delete, update.
   */
  void IndexColsParseWhereHelper(
      const expression::AbstractExpression *where_expr,
      IndexConfiguration &config);

  /**
   * @brief Helper to parse the group by clause in the SQL statements such as
   * select, delete, update.
   */
  void IndexColsParseGroupByHelper(
      std::unique_ptr<parser::GroupByDescription> &where_expr,
      IndexConfiguration &config);

  /**
   * @brief Helper to parse the order by clause in the SQL statements such as
   * select, delete, update.
   */
  void IndexColsParseOrderByHelper(
      std::unique_ptr<parser::OrderDescription> &order_by,
      IndexConfiguration &config);

  /**
   * @brief Helper function to convert a tuple of <db_oid, table_oid, col_oid>
   * to an IndexObject and store into the IndexObject shared pool.
   *
   * @param - tuple_col: representation of a column
   * @param - config: returns a new index object here
   */
  void IndexObjectPoolInsertHelper(
      const std::tuple<oid_t, oid_t, oid_t> tuple_col,
      IndexConfiguration &config);

  /**
   * @brief Create a new index configuration which is a cross product of the
   * given configurations. Ex: {I1} * {I23, I45} = {I123, I145}
   *
   * @param - configuration1: config1
   * @param - configuration2: config2
   * @param - result: cross product
   */
  void CrossProduct(const IndexConfiguration &configuration1,
                    const IndexConfiguration &configuration2,
                    IndexConfiguration &result);

  // Set of parsed and bound queries
  Workload query_set_;
  // Common context of index selection object.
  IndexSelectionContext context_;
};

}  // namespace brain
}  // namespace peloton

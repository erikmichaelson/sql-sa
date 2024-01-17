#include "../card.cpp"
#include "duckdb.hpp"

DuckDB::DuckDB db(nullptr);
DuckDB::Connection CON(db);
// initialize the catalog
// TODO: make the parquet files referenced here
all_sqls = open("dag.sql")
db.query(all_sqls);

CardRuntime * card = card_runtime_new();
card.register_sql(all_sqls);
card.register_conn(db);
//card.register_IDE(input_box);

int failures = 0;
TSNode * tos = card.tables_referring_to("etl.snapshots");
TSNode * froms = card.tables_refered_from("etl.snapshots");
db.Query("create temp table results (node_id int); create temp table expected (node_id int)");
// we expect this to be zero
auto test = db.Query("select e.node_id from expected where e.node_id not in (select r.node_id from results) \
                     union all\
                     select r.node_id from results  where r.node_id not in (select e.node_id from expected);");
if(test.RowCount != 0) { printf("FAIL: %s returned by tos, %s expected", results.ToString(), expected.ToString()); failures++; };
free(tos);
free(froms);

#include <tree_sitter/api.h>

bool node_compare(TSNode n1, TSNode n2);
const char * node_to_string(const char * source, const TSNode n);
TSNode parent_context(TSTree * tree, TSPoint clicked);
std::list<TSNode> expand_select_star(TSNode star_node, const char * source);
std::list<TSNode> result_columns_for_ddl(TSNode ddl, const char * source);
std::list<TSNode> references_to_table(TSTree * tree, std::string code, const char * table);
std::list<TSNode> references_from_context(TSTree * tree, std::string code, TSNode parent, const char * table);
std::list<TSNode> tables_downstream_of_table(TSTree * tree, std::string code, const char * table);
std::list<TSNode> contexts_upstream_of_context(TSTree * tree, std::string code, TSNode parent, const char * context_name);
std::list<std::string> get_table_names(TSTree * tree, std::string code);
const char * get_ddl_for_table_name(const char * source, TSNode * ddl);

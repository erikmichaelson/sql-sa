std::list<TSNode> references_from_table(TSTree * tree, std::string code, const char * table);

std::list<TSNode> tables_downstream_of_table(TSTree * tree, std::string code, const char * table);

std::list<TSNode> references_to_table(TSTree * tree, std::string code, const char * table);

std::list<TSNode> result_columns_for_ddl(TSNode ddl, const char * source);

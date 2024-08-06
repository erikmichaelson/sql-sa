#include "card.cpp"

int test_references_from(TSParser * parser) {
    std::ifstream fd;
    fd.open("dag.sql");
    std::string dag( (std::istreambuf_iterator<char>(fd) ),
                     (std::istreambuf_iterator<char>()));
    fd.close();
    TSTree * tree = ts_parser_parse_string(
        parser,
        NULL,
        dag.c_str(),
        (int) dag.size()
    );

    int failures = 0;
    std::list<TSNode> l = references_from_table(tree, dag, "dag.another");
    std::vector<TSNode> res { std::make_move_iterator(std::begin(l)), std::make_move_iterator(std::end(l)) };
    if(ts_node_start_point(res[0]).row != 42 && ts_node_start_point(res[0]).column != 9 ) {
        printf("FAILURE: expected a node starting at row %i col %i with text %s, got row %i col %i\n"
                ,42, 9, node_to_string(dag.c_str(), res[0])
                ,ts_node_start_point(res[0]).row, ts_node_start_point(res[0]).column);
        failures++;
    }
    if(ts_node_start_point(res[1]).row != 43 && ts_node_start_point(res[1]).column != 14) {
        printf("FAILURE: expected a node starting at row %i col %i with text %s, got row %i col %i\n"
                ,43, 14, node_to_string(dag.c_str(), res[1])
                ,ts_node_start_point(res[1]).row, ts_node_start_point(res[1]).column);
        failures++;
    }

    ts_tree_delete(tree);
    return failures; 
}

int main() {
    TSParser * parser = ts_parser_new();
    ts_parser_set_language(parser, tree_sitter_sql());

    int failures = 0;
    failures += test_references_from(parser);
    printf("Total failures: %i\n", failures);

    ts_parser_delete(parser);
    return 0;
}

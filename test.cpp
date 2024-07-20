#include "card.cpp"

int compare_nodes(std:list<TSNode>, std::list<TSPoint>) {
    for();
}

int test_references_from() {
    std::string dag = open("dag.sql").read();
    TSTree * tree = ts_parser_parse(dag.c_str());
    std::list<TSNode> res = references_from_table(tree, dag, "dag.another");
    if(ts_node_start_point(res[0]).row != 42 && ts_node_start_point(res[0]) != 9 ) {
        printf("FAILURE: %s\n");
        failures++;
    if(ts_node_start_point(res[1]).row != 43 && ts_node_start_point(res[1]) != 14) {
        printf("FAILURE: %s\n");
        failures++;
    }
    return failures; 
}

int main(int argc, char ** argv) {
    int failures = 0;
    failures += test_references_from();
    printf("Failures: %i\n", failures);
}

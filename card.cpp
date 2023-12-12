#include <string>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <sstream>
// testing this out
#include <tree_sitter/api.h>
#include <sql.h>

std::string * get_file_order() {
    std::string *ret = (std::string *)malloc(sizeof(std::string));
    // find FROM tables, JOINed tables

    // build adjacency matrix
    /*     Pulls from
          A B C D E F G
        ------------------
       A|   - -   -      |
       B|-    -          |
       C|         - -    |
       D|-    -          |
       E|     - - -      |
       F|-        -      |
       G|-          -    |
        ------------------
    */

    return ret;
}

std::string open_sqls() {
    std::string ret;
    // open all SQL files into a buffer. Hideously inefficient, but we'll small atm
    DIR* cwd = opendir(".");
    while(struct dirent* e = readdir(cwd)) {
        std::string a = std::string(e->d_name);
        if(a.find(".sql") != std::string::npos) {
            printf("Looking in 'sql' file %s\n", e->d_name);
            std::ifstream fd;
            fd.open(e->d_name);
            std::string new_ret( (std::istreambuf_iterator<char>(fd) ),
                                 (std::istreambuf_iterator<char>()    ) );
            ret.append(new_ret);
            fd.close();
        }
    }
    return ret;
}

int main() {
    TSParser *parser = ts_parser_new();
    ts_parser_set_language(parser, tree_sitter_sql());

    std::string all_sqls = open_sqls();
    TSTree * tree = ts_parser_parse_string(
        parser,
        NULL,
        all_sqls.c_str(),
        strlen(all_sqls.c_str())
    );

    std::cout << all_sqls;

    TSQueryCursor * cursor = ts_query_cursor_new();
    const char * q = "(relation ( ( object_reference schema: (identifier)? name: (identifier)) @reference))";

    //printf("parsed root: %s", ts_node_string(ts_tree_root_node(tree)));
    //printf("query : %s", q);

    uint32_t q_error_offset;
    TSQueryError q_error;
    TSQuery * table_names_q = ts_query_new(
        tree_sitter_sql(),
        q,
        strlen(q),
        &q_error_offset,
        &q_error
    );

    ts_query_cursor_exec(cursor, table_names_q, ts_tree_root_node(tree));
    TSQueryMatch cur_match;

    //fprintf(stderr, "before loop\n");
    while(ts_query_cursor_next_match(cursor, &cur_match)) {
        //fprintf(stderr, "in loop\n");
        //printf("%s", ts_node_string(cur_match.captures[0].node));
        for(int i = 0; i < cur_match.capture_count; i++) {
            TSPoint start = ts_node_start_point(cur_match.captures[i].node);
            TSPoint end = ts_node_end_point(cur_match.captures[i].node);
            printf("[%2d:%-2d - %2d:%-2d] ", start.row, start.column, end.row, end.column);
            printf("id: %i, capture: %s\n", cur_match.id, ts_node_string(cur_match.captures[i].node));
        }
        //fprintf(stderr, "after print in loop\n");
    }
    //fprintf(stderr, "after loop\n");

    ts_tree_delete(tree);
    ts_parser_delete(parser);
    return 0;
}

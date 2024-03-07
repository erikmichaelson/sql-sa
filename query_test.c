#include <tree_sitter/api.h>
#include <sql.h>
#include <string.h>
#include <stdio.h>

int main(int argc, char ** argv) {
    // mostly handling args
    // usage `query FILENAME "(query (( here: (identifier) )) @saved)" `
    if(argc != 3) {
        printf("ERROR: usage is `query FILENAME \"(query (( here: (identifier) )) @saved)\" `\n");
        exit(1);
    }
    FILE * f = fopen(argv[1], "r");
    char * source;
    // todo: how do you test whether fopen worked?
    if(f) {
        fseek(f, 0L, SEEK_END);
        off_t size = ftello(f);
        rewind(f);
        source = (char *)malloc(size);
        fread(source, size, 1, f);
    } else {
        printf("ERROR: failed to open file for reading\n");
        exit(1);
    }
    TSParser * p = ts_parser_new();
    ts_parser_set_language(p, tree_sitter_sql());
    TSTree * tree = ts_parser_parse_string(p, NULL, source, strlen(source));

    TSQueryError q_error;
    unsigned int q_error_offset;
    TSQuery * q = ts_query_new(tree_sitter_sql(), argv[2], strlen(argv[2]), &q_error_offset, &q_error);
    TSQueryCursor * cursor = ts_query_cursor_new();
    ts_query_cursor_exec(cursor, q, ts_tree_root_node(tree));
    TSQueryMatch cur_match;
    while(ts_query_cursor_next_match(cursor, &cur_match)) {
        for(int i = 0; i < cur_match.capture_count; i++) {
            printf("%.*s\n", ts_node_end_byte(cur_match.captures[i].node) - ts_node_start_byte(cur_match.captures[i].node)
                         , source + ts_node_start_byte(cur_match.captures[i].node));
        }
    }
}

#include <string>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <sstream>
// testing this out
#include <tree_sitter/api.h>
#include <sql.h>
#include <duckdb.hpp>

//auto con = duckdb::Connection(":memory:");

const char * node_to_string(const char * source, const TSNode n) {
    char * ret = (char *) malloc((ts_node_end_byte(n) - ts_node_start_byte(n)) + 1);
    snprintf(ret, (ts_node_end_byte(n) - ts_node_start_byte(n) + 1), "%s", source + ts_node_start_byte(n));
    ret[ts_node_end_byte(n) - ts_node_start_byte(n)] = '\0';
    return ret;
}

// fully bound column
typedef struct {
    std::string schema;
    std::string table;
    std::string column;
} STC;

/*
STC * pseudo_bind(string * tables, size_t num_tables, string column) {
    string q = "select table_schema, table_name, column_name from information_schema.columns where table_name in (";
    for(int i = 0; i < num_tables; i++) {
        q.append("'%s'", tables[i]);
    }
    q.append(") and column_name = '%s'", column);
    auto result = con.Query(q);
    if(result.RowCount() > 1) {
        printf("BINDING ERROR: column %s appears in both tables");
        exit(1);
    }
    STC * ret = (STC *) malloc(sizeof(STC));
    ret->schema = result.GetValue(0,0).ToString();
    ret->table = result.GetValue(0,1).ToString();
    ret->column = result.GetValue(0,2).ToString();
    return ret;
}

id_downstreams(STC table_ref) {
    // djkstra's out
    node cur_node;
    node_stack.push(table_ref);
    while (peek(node_stack) != null) {
        cur_node = pop(node_stack);
        if(cur_node == )
    }
    return bits;
}

highlight_downstreams(STC * columns) {
    for(c in columns) {
        std::string q = "(relation(object_reference( schema: (identifier)? table: (identifier) )@reference ))";
        ts_query(ts_root_node(tree), q.c_str(), strlen(q));
        for(int j = 0; j < q_result.capture_count; j++){
            if(strcmp(q_result.captures[i], ) == 0) {
                ts_node_start(q_result.captures[i].node);
            }
        }
    }
}
*/

std::string * get_file_order() {
    std::string *ret = (std::string *)malloc(sizeof(std::string));
    // find FROM tables, JOINed tables

    const char * q = "(relation @type ( (object_reference schema: (identifier)? table: (identifier) ) @reference))";
    //TSQuery(source, strlen(source)
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

/*
    compares the first substrlen bytes of str2 to the totality of str1 (up to the \0)
    str1 therefore needs a \0 at its end, str2 cannot. Maybe I should change this?
    Since I assume this is being called with ts_node start and end bytes feeding the
    arguments, I'm assuming these ranges won't have \0 in them (before the last character)
    if they do, we're returning FALSE. You need to know the size of the strings you're passing
    in here

    returns 1 if true, 0 otherwise
*/
int substrcmp(const char * str1, const char * str2, uint32_t substrlen) {
    int i = 0;
    while(i < substrlen) {
        if(str1[i] == '\0' || str2[i] == '\0' || str1[i] != str2[i])
            return 0;
        i++;
    }
    return 1;
}

std::string open_sqls(std::string files) {
    std::string ret;
    // open all SQL files into a buffer. Hideously inefficient, but we're small atm
    if(files == "ALL") {
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
    } else {
        printf("Looking in 'sql' file %s\n", files.c_str());
        std::ifstream fd;
        fd.open(files);
        std::string new_ret( (std::istreambuf_iterator<char>(fd) ),
                             (std::istreambuf_iterator<char>()    ) );
        ret.append(new_ret);
        fd.close();
    }
    return ret;
}

int main(int argc, char ** argv) {
    std::string files = "ALL";
    if(argc > 1)
        files = std::string(argv[1]);

    TSParser *parser = ts_parser_new();
    ts_parser_set_language(parser, tree_sitter_sql());

    std::string all_sqls = open_sqls(files);
    TSTree * tree = ts_parser_parse_string(
        parser,
        NULL,
        all_sqls.c_str(),
        strlen(all_sqls.c_str())
    );

    std::cout << all_sqls;

    TSQueryCursor * cursor = ts_query_cursor_new();
    const char * q = "(relation ( ( object_reference schema: (identifier)? name: (identifier)) @reference))";

    printf("parsed root: %s\n", ts_node_string(ts_tree_root_node(tree)));
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

    std::string * table_names;
    //fprintf(stderr, "before loop\n");
    while(ts_query_cursor_next_match(cursor, &cur_match)) {
        //fprintf(stderr, "in loop\n");
        //printf("%s", ts_node_string(cur_match.captures[0].node));
        table_names = (std::string *) malloc(sizeof(std::string) * cur_match.capture_count);
        for(int i = 0; i < cur_match.capture_count; i++) {
            TSPoint start = ts_node_start_point(cur_match.captures[i].node);
            TSPoint end = ts_node_end_point(cur_match.captures[i].node);
            printf("[%2d:%-2d - %2d:%-2d] ", start.row, start.column, end.row, end.column);
            int len = start.column - end.column;
            printf("id: %i, references: %.*s, capture: %s\n", cur_match.id,
                            ts_node_end_byte(cur_match.captures[i].node) - ts_node_start_byte(cur_match.captures[i].node),
                            all_sqls.c_str() + ts_node_start_byte(cur_match.captures[i].node),
                            //ts_node_end_byte(cur_match.captures[i+1].node) - ts_node_start_byte(cur_match.captures[i+1].node),
                            //all_sqls.c_str() + ts_node_start_byte(cur_match.captures[i+1].node),
                            ts_node_string(cur_match.captures[i].node));
            const char * q;
            //snprintf(q, "insert into table_refs values (%i, %s);", start.column - end.column, all_sqls + start);
            //con.Query(q);
        }
        //fprintf(stderr, "after print in loop\n");
    }
    //fprintf(stderr, "after loop\n");

    /* column checking
    duckdb::ColumnRef c = duckdb::ColumnRef(
    duckdb::Bind(
    */
    const char * table_were_looking_for = "etl.snapshot";
    const char * ddl = "(create_table (keyword_create) (keyword_table) (object_reference schema: (identifier)? name: (identifier)) @definition)";
    // reusing error offsets and cursors from before
    TSQuery * ddl_q = ts_query_new(tree_sitter_sql(), ddl, strlen(ddl), &q_error_offset, &q_error);
    ts_query_cursor_exec(cursor, ddl_q, ts_tree_root_node(tree));
    while(ts_query_cursor_next_match(cursor, &cur_match)) {
        for(int i = 0; i < cur_match.capture_count; i++) {
            if(substrcmp(table_were_looking_for
                        ,all_sqls.c_str() + ts_node_start_byte(cur_match.captures[i].node)
                        ,(ts_node_end_byte(cur_match.captures[i].node) - ts_node_start_byte(cur_match.captures[i].node)))) {
                            printf("FOUND THE CORRECT DDL: %.*s\n"
                                ,(ts_node_end_byte(cur_match.captures[i].node) - ts_node_start_byte(cur_match.captures[i].node))
                                ,all_sqls.c_str() + ts_node_start_byte(cur_match.captures[i].node));
                        }
        }
    }

    free(table_names);
    ts_tree_delete(tree);
    ts_parser_delete(parser);
    return 0;
}

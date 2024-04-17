#include <string>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <sstream>
// testing this out
#include <tree_sitter/api.h>
#include <sql.h>
#include <duckdb.hpp>
#include <list>

duckdb::DuckDB db(nullptr);
duckdb::Connection con(db);

bool node_compare(TSNode n1, TSNode n2) {
    return ts_node_start_byte(n1) < ts_node_start_byte(n2);
}

const char * node_to_string(const char * source, const TSNode n) {
    char * ret = (char *) malloc((ts_node_end_byte(n) - ts_node_start_byte(n)) + 1);
    snprintf(ret, (ts_node_end_byte(n) - ts_node_start_byte(n) + 1), "%s", source + ts_node_start_byte(n));
    ret[ts_node_end_byte(n) - ts_node_start_byte(n)] = '\0';
    return ret;
}

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

typedef enum { PURPLE, RED } HIGHLIGHT_COLOR;

typedef struct {
    TSNode node;
    HIGHLIGHT_COLOR color;
} node_color_map;

// precondition: highlight_token_starts is sorted
std::string format_term_highlights(std::string source, const node_color_map * highlight_tokens, uint32_t n) {
    int adj = 0;
    for(int i = 0; i < n; i++) {
        printf("%i ", i);
        if(highlight_tokens[i].color == RED) {
            source.insert(ts_node_start_byte(highlight_tokens[i].node) + adj, "\e[31m", 5);
            adj += 5;
         } else if(highlight_tokens[i].color == PURPLE) {
            source.insert(ts_node_start_byte(highlight_tokens[i].node) + adj, "\e[35m", 5);
            adj += 5;
        }
        source.insert(ts_node_end_byte(highlight_tokens[i].node) + adj, "\e[0m", 4);
        adj += 4;
    }
    return source;
}

std::string open_sqls(std::string files) {
    std::string ret;
    // open all SQL files into a buffer. Hideously inefficient, but we're small atm
    if(files == "ALL") {
        DIR* cwd = opendir("./test/");
        while(struct dirent* e = readdir(cwd)) {
            std::string a = std::string(e->d_name);
            std::cout << a;
            if(a.find(".sql") != std::string::npos) {
                printf("Looking in 'sql' file %s\n", e->d_name);
                std::ifstream fd;
                fd.open(e->d_name);
                std::string new_ret( (std::istreambuf_iterator<char>(fd) ),
                                     (std::istreambuf_iterator<char>()    ) );
                std::cout << new_ret;
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
        if(ret.length() == 0) {
            printf("ERROR: nothing read from file\n;(fixit)");
            exit(1);
        }
        fd.close();
    }
    return ret;
}

/*
char * get_source_for_field(TSNode term_stmt) {
    char * alias_ref;
    // term >> _expression >> field >> object_reference >> identifier >> _identifier
    TSQueryCursor * cursor = ts_query_cursor_new();
    const char * q = "(field ( object_reference (identifier) @table_alias ))";
    ts_query_cursor_exec(q, 53);
    alias_ref = to_string(ts_node_child_by_field_name(term_stmt, "alias_reference", ));
    TSNode create_table_stmt = term_stmt;
    ts_query_cursor_delete(cursor);
    while(term_stmt
    // TODO: unaliased (guessing / pseudo-catalog case)
}

TSNode parent_context(TSPoint clicked) {
    while(strcmp(ts_node_type(clicked), "cte") && strcmp(ts_node_type(clicked), "create_query") && strcmp(ts_node_type(clicked), "subquery"))
        clicked = ts_node_parent(clicked);
    return clicked;
}*/

std::list<TSNode> expand_select_all(TSNode star_node, const char * source);

std::list<TSNode> result_columns_for_ddl(TSNode ddl, const char * source) {
    const char * q = "(select_statement: (select (term: [(identifier) (all_fields)] @fieldname)))";
    TSQueryCursor * cursor = ts_query_cursor_new();
    uint32_t q_error_offset;
    TSQueryError q_error;
    TSQuery * selection_q = ts_query_new(tree_sitter_sql(), q, 54, &q_error_offset, &q_error);
    ts_query_cursor_exec(cursor, selection_q, ts_tree_root_node(ddl.tree));
    std::list<TSNode> field_list;
    TSQueryMatch cur_match;
    while(ts_query_cursor_next_match(cursor, &cur_match)) {
        if(!ts_node_field_name_for_child(cur_match.captures[0].node, 0), "all_fields")
            field_list.merge(expand_select_all(cur_match.captures[0].node, source), node_compare);
        field_list.push_front(cur_match.captures[0].node);
    }
    return field_list;
}

// todo: write ts_node_text_equals(TSNode n, const char * c)
// OR figure out how their query predicates work

std::list<TSNode> expand_select_all(TSNode star_node, const char * source) {
    std::list<TSNode> field_list;
    const char * reference = node_to_string(source, ts_node_child(ts_node_child(star_node, 0), 0));
    while(strcmp(ts_node_field_name_for_child(star_node, 0), "select"))
        star_node = ts_node_parent(star_node);
    TSQueryError q_error;
    uint32_t q_error_offset;
    const char * q = "(relation (object_reference schema: (identifier)? name: (identifier)) @table alias: (identifier) @alias)";
    TSQuery * references = ts_query_new(tree_sitter_sql(), q, 105, &q_error_offset, &q_error);
    TSQueryCursor * cursor = ts_query_cursor_new();
    ts_query_cursor_exec(cursor, references, star_node);
    TSQueryMatch cur_match;
    while(ts_query_cursor_next_match(cursor, &cur_match)) {
        // first predicate should only happen if all cols from all tables are selected
        if(ts_node_child_count(star_node) == 1) {
            field_list.merge(result_columns_for_ddl(cur_match.captures[0].node, source), node_compare);
        // the full table matched
        } else if(!strcmp(source + ts_node_start_byte(cur_match.captures[0].node), reference)) {
            field_list.merge(result_columns_for_ddl(cur_match.captures[0].node, source), node_compare);
            // if the reference before the * was an alias that matches this alias, get all of the fields
            // from the associated table
        } else if(!strcmp(source + ts_node_start_byte(cur_match.captures[1].node), reference))
            field_list.merge(result_columns_for_ddl(cur_match.captures[0].node, source), node_compare);

    }
    free(references);
    return field_list;
}

int main(int argc, char ** argv) {
    std::string files = "ALL";

    if(argc > 1)
        files = std::string(argv[1]);

    TSParser *parser = ts_parser_new();
    ts_parser_set_language(parser, tree_sitter_sql());

    std::string all_sqls = open_sqls(files);
    //printf("files opened\n");
    TSTree * tree = ts_parser_parse_string(
        parser,
        NULL,
        all_sqls.c_str(),
        strlen(all_sqls.c_str())
    );
    //FILE * dg = fopen("dotgraph.dot", "w");
    //ts_tree_print_dot_graph(tree, fileno(dg));
    //printf("parsed root: %s\n", ts_node_string(ts_tree_root_node(tree)));
    //printf("trees built\n");
    // reused for all queries in `main`
    TSQueryCursor * cursor = ts_query_cursor_new();
    uint32_t q_error_offset;
    TSQueryError q_error;
    TSQueryMatch cur_match;
    // this isn't reused
    char * table;
    TSQuery * cli_query;
    char * q;

    if(argc > 2) {
        // show either DDL or references for a table
        if(argc < 4 || strcmp(argv[2], "--show")) {
            printf("used wrong"); // had a \n. REMOVED IT (yeah yeah)
            exit(1);
        }
        if(!strcmp(argv[3], "ddl")) {
            printf("DDL not implemented");
            exit(1);
        } else if (!strcmp(argv[3], "references")) {
            if(argc != 6) { printf("used wrong"); exit(1); }
            if(!strcmp(argv[4], "--to")) {
                printf("in TO references\n");
                // all places this table is referenced
                table = (char *) malloc(strlen(argv[5]));
                strcpy(table, argv[5]);
                q = (char *) malloc(200);
                sprintf(q, "(relation ((object_reference schema: (identifier)%c name: (identifier)) @reference ))", '?', table);
                printf("to query: %s\n", q);
                printf("the following tables reference %s\n", table);
            } else if (!strcmp(argv[4], "--from")) {
                printf("in FROM references\n");
                // all tables this table references 
                table = (char *) malloc(strlen(argv[5]));
                strcpy(table, argv[5]);
                q = (char *) malloc(250);
                sprintf(q, "(create_table (keyword_create) (keyword_table) (object_reference schema: (identifier)? name: (identifier))@create_name\
(relation ((object_reference schema: (identifier) name: (identifier))@reference ) ))");
                printf("from query: %s\n", q);
            }
        }

        cli_query = ts_query_new(tree_sitter_sql(), q, strlen(q), &q_error_offset, &q_error);
        ts_query_cursor_exec(cursor, cli_query, ts_tree_root_node(tree));
        printf("exec-ed\n");
        int n = 0;
        while(ts_query_cursor_next_match(cursor, &cur_match)) { n += cur_match.capture_count; }
        // since TS predicates don't work we have to overallocate
        TSNode * capture_nodes = (TSNode *) malloc(sizeof(TSNode) * n);
        ts_query_cursor_exec(cursor, cli_query, ts_tree_root_node(tree));
        // don't know why they have capture count if it's just straight wrong... :/
        // this process is a massive computational waste
        n = 0;
        fprintf(stderr, "n reset: %i ", n);
        while(ts_query_cursor_next_match(cursor, &cur_match)) {
            //fprintf(stderr, "num captures (2 would mean create and ref) %i\n", cur_match.capture_count);
            for(int i = 0; i < cur_match.capture_count; i++) {
/*                if(substrcmp(table
                            ,all_sqls.c_str() + ts_node_start_byte(cur_match.captures[i].node)
                            ,(ts_node_end_byte(cur_match.captures[i].node) - ts_node_start_byte(cur_match.captures[i].node)))) {*/
                    capture_nodes[n] = cur_match.captures[i].node;
                    n++;
                //}
            }
        }
        fprintf(stderr, "n: %i ", n);
        //print_highlights_to_term(all_sqls, capture_nodes, n);
        free(capture_nodes);

        ts_query_delete(cli_query);
        free(q);
    }


    //std::cout << all_sqls;

    const char * q1 = "(relation ( object_reference schema: (identifier)? name: (identifier)) @reference alias: (identifier) @alias )";
    //const char * q1 = "(relation ( ( object_reference schema: (identifier)? name: (identifier)) @reference ))";
    //printf("q1 assigned\n");

    fprintf(stderr, "query : %s", q1);

    TSQuery * table_names_q = ts_query_new(
        tree_sitter_sql(),
        q1,
        strlen(q1),
        &q_error_offset,
        &q_error
    );

    ts_query_cursor_exec(cursor, table_names_q, ts_tree_root_node(tree));

    std::string * table_names;
    //fprintf(stderr, "before loop\n");
    while(ts_query_cursor_next_match(cursor, &cur_match)) {
        fprintf(stderr, "in loop\n");
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
            //snprintf(q, "insert into table_refs values (%i, %s);", start.column - end.column, all_sqls + start);
            //con.Query(q);
        }
        //fprintf(stderr, "after print in loop\n");
    }
    //fprintf(stderr, "after loop\n");

    /* column checking
    duckdb::ColumnRef c = duckdb::ColumnRef(
    duckdb::Bind(
    const char * table = "etl.snapshot";
    */
    const char * ddl = "(create_table (keyword_create) (keyword_table) (object_reference schema: (identifier)? name: (identifier)) @definition)";
    // reusing error offsets and cursors from before
    TSQuery * ddl_q = ts_query_new(tree_sitter_sql(), ddl, strlen(ddl), &q_error_offset, &q_error);
    ts_query_cursor_exec(cursor, ddl_q, ts_tree_root_node(tree));
    int j;
    while(ts_query_cursor_next_match(cursor, &cur_match)) {
        if(!strncmp(table
                    ,all_sqls.c_str() + ts_node_start_byte(cur_match.captures[0].node)
                    ,(ts_node_end_byte(cur_match.captures[0].node) - ts_node_start_byte(cur_match.captures[0].node)))) {
                        printf("FOUND THE CORRECT DDL: %.*s\n"
                            ,(ts_node_end_byte(cur_match.captures[0].node) - ts_node_start_byte(cur_match.captures[0].node))
                            ,all_sqls.c_str() + ts_node_start_byte(cur_match.captures[0].node));
                        //print_highlights_to_term(all_sqls, &cur_match.captures[0].node, 1);
                        printf("%i\n", j);
                        printf("%s\n", ts_node_string(ts_node_parent(cur_match.captures[0].node)));
                        break;
                    }
        j++;
    }
    printf("%i\n %s\n", j, ts_node_string(cur_match.captures[0].node));

    TSNode node = ts_node_parent(cur_match.captures[0].node);
    // this limits it to all of the direct from / joined tables (no CTEs / subqueries)
    ts_query_cursor_set_max_start_depth(cursor, 4);
    ts_query_cursor_exec(cursor, table_names_q, node);
    int nodes;
    while(ts_query_cursor_next_match(cursor, &cur_match))
        nodes += cur_match.capture_count;
    node_color_map * table_refs = (node_color_map *) malloc(sizeof(node_color_map) * nodes);
    ts_query_cursor_exec(cursor, table_names_q, node);
    int i = 0;
    while(ts_query_cursor_next_match(cursor, &cur_match)) {
        printf("capture count: %i (2 would mean alias captured)\n", cur_match.capture_count);
        node_color_map c;
        c.node = cur_match.captures[0].node;
        c.color = PURPLE;
        table_refs[i] = c;
        i++;
        if(cur_match.capture_count > 1) {
            node_color_map a;
            a.node = cur_match.captures[1].node;
            a.color = RED;
            table_refs[i] = a;
            i++;
        }
    }
    std::string f = format_term_highlights(all_sqls, table_refs, i);
    printf("%s", f.c_str());

    free(table_refs);
    ts_tree_delete(tree);
    ts_parser_delete(parser);
    return 0;
}

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

//duckdb::DuckDB db(nullptr);
//duckdb::Connection con(db);

void debug_print_node_capture_info(uint32_t id, TSNode node, std::string all_sqls) {
    printf("id: %i, references: %.*s, capture: %s\n", id,
                    ts_node_end_byte(node) - ts_node_start_byte(node),
                    all_sqls.c_str() + ts_node_start_byte(node),
                    //ts_node_end_byte(cur_match.captures[i+1].node) - ts_node_start_byte(cur_match.captures[i+1].node),
                    //all_sqls.c_str() + ts_node_start_byte(cur_match.captures[i+1].node),
                    ts_node_string(node));
    return;
}

bool node_compare(TSNode n1, TSNode n2) {
    return ts_node_start_byte(n1) < ts_node_start_byte(n2);
}

const char * node_to_string(const char * source, const TSNode n) {
    int end = ts_node_end_byte(n);
    int start = ts_node_start_byte(n);
    char * ret = (char *) malloc((end - start) + 1);
    snprintf(ret, (end - start + 1), "%s", source + start);
    ret[end - start] = '\0';
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

typedef struct {
    node_color_map * ncms;
    uint32_t length;
} node_color_map_list;

// precondition: highlight_token_starts is sorted
std::string format_term_highlights(std::string source, const node_color_map_list * highlight_tokens) {
    int adj = 0;
    for(int i = 0; i < highlight_tokens->length; i++) {
        fprintf(stderr, "%i ", i);
        if(highlight_tokens->ncms[i].color == RED) {
            source.insert(ts_node_start_byte(highlight_tokens->ncms[i].node) + adj, "\e[31m", 5);
            adj += 5;
         } else if(highlight_tokens->ncms[i].color == PURPLE) {
            source.insert(ts_node_start_byte(highlight_tokens->ncms[i].node) + adj, "\e[35m", 5);
            adj += 5;
        }
        source.insert(ts_node_end_byte(highlight_tokens->ncms[i].node) + adj, "\e[0m", 4);
        adj += 4;
    }
    return source;
}

node_color_map_list * reflist_to_highlights(std::list<TSNode> reflist) {
    node_color_map_list * ret;
    // again, alloc double since we don't know how many of these have an alias too
    ret->ncms = (node_color_map *) malloc(2 * reflist.size() * sizeof(node_color_map));
    int i = 0;
    for(TSNode ref : reflist) {
        node_color_map hl;
        hl.node = ref;
        hl.color = PURPLE;
        ret->ncms[i] = hl;
        i++;
        if(ts_node_child_count(ts_node_parent(ref)) == 2) {
            node_color_map hl2;
            hl2.node = ts_node_next_sibling(ref);
            hl2.color = RED;
            ret->ncms[i] = hl2;
            i++;
        }
    }
    ret->length = i;
    printf("conversion to highlight list successful\n");
    return ret;
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

std::list<TSNode> expand_select_star(TSNode star_node, const char * source);

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
        if(!strcmp(ts_node_field_name_for_child(cur_match.captures[0].node, 0), "all_fields"))
            field_list.merge(expand_select_star(cur_match.captures[0].node, source), node_compare);
        field_list.push_front(cur_match.captures[0].node);
    }
    return field_list;
}

// todo: write ts_node_text_equals(TSNode n, const char * c)
// OR figure out how their query predicates work

std::list<TSNode> expand_select_star(TSNode star_node, const char * source) {
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

std::list<TSNode> references_to_table(TSTree * tree, std::string code, const char * table) {
    TSQueryCursor * cursor = ts_query_cursor_new();
    uint32_t q_error_offset;
    TSQueryError q_error;
    TSQueryMatch cur_match;
    TSQuery * table_references = ts_query_new(
        tree_sitter_sql(),
        "(relation ( object_reference schema: (identifier)? name: (identifier)) @reference alias: (identifier)? @alias )",
        111,
        &q_error_offset,
        &q_error
    );
    ts_query_cursor_exec(cursor, table_references, ts_tree_root_node(tree));
    int all_references_count;
    while(ts_query_cursor_next_match(cursor, &cur_match)) { all_references_count++; }
    printf("relation references with aliases: %i\n", all_references_count);
    // bc each of these can have two captures - node and alias - we need to pre-alloc twice the space
    std::list<TSNode> reflist;

    ts_query_cursor_exec(cursor, table_references, ts_tree_root_node(tree));
    while(ts_query_cursor_next_match(cursor, &cur_match)) {
        if(!strncmp(table
                    ,code.c_str() + ts_node_start_byte(cur_match.captures[0].node)
                    ,(ts_node_end_byte(cur_match.captures[0].node) - (ts_node_start_byte(cur_match.captures[0].node))))) {
            reflist.push_front(cur_match.captures[0].node);
        }
    }
    printf("relation references to %s: %i\n", table, reflist.size());
    ts_query_cursor_delete(cursor);
    ts_query_delete(table_references);
    return reflist;
}

std::list<TSNode> references_from_table(TSTree * tree, std::string code, const char * table) {
    TSQueryCursor * cursor = ts_query_cursor_new();
    uint32_t q_error_offset;
    TSQueryError q_error;
    TSQueryMatch cur_match;
    // reusing error offsets and cursors from before
    TSQuery * create_table_q = ts_query_new(
        tree_sitter_sql(),
        "(create_table (keyword_create) (keyword_table) (object_reference schema: (identifier)? name: (identifier)) @definition)",
        119,
        &q_error_offset,
        &q_error
    );
    ts_query_cursor_exec(cursor, create_table_q, ts_tree_root_node(tree));
    int j;
    while(ts_query_cursor_next_match(cursor, &cur_match)) {
        if(!strncmp(table
                    ,code.c_str() + ts_node_start_byte(cur_match.captures[0].node)
                    ,(ts_node_end_byte(cur_match.captures[0].node) - ts_node_start_byte(cur_match.captures[0].node)))) {
                        /*printf("FOUND THE CORRECT DDL: %.*s\n"
                            ,(ts_node_end_byte(cur_match.captures[0].node) - ts_node_start_byte(cur_match.captures[0].node))
                            ,code.c_str() + ts_node_start_byte(cur_match.captures[0].node));*/
                        printf("%i\n", j);
                        printf("%s\n", ts_node_string(ts_node_parent(cur_match.captures[0].node)));
                        break;
                    }
        j++;
    }
    printf("%i\n %s\n", j, ts_node_string(cur_match.captures[0].node));

    TSNode node = ts_node_parent(cur_match.captures[0].node);

    TSQuery * table_names_q = ts_query_new(
        tree_sitter_sql(),
        "(relation ( object_reference schema: (identifier)? name: (identifier)) @reference alias: (identifier)? @alias )",
        111,
        &q_error_offset,
        &q_error
    );

    std::list<TSNode> reflist;

    // this limits it to all of the direct from / joined tables (no CTEs / subqueries)
    ts_query_cursor_set_max_start_depth(cursor, 4);
    ts_query_cursor_exec(cursor, table_names_q, node);
    while(ts_query_cursor_next_match(cursor, &cur_match)) {
        reflist.push_front(cur_match.captures[0].node);
    }

    ts_query_cursor_delete(cursor);
    ts_query_delete(table_names_q);
    ts_query_delete(create_table_q);
    return reflist;
}

// returns all tables which are downstream of the requested table
std::list<TSNode> tables_downstream_of_table(TSTree * tree, std::string code, const char * table) {
    // TODO: need to change references to and from to return lists of nodes instead of
    // void and highlight - the issue is the highlighting gets both reference and alias in
    // one go, so we need to figure out how to split that...
    
    // need to decide if convention in this app is to pass references to relations around
    // by their tightest node handle (just the table) or the handle for the table *and* the
    // alias. Space wise it's a wash

    // I kind of like the idea of always passing the aliases too and then having a conversion
    // between a list of ref nodes with aliases to a list of node_color_maps
    std::list<TSNode> reflist;
    std::list<TSNode> dfs = references_to_table(tree, code, table);
    while(dfs.size() > 0) {
        const char * table = node_to_string(code.c_str(), dfs.front());
        TSNode next_table = ts_node_parent(dfs.front());
        //printf("iterating upwards:\tlooking for symbol id: %i\t%s\n", create_table_symbol, ts_language_symbol_name(tree_sitter_sql(), ts_node_symbol(next_table)));
        //TSSymbol create_table_symbol = ts_language_symbol_for_name(tree_sitter_sql(), "create_table", 12, false);
        // hard-coding this in after printing it out. Confused why ts_language_symbol_for_name isn't working
        while( ts_node_symbol(next_table) != 478 ) {
            printf("iterating upwards:\tsymbol id: %i\t%s\n", ts_node_symbol(next_table), ts_language_symbol_name(tree_sitter_sql(), ts_node_symbol(next_table)));
            next_table = ts_node_parent(next_table);
        }
        // tree structure of a create_table statement:
        // (create_table (keyword_create) (keyword_table) (object_reference schema: (identifier)? name: (identifier)) @definition)
        // we want the object_reference part
        next_table = ts_node_child(next_table, 2);
        // this returns the create_table DDL for each of the downstream tables
        reflist.push_front(next_table);
        printf("looking for references to %s, next DDL to search: %s\n", table,  node_to_string(code.c_str(), next_table));
        std::list<TSNode> next_up = references_to_table(tree, code, node_to_string(code.c_str(), next_table));
        dfs.pop_front();
        reflist.merge(next_up, node_compare);
        dfs.merge(next_up, node_compare);
    }
    printf("# downstream tables: %i\n", reflist.size());
    return reflist;
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

    if(argc > 2) {
        // show either DDL or references for a table
        if(argc < 4 || strcmp(argv[2], "--show")) {
            printf("used wrong"); // had a \n. REMOVED IT (yeah yeah)
            exit(1);
        }

        // what does "DDL" mean?
        if(!strcmp(argv[3], "ddl")) {
            printf("DDL not implemented");
            exit(1);
        } else if (!strcmp(argv[3], "references")) {
            if(argc != 6) { printf("used wrong"); exit(1); }
            if(!strcmp(argv[4], "--to")) {
                printf("in TO references\n");
                // all places this table is referenced
                printf("the following tables reference %s\n", argv[5]);
                std::list<TSNode> to_reflist = references_to_table(tree, all_sqls, argv[5]);
                to_reflist.sort(node_compare);
                node_color_map_list * to_highlights = reflist_to_highlights(to_reflist);
                printf("%s\n", format_term_highlights(all_sqls, to_highlights).c_str());
                free(to_highlights->ncms);
            } else if (!strcmp(argv[4], "--from")) {
                printf("in FROM references\n");
                // all tables this table references 
                printf("the following tables are referenced from %s\n", argv[5]);
                std::list<TSNode> from_reflist = references_from_table(tree, all_sqls, argv[5]);
                from_reflist.sort(node_compare);
                node_color_map_list * from_highlights = reflist_to_highlights(from_reflist);
                std::string ret = format_term_highlights(all_sqls, from_highlights);
                printf("highlights formatted\n");
                printf("%s\n", ret.c_str());
                free(from_highlights->ncms);
                fprintf(stderr, "after printing full code\n");
            }
        } else if (!strcmp(argv[3], "downstream")) {
            if (argc != 6) { printf("used wrong"); exit(1); }
            if (!strcmp(argv[4], "--of")) {
                printf("in downstream of\n");
                std::list<TSNode> downstream_reflist = tables_downstream_of_table(tree, all_sqls, argv[5]);
                downstream_reflist.sort(node_compare);
                node_color_map_list * downstream_highlights = reflist_to_highlights(downstream_reflist);
                printf("%s\n", format_term_highlights(all_sqls, downstream_highlights).c_str());
                free(downstream_highlights->ncms);
            } else {
                printf("used wrong"); exit(1);
            }
        }
        fprintf(stderr, "still handling args\n");
    }

    fprintf(stderr, "back in body of main\n");

    //std::cout << all_sqls;

    const char * q1 = "(relation ( object_reference schema: (identifier)? name: (identifier)) @reference alias: (identifier) @alias )";
    //const char * q1 = "(relation ( ( object_reference schema: (identifier)? name: (identifier)) @reference ))";
    printf("q1 assigned\n");

    fprintf(stderr, "query : %s\n", q1);

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
        //fprintf(stderr, "in loop\n");
        //printf("%s", ts_node_string(cur_match.captures[0].node));
        table_names = (std::string *) malloc(sizeof(std::string) * cur_match.capture_count);
        for(int i = 0; i < cur_match.capture_count; i++) {
            TSPoint start = ts_node_start_point(cur_match.captures[i].node);
            TSPoint end = ts_node_end_point(cur_match.captures[i].node);
            printf("[%2d:%-2d - %2d:%-2d] ", start.row, start.column, end.row, end.column);
            int len = start.column - end.column;
            debug_print_node_capture_info(cur_match.id, cur_match.captures[i].node, all_sqls);
        }
        //fprintf(stderr, "after print in loop\n");
    }
    //fprintf(stderr, "after loop\n");

    /* column checking
    duckdb::ColumnRef c = duckdb::ColumnRef(
    duckdb::Bind(
    const char * table = "etl.snapshot";
    */
    ts_tree_delete(tree);
    ts_parser_delete(parser);
    return 0;
}

#include <string>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <sstream>
// testing this out
#include <tree_sitter/api.h>
#include <sql.h>
#include <list>

void debug_rint_node_capture_info(uint32_t id, TSNode node, std::string all_sqls) {
    printf("id: %i, references: %.*s, capture: %s\n", id,
                    ts_node_end_byte(node) - ts_node_start_byte(node),
                    all_sqls.c_str() + ts_node_start_byte(node),
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
std::string format_term_highlights(std::string source, const node_color_map_list highlight_tokens) {
    int adj = 0;
    for(int i = 0; i < highlight_tokens.length; i++) {
        //fprintf(stderr, "%i ", i);
        if(highlight_tokens.ncms[i].color == RED) {
            source.insert(ts_node_start_byte(highlight_tokens.ncms[i].node) + adj, "\e[31m", 5);
            adj += 5;
         } else if(highlight_tokens.ncms[i].color == PURPLE) {
            source.insert(ts_node_start_byte(highlight_tokens.ncms[i].node) + adj, "\e[35m", 5);
            adj += 5;
        }
        source.insert(ts_node_end_byte(highlight_tokens.ncms[i].node) + adj, "\e[0m", 4);
        adj += 4;
    }
    return source;
}

node_color_map_list reflist_to_highlights(std::list<TSNode> reflist) {
    node_color_map_list ret;
    // again, alloc double since we don't know how many of these have an alias too
    ret.ncms = (node_color_map *) malloc(2 * reflist.size() * sizeof(node_color_map));
    int i = 0;
    for(TSNode ref : reflist) {
        node_color_map hl;
        hl.node = ref;
        hl.color = PURPLE;
        ret.ncms[i] = hl;
        i++;
        if(ts_node_child_count(ts_node_parent(ref)) == 2) {
            node_color_map hl2;
            hl2.node = ts_node_next_sibling(ref);
            hl2.color = RED;
            ret.ncms[i] = hl2;
            i++;
        }
    }
    ret.length = i;
    //printf("conversion to highlight list successful\n");
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
                //printf("Looking in 'sql' file %s\n", e->d_name);
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
        //printf("Looking in 'sql' file %s\n", files.c_str());
        std::ifstream fd;
        fd.open(files);
        std::string new_ret( (std::istreambuf_iterator<char>(fd) ),
                             (std::istreambuf_iterator<char>()    ) );
        ret.append(new_ret);
        if(ret.length() == 0) {
            //printf("ERROR: nothing read from file\n;(fixit)");
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
*/

TSNode parent_context(TSTree * tree, TSPoint clicked) {
    TSTreeCursor tc = ts_tree_cursor_new(ts_tree_root_node(tree));
    ts_tree_cursor_goto_first_child_for_point(&tc, clicked);
    TSNode n = ts_tree_cursor_current_node(&tc);
    // 482 = create_query, 460 = CTE, 628 = subuquery
    while(ts_tree_cursor_goto_parent(&tc)) {
        n = ts_tree_cursor_current_node(&tc);
        TSSymbol s = ts_node_symbol(n);
        if(s == 482 || s == 460 || s == 628)
            printf("row %i, cols %i-%i", ts_node_start_point(n).row, ts_node_start_point(n).column, ts_node_end_point(n).column);
            break;
    }
    ts_tree_cursor_delete(&tc);
    return n;
}

// gets the object_reference node for the create table statement for a table
// returns the root node of the program if the exact table name searched for isn't found
TSNode create_table_node_for_table_name(const TSTree * tree, std::string code, const char * table){
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
    int found = 0;
    ts_query_cursor_exec(cursor, create_table_q, ts_tree_root_node(tree));
    while(ts_query_cursor_next_match(cursor, &cur_match)) {
        if(!strncmp(table
                    ,code.c_str() + ts_node_start_byte(cur_match.captures[0].node)
                    ,(ts_node_end_byte(cur_match.captures[0].node) - ts_node_start_byte(cur_match.captures[0].node)))) {
                        found = 1;
                        break;
                    }
    }

    if (!found)
        return ts_tree_root_node(tree);
    //printf("after the earthquake (DDL drizzy found)\n");
    TSNode ret = cur_match.captures[0].node; 
    ts_query_cursor_delete(cursor);
    ts_query_delete(create_table_q);
    return ret;
}


std::list<TSNode> expand_select_star(TSNode star_node, const char * source);

// takes a TSNode at a "create_table" node, returns a list of column names
std::list<TSNode> result_columns_for_ddl(TSNode ddl, const char * source) {
    std::list<TSNode> ret;
    char q[121];
    TSNode ddl_body = ddl;
    int i = 0;
    while(i < ts_node_child_count(ts_node_parent(ddl))) {
        //fprintf(stderr, "ts_node_symbol: %s (id: %i)\n", ts_language_symbol_name(tree_sitter_sql(), ts_node_symbol(ddl_body)), ts_node_symbol(ddl_body));
        if(ts_node_symbol(ddl_body) == 482) {
            strcpy(q, "(select (select_expression (term [value: (field . name: (identifier) . ) alias: (identifier) (all_fields)] @fieldname)))");
            break;
        }
        // handle the case that this is a straight up list of columns not create_query
        else if(ts_node_symbol(ddl_body) == 577) {
            strcpy(q, "(column_definitions (column_definition name: (identifier) @col_def))");
            break;
        }
        ddl_body = ts_node_next_sibling(ddl_body);
        i++;
    }
    if(i == ts_node_child_count(ts_node_parent(ddl))) {
        printf("ERROR: ddl node doesn't have a create_query or column_definitions child node\n");
        exit(1);
    }
    //fprintf(stderr, "looking for columns in this tree:\n%s\n", ts_node_string(ts_node_parent(ddl)));
    TSQueryCursor * cursor = ts_query_cursor_new();
    uint32_t q_error_offset;
    TSQueryError q_error;
    TSQuery * selection_q = ts_query_new(tree_sitter_sql(), q, strlen(q), &q_error_offset, &q_error);
    ts_query_cursor_exec(cursor, selection_q, ts_node_parent(ddl));
    TSQueryMatch cur_match;
    while(ts_query_cursor_next_match(cursor, &cur_match)) {
        //fprintf(stderr, "cursor iterating");
        //fprintf(stderr, "ts_node_symbol: %s (id: %i)\n", ts_language_symbol_name(tree_sitter_sql(), ts_node_symbol(cur_match.captures[0].node)), ts_node_symbol(cur_match.captures[0].node));
        if(ts_node_symbol(cur_match.captures[0].node) == 590)
            ret.merge(expand_select_star(cur_match.captures[0].node, source), node_compare);
        else
            ret.push_front(cur_match.captures[0].node);
    }
    ts_query_cursor_delete(cursor);
    ts_query_delete(selection_q);
    //fprintf(stderr, "done iterating\n");
    return ret;
}

// todo: write ts_node_text_equals(TSNode n, const char * c)
// OR figure out how their query predicates work

std::list<TSNode> expand_select_star(TSNode star_node, const char * source) {
    //fprintf(stderr, "in expand select star. star_node children: %i\nstar_node: %s", ts_node_child_count(star_node), ts_node_string(star_node));
    std::list<TSNode> field_list;
    const char * reference = "-1";
    if(ts_node_child_count(star_node) > 1)
        reference = node_to_string(source, ts_node_child(ts_node_child(star_node, 0), 0));
        //fprintf(stderr, "table specifier on select all\n");
    //fprintf(stderr, "ts_node_symbol: %s (id: %i) reference: %s. ref len = %i\n"
            //,ts_language_symbol_name(tree_sitter_sql(), ts_node_symbol(star_node)), ts_node_symbol(star_node), reference, strlen(reference));
    // "select" node id is 
    while(ts_node_symbol(star_node) != 482) {
        //fprintf(stderr, "ts_node_symbol: %s (id: %i)\n", ts_language_symbol_name(tree_sitter_sql(), ts_node_symbol(star_node)), ts_node_symbol(star_node));
        star_node = ts_node_parent(star_node);
    }
    TSQueryError q_error;
    uint32_t q_error_offset;
    const char * q = "(relation (object_reference schema: (identifier)? name: (identifier)) @table alias: (identifier) @alias)";
    TSQuery * references_q = ts_query_new(tree_sitter_sql(), q, 104, &q_error_offset, &q_error);
    TSQueryCursor * cursor = ts_query_cursor_new();
    ts_query_cursor_exec(cursor, references_q, star_node);
    //fprintf(stderr, "exec-ed\n");
    TSQueryMatch cur_match;
    std::string ssource = source;
    int i = 0;
    while(ts_query_cursor_next_match(cursor, &cur_match)) {
        TSNode ddl_node = create_table_node_for_table_name(star_node.tree, ssource, node_to_string(source, cur_match.captures[0].node));
        //fprintf(stderr, "match %i: %s , %s\n", i, node_to_string(source, cur_match.captures[0].node), node_to_string(source, cur_match.captures[1].node));
        // first predicate should only happen if all cols from all tables are selected
        if(!strcmp("-1", reference)) {
            field_list.merge(result_columns_for_ddl(ddl_node, source), node_compare);
            //fprintf(stderr, "merging field list:\n");
        // the full table matched
        } else if(!strncmp(source + ts_node_start_byte(cur_match.captures[0].node), reference, strlen(reference))) {
            //fprintf(stderr, "matched full table name: %s:\n", reference);
            field_list.merge(result_columns_for_ddl(ddl_node, source), node_compare);
            // if the reference before the * was an alias that matches this alias, get all of the fields
            // from the associated table
        } else if(!strncmp(source + ts_node_start_byte(cur_match.captures[1].node), reference, strlen(reference))) {
            //fprintf(stderr, "matched table alias: %s:\n", reference);
            field_list.merge(result_columns_for_ddl(ddl_node, source), node_compare);
        }
        i++;
    }
    free(references_q);
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
    //printf("relation references to %s: %i\n", table, reflist.size());
    ts_query_cursor_delete(cursor);
    ts_query_delete(table_references);
    return reflist;
}

std::list<TSNode> references_from_table(TSTree * tree, std::string code, const char * table) {
    TSQueryCursor * cursor = ts_query_cursor_new();
    uint32_t q_error_offset;
    TSQueryError q_error;
    TSQueryMatch cur_match;

    TSNode node = ts_node_parent(create_table_node_for_table_name(tree, code, table));

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
    return reflist;
}

// returns all tables which are downstream of the requested table
std::list<TSNode> tables_downstream_of_table(TSTree * tree, std::string code, const char * table) {
    // TODO: done. See old commits for thought process
    std::list<TSNode> reflist;
    std::list<TSNode> dfs = references_to_table(tree, code, table);
    while(dfs.size() > 0) {
        TSNode next_table = dfs.front();
        dfs.pop_front();
        //printf("iterating upwards:\tlooking for symbol id: %i\t%s\n", create_table_symbol, ts_language_symbol_name(tree_sitter_sql(), ts_node_symbol(next_table)));
        //TSSymbol create_table_symbol = ts_language_symbol_for_name(tree_sitter_sql(), "create_table", 12, false);
        // hard-coding this in after printing it out. Confused why ts_language_symbol_for_name isn't working
        while( ts_node_symbol(next_table) != 478 ) {
            next_table = ts_node_parent(next_table);
        }
        // tree structure of a create_table statement:
        // (create_table (keyword_create) (keyword_table) (object_reference schema: (identifier)? name: (identifier)) @definition)
        // we want the object_reference part
        next_table = ts_node_child(next_table, 2);
        // this returns the create_table DDL for each of the downstream tables
        reflist.push_front(next_table);
        //fprintf(stderr, "looking for references to %s, next DDL to search: %s\n", node_to_string(code.c_str(), dfs.front()),  node_to_string(code.c_str(), next_table));
        std::list<TSNode> next_up = references_to_table(tree, code, node_to_string(code.c_str(), next_table));
        // so turns out LIST1.merge(LIST2) is not a "pure function" - it deletes everything from LIST2
        // good news is merging with reflist wasn't even logically accurate - we add highlights when we 
        // get them by the create_table handle, not what's returned from refs_to_table, which is object_reference nodes
        dfs.merge(next_up, node_compare);
    }
    return reflist;
}

std::list<TSNode> tables_upstream_of_table(TSTree * tree, std::string code, const char * table) {
    std::list<TSNode> reflist;
    std::list<TSNode> dfs = references_from_table(tree, code, table);
    while(dfs.size() > 0) {
        TSNode next_table = dfs.front();
        dfs.pop_front();
        reflist.push_front(next_table);
        fprintf(stderr, "Everything but merging. Node: %s\n", node_to_string(code.c_str(), next_table));
        std::list<TSNode> refs = references_from_table(tree, code, node_to_string(code.c_str(), next_table));
        if(refs.size())
            dfs.merge(refs, node_compare);
        fprintf(stderr, "Merged\n");
    }
    fprintf(stderr, "Returning from upstream of\n");
    return reflist;
}

typedef struct {
    int * points;
    int   size;
} cd_nodelist;

extern "C" {
    cd_nodelist references_from_table_c(const char * code, const char * table) {
        cd_nodelist ret;
        std::string _code = code;
        // crazy inefficient to do this again - neovim already maintains a tree,
        // but let's just get it working
        TSParser * parser = ts_parser_new();
        ts_parser_set_language(parser, tree_sitter_sql());

        TSTree * tree = ts_parser_parse_string(
            parser,
            NULL,
            code,
            strlen(code)
        );

        std::list<TSNode> res = references_from_table(tree, _code, table);
        // janky way to do block of int[3]s
        ret.points = (int *)malloc(sizeof(int) * 3 * res.size());
        int i = 0;
        for(TSNode n: res) {
            ret.points[(i * 3) + 0] = ts_node_start_point(n).row;
            ret.points[(i * 3) + 1] = ts_node_start_point(n).column;
            ret.points[(i * 3) + 2] =   ts_node_end_point(n).column;
            i++;
        }
        ret.size = i;

        ts_tree_delete(tree);
        ts_parser_delete(parser);
        return ret;
    }

    cd_nodelist references_to_table_c(const char * code, const char * table) {
        cd_nodelist ret;
        std::string _code = code;
        // crazy inefficient to do this again - neovim already maintains a tree,
        // but let's just get it working
        TSParser * parser = ts_parser_new();
        ts_parser_set_language(parser, tree_sitter_sql());

        TSTree * tree = ts_parser_parse_string(
            parser,
            NULL,
            code,
            strlen(code)
        );

        std::list<TSNode> res = references_to_table(tree, _code, table);
        // janky way to do block of int[3]s
        ret.points = (int *)malloc(sizeof(int) * 3 * res.size());
        int i = 0;
        for(TSNode n: res) {
            ret.points[(i * 3) + 0] = ts_node_start_point(n).row;
            ret.points[(i * 3) + 1] = ts_node_start_point(n).column;
            ret.points[(i * 3) + 2] =   ts_node_end_point(n).column;
            i++;
        }
        ret.size = i;

        ts_tree_delete(tree);
        ts_parser_delete(parser);
        return ret;
    }

    cd_nodelist tables_downstream_of_table_c(const char * code, const char * table) {
        cd_nodelist ret;
        std::string _code = code;
        TSParser * parser = ts_parser_new();
        ts_parser_set_language(parser, tree_sitter_sql());

        TSTree * tree = ts_parser_parse_string(
            parser,
            NULL,
            code,
            strlen(code)
        );

        std::list<TSNode> res = tables_downstream_of_table(tree, _code, table);
        // janky way to do block of int[3]s
        ret.points = (int *)malloc(sizeof(int) * 3 * res.size());
        int i = 0;
        for(TSNode n: res) {
            ret.points[(i * 3) + 0] = ts_node_start_point(n).row;
            ret.points[(i * 3) + 1] = ts_node_start_point(n).column;
            ret.points[(i * 3) + 2] =   ts_node_end_point(n).column;
            i++;
        }
        ret.size = i;

        ts_tree_delete(tree);
        ts_parser_delete(parser);
        return ret;
    }

    cd_nodelist tables_upstream_of_table_c(const char * code, const char * table) {
        cd_nodelist ret;
        std::string _code = code;
        TSParser * parser = ts_parser_new();
        ts_parser_set_language(parser, tree_sitter_sql());
        TSTree * tree = ts_parser_parse_string(
            parser,
            NULL,
            code,
            strlen(code)
        );

        std::list<TSNode> res = tables_upstream_of_table(tree, _code, table);
        ret.points = (int *)malloc(sizeof(int) * 3 * res.size());
        int i = 0;
        for(TSNode n: res) {
            ret.points[(i * 3) + 0] = ts_node_start_point(n).row;
            ret.points[(i * 3) + 1] = ts_node_start_point(n).column;
            ret.points[(i * 3) + 2] =   ts_node_end_point(n).column;
            i++;
        }
        ret.size = i;

        ts_tree_delete(tree);
        ts_parser_delete(parser);
        return ret;
    }

    typedef struct {
        char ** fields;
        int     size;
    } cd_stringlist;

    cd_stringlist result_columns_for_table_c(const char * code, const char * table) {
        cd_stringlist ret;

        std::string _code = code;
        TSParser * parser = ts_parser_new();
        ts_parser_set_language(parser, tree_sitter_sql());

        TSTree * tree = ts_parser_parse_string(
            parser,
            NULL,
            code,
            strlen(code)
        );

        TSNode ddl = create_table_node_for_table_name(tree, _code, table);

        std::list<TSNode> res = result_columns_for_ddl(ddl, code);
        // janky way to do block of int[3]s
        ret.fields = (char **)malloc(sizeof(char *) * res.size());
        int i = 0;
        for(TSNode n: res) {
            ret.fields[i] = (char *)malloc(ts_node_end_byte(n) - ts_node_start_byte(n));
            strcpy(ret.fields[i], node_to_string(code, n));
            i++;
        }
        ret.size = i;

        ts_tree_delete(tree);
        ts_parser_delete(parser);
        return ret;
    }

    // do the dumb recalc the tree thing again
    cd_nodelist parent_context_c(const char * code, TSPoint clicked) {
        cd_nodelist ret;
        TSParser * parser = ts_parser_new();
        ts_parser_set_language(parser, tree_sitter_sql());
        TSTree * tree = ts_parser_parse_string(
            parser,
            NULL,
            code,
            strlen(code)
        );
        TSNode n = parent_context(tree, clicked);
        ret.points = (int *)malloc(sizeof(int) * 3);
        ret.points[0] = ts_node_start_point(n).row;
        ret.points[1] = ts_node_start_point(n).column;
        ret.points[2] =   ts_node_end_point(n).column;
        ret.size = 1;

        ts_tree_delete(tree);
        ts_parser_delete(parser);
        return ret;
    }
}

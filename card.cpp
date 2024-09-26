#include <string>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <sstream>
// testing this out
#include <tree_sitter/api.h>
#include <sql.h>
#include <list>
#include <set>

void debug_print_node_capture_info(uint32_t id, TSNode node, std::string all_sqls) {
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

// node symbol number definitions
#define OBJ_REF_NODE      550
#define IDENTIFIER_NODE   637
#define CREATE_QUERY_NODE 482
#define CTE_NODE          460
#define SUBQUERY_NODE     628
#define PROGRAM_NODE      393

typedef struct {
    const char *    source;
    TSLanguage *    language;
    TSParser *      parser;
    TSTree *        tree;
    TSQueryCursor * cursor;
    TSQuery *       REFERENCES_FROM_Q;
    TSQuery *       REFERENCES_TO_Q;
    TSQuery *       CONTEXT_DEF_TABLE_Q;
    TSQuery *       CONTEXT_DEF_SUB_Q;
    TSQuery *       FIELD_DEF_Q;
    TSQuery *       COLUMN_DEF_Q;
} card_runtime;

card_runtime * card_runtime_init(const char * source) {
    card_runtime * ret = (card_runtime *)malloc(sizeof(card_runtime));
    ret->source = source;
    ret->language = tree_sitter_sql();
    ret->parser = ts_parser_new();
    ts_parser_set_language(ret->parser, ret->language);
    ret->tree = ts_parser_parse_string(
        ret->parser,
        NULL,
        source,
        strlen(source)
    );
    // TODO: deserialize this instead of alloc
    ret->cursor = ts_query_cursor_new();
    char buf[500];
    FILE* QUERY_FILE = fopen("card_queries.tsq", "r");
    fread(buf, 1, 355, QUERY_FILE);
    ret->REFERENCES_FROM_Q = ts_query_deserialize(buf, ret->language);
    /*
    const char * cdtq = "(create_table (object_reference schema: (identifier)? name: (identifier)) @definition)";
    ret->CONTEXT_DEF_TABLE_Q = ts_query_new(
        ret->language,
        cdtq,
        strlen(cdtq),
        &q_error_offset,
        &q_error
    );
    const char * cdsq = "[(cte (identifier) @definition) ((subquery) (keyword_as)? (identifier) @definition)]";
    ret->CONTEXT_DEF_SUB_Q = ts_query_new(
        ret->language,
        cdsq,
        strlen(cdsq),
        &q_error_offset,
        &q_error
    );
    const char * fldq = "(select (select_expression (term [value: (field . name: (identifier) . ) alias: (identifier) (all_fields)] @fieldname)))";
    ret->FIELD_DEF_Q = ts_query_new(
        ret->language,
        fldq,
        strlen(fldq),
        &q_error_offset,
        &q_error
    );
    const char * colq = "(column_definitions (column_definition name: (identifier) @col_def))";
    ret->COLUMN_DEF_Q = ts_query_new(
        ret->language,
        colq,
        strlen(colq),
        &q_error_offset,
        &q_error
    );
    */
    return ret;
}

void card_runtime_deinit(card_runtime * r) {
    ts_query_delete(r->REFERENCES_FROM_Q);
    ts_query_delete(r->REFERENCES_TO_Q);
    ts_query_delete(r->CONTEXT_DEF_TABLE_Q);
    ts_query_delete(r->CONTEXT_DEF_SUB_Q);
    ts_query_delete(r->FIELD_DEF_Q);
    ts_query_delete(r->COLUMN_DEF_Q);
    ts_query_cursor_delete(r->cursor);
    ts_tree_delete(r->tree);
    ts_parser_delete(r->parser);
    free(r);
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

std::list<std::string> get_table_names(card_runtime * r) {
    TSQueryMatch table_match;

    ts_query_cursor_exec(r->cursor, r->CONTEXT_DEF_TABLE_Q, ts_tree_root_node(r->tree));
    std::list<std::string> ret;
    while(ts_query_cursor_next_match(r->cursor, &table_match)) {
        std::string s = node_to_string(r->source, table_match.captures[0].node);
        ret.push_back(s);
    }
    return ret;
}

TSNode parent_context(const TSTree * tree, TSPoint clicked) {
    TSNode n = ts_node_descendant_for_point_range(ts_tree_root_node(tree), clicked, clicked);
    //fprintf(stderr, "parent context starting node %s\n", node_to_string(n));
    // 478 = create_table object_reference, 482 = create_query, 460 = CTE, 628 = subquery, 393 = program (root)
    TSSymbol s;
    while(ts_node_symbol(n) != 393) {
        n = ts_node_parent(n);
        s = ts_node_symbol(n);
        //printf("node symbol: %s - %i\n", ts_language_symbol_name(ts_tree_language(tree), ts_node_symbol(n)), s);
        if(s == 393 || s == 478 || s == 460 || s == 628) {
            break;
        }
    }
    if(s == 478)
        n = ts_node_child(n, 2);
    if(s == CTE_NODE)
        n = ts_node_child(n, 0);
    if(s == SUBQUERY_NODE) {
        while(ts_node_symbol(n) != IDENTIFIER_NODE)
            n = ts_node_next_sibling(n);
    }
    s = ts_node_symbol(n);
    //printf("node symbol: %s - %i\n", ts_language_symbol_name(ts_tree_language(tree), s), s);
    if(s != OBJ_REF_NODE && s != IDENTIFIER_NODE) {
        printf("ERROR: parent_context returned non object_reference / identifier node %i\n", s);
        //exit(1);
    }

    return n;
}

// gets the object_reference node for the create table statement for a table
// returns the root node of the program if the exact table name searched for isn't found
//
// Expanding the usage of this function so it can be called from any of 
//  1) inside a table def to get a CTE / subquery def 
//  2) globally as a helper function
//  3) globally from a text search bar in an editor
// parent node will be set to tree_root for queries that are obviously going to be 
// going to have to extend the neovim extension to get the node at the point of the visual cursor,
// not just the text there. Back to the forums!
TSNode context_definition(card_runtime * r, TSNode parent, const char * context_name){
    TSQueryMatch table_match;
    int table_found = 0;

    // so, awkward, but we're just doing two searches and comparing the results. First search is
    // legacy code to get global table definitions, second will be local for "subcontexts"
    ts_query_cursor_exec(r->cursor, r->CONTEXT_DEF_TABLE_Q, ts_tree_root_node(r->tree));
    while(ts_query_cursor_next_match(r->cursor, &table_match)) {
        if(!strncmp(context_name
                    ,r->source + ts_node_start_byte(table_match.captures[0].node)
                    ,(ts_node_end_byte(table_match.captures[0].node) - ts_node_start_byte(table_match.captures[0].node)))) {
                        table_found = 1;
                        break;
                    }
    }
    
    int sub_found = 0;
    TSQueryMatch sub_match;
    // HACK - tables take precedence
    if(!table_found && ts_node_symbol(parent) != PROGRAM_NODE) {
        // by all known laws of SQL there can't be a subquery and CTE with the same name - CONFIRMED in DuckDB. Binder Error
        // Turns out we can have freaking "arbitrarily" nested select statements. Might have to get logically gross here
        ts_query_cursor_set_max_start_depth(r->cursor, 5);
        ts_query_cursor_exec(r->cursor, r->CONTEXT_DEF_SUB_Q, parent);
        //fprintf(stderr, "in context_defintion, exec-ed subquery search, looking for %s\n", context_name);
        while(ts_query_cursor_next_match(r->cursor, &sub_match)) {
            if(!strncmp(context_name
                        ,r->source + ts_node_start_byte(sub_match.captures[0].node)
                        ,(ts_node_end_byte(sub_match.captures[0].node) - ts_node_start_byte(sub_match.captures[0].node)))) {
                            sub_found = 1;
                            //fprintf(stderr, "in context_defintion, subquery FOUND\n");
                            break;
                        }
        }
    }

    TSNode ret = ts_tree_root_node(r->tree);
    if (table_found)
        ret = table_match.captures[0].node;
    else if (sub_found)
        ret = sub_match.captures[0].node;
    // if it doesn't hit either `if` we know there aren't any tables, relevant CTEs or subqueries with that name
    // ret will stay the default of the whole parse tree's root, which should be handled by caller
    //fprintf(stderr, "returning from context_defintion\n");

    //printf("after the earthquake (DDL drizzy found)\n");
    return ret;
}

TSNode context_ddl(card_runtime * r, TSPoint p, const char * context_name) {
    TSNode hl_node;
    if(p.row == 0 && p.column == 0)
        hl_node = ts_tree_root_node(r->tree);
    else {
        hl_node = parent_context(r->tree, p);
        if(ts_node_symbol(hl_node) == OBJ_REF_NODE)
            hl_node = ts_node_parent(hl_node);
        else if(ts_node_symbol(hl_node) == IDENTIFIER_NODE) {
            //fprintf(stderr, "identifier node: %s\n", node_to_string(source, hl_node));
            if(ts_node_symbol(ts_node_parent(hl_node)) == CTE_NODE) {
                hl_node = ts_node_parent(hl_node);
            } else {
                hl_node = ts_node_parent(ts_node_parent(hl_node));
            }
        }
    }
    TSNode def_name_node = context_definition(r, hl_node, context_name);
    /*fprintf(stderr, "node: %s ts_node_symbol: %s (id: %i)\n", node_to_string(source, def_name_node)
                    ,ts_language_symbol_name(tree_sitter_sql(), ts_node_symbol(def_name_node)), ts_node_symbol(def_name_node));*/
    if(ts_node_symbol(def_name_node) == 393) {
        fprintf(stderr, "ERROR: context by name %s not found in the source code\n", context_name);
        exit(1);
    }
    TSNode ret;
    if(ts_node_symbol(def_name_node) == OBJ_REF_NODE)
        ret = ts_node_parent(def_name_node);
    else if(ts_node_symbol(def_name_node) == IDENTIFIER_NODE) {
        // this should only be reached when looking for references inside a subquery
        // BUT its getting called on cust_level - a CTE reference. That shouldn't happen
        // figured it out
        // references TO a CTE are object references, the *name* of a CTE where its created is an identifier
        // as compared to the *name* of create_table table which is itself an object_reference
        if(ts_node_symbol(ts_node_parent(def_name_node)) == CTE_NODE) {
            ret = ts_node_parent(def_name_node);
        } else {
            ret = ts_node_prev_sibling(ts_node_prev_sibling(def_name_node));
        }
    }

    return ret;
}


std::list<TSNode> expand_select_star(card_runtime * r, TSNode star_node);

// takes a TSNode at a "create_table" node, returns a list of column names
std::list<TSNode> result_columns_for_ddl(card_runtime * r, TSNode ddl) {
    std::list<TSNode> ret;
    TSQuery * selection_q;
    TSNode ddl_body = ddl;
    int i = 0;
    while(i < ts_node_child_count(ts_node_parent(ddl))) {
        //fprintf(stderr, "ts_node_symbol: %s (id: %i)\n", ts_language_symbol_name(tree_sitter_sql(), ts_node_symbol(ddl_body)), ts_node_symbol(ddl_body));
        if(ts_node_symbol(ddl_body) == 482) {
            selection_q = r->FIELD_DEF_Q;
            break;
        }
        // handle the case that this is a straight up list of columns not create_query
        else if(ts_node_symbol(ddl_body) == 577) {
            selection_q = r->COLUMN_DEF_Q;
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
    ts_query_cursor_exec(r->cursor, selection_q, ts_node_parent(ddl));
    TSQueryMatch cur_match;
    while(ts_query_cursor_next_match(r->cursor, &cur_match)) {
        //fprintf(stderr, "cursor iterating");
        //fprintf(stderr, "ts_node_symbol: %s (id: %i)\n", ts_language_symbol_name(tree_sitter_sql(), ts_node_symbol(cur_match.captures[0].node)), ts_node_symbol(cur_match.captures[0].node));
        if(ts_node_symbol(cur_match.captures[0].node) == 590)
            ret.merge(expand_select_star(r, cur_match.captures[0].node), node_compare);
        else
            ret.push_front(cur_match.captures[0].node);
    }
    //fprintf(stderr, "done iterating\n");
    return ret;
}

// todo: write ts_node_text_equals(TSNode n, const char * c)
// OR figure out how their query predicates work

std::list<TSNode> expand_select_star(card_runtime * r, TSNode star_node) {
    //fprintf(stderr, "in expand select star. star_node children: %i\nstar_node: %s", ts_node_child_count(star_node), ts_node_string(star_node));
    std::list<TSNode> field_list;
    const char * reference = "-1";
    if(ts_node_child_count(star_node) > 1)
        reference = node_to_string(r->source, ts_node_child(ts_node_child(star_node, 0), 0));
        //fprintf(stderr, "table specifier on select all\n");
    //fprintf(stderr, "ts_node_symbol: %s (id: %i) reference: %s. ref len = %i\n"
            //,ts_language_symbol_name(tree_sitter_sql(), ts_node_symbol(star_node)), ts_node_symbol(star_node), reference, strlen(reference));
    // "select" node id is 
    while(ts_node_symbol(star_node) != 482) {
        //fprintf(stderr, "ts_node_symbol: %s (id: %i)\n", ts_language_symbol_name(tree_sitter_sql(), ts_node_symbol(star_node)), ts_node_symbol(star_node));
        star_node = ts_node_parent(star_node);
    }
    ts_query_cursor_exec(r->cursor, r->REFERENCES_TO_Q, star_node);
    //fprintf(stderr, "exec-ed\n");
    TSQueryMatch cur_match;
    std::string ssource = r->source;
    int i = 0;
    while(ts_query_cursor_next_match(r->cursor, &cur_match)) {
        // TODO: 1) fix this whole dang function 2) check if the `select` node we're working with is the right thing to use as a parent node
        TSNode ddl_node = context_definition(r, star_node, node_to_string(r->source, cur_match.captures[0].node));
        //fprintf(stderr, "match %i: %s , %s\n", i, node_to_string(r->source, cur_match.captures[0].node), node_to_string(r->source, cur_match.captures[1].node));
        // first predicate should only happen if all cols from all tables are selected
        if(!strcmp("-1", reference)) {
            field_list.merge(result_columns_for_ddl(r, ddl_node), node_compare);
            //fprintf(stderr, "merging field list:\n");
        // the full table matched
        } else if(!strncmp(r->source + ts_node_start_byte(cur_match.captures[0].node), reference, strlen(reference))) {
            //fprintf(stderr, "matched full table name: %s:\n", reference);
            field_list.merge(result_columns_for_ddl(r, ddl_node), node_compare);
            // if the reference before the * was an alias that matches this alias, get all of the fields
            // from the associated table
        } else if(!strncmp(r->source + ts_node_start_byte(cur_match.captures[1].node), reference, strlen(reference))) {
            //fprintf(stderr, "matched table alias: %s:\n", reference);
            field_list.merge(result_columns_for_ddl(r, ddl_node), node_compare);
        }
        i++;
    }
    return field_list;
}

std::list<TSNode> references_to_table(card_runtime * r, const char * table) {
    std::list<TSNode> reflist;
    TSQueryMatch cur_match;
    ts_query_cursor_exec(r->cursor, r->REFERENCES_TO_Q, ts_tree_root_node(r->tree));
    while(ts_query_cursor_next_match(r->cursor, &cur_match)) {
        if(!strncmp(table
                    ,r->source + ts_node_start_byte(cur_match.captures[0].node)
                    ,(ts_node_end_byte(cur_match.captures[0].node) - (ts_node_start_byte(cur_match.captures[0].node))))) {
            reflist.push_front(cur_match.captures[0].node);
        }
    }
    return reflist;
}

std::list<TSNode> references_from_context(card_runtime * r, TSNode parent, const char * context_name) {
    std::list<TSNode> reflist;
    std::string code = r->source;

    //fprintf(stderr, "in refs from, pre anything\n");
    TSNode node = context_definition(r, parent, context_name);
    // UGLY. TODO: fix
    if(ts_node_symbol(node) == OBJ_REF_NODE)
        node = ts_node_parent(node);
    else if(ts_node_symbol(node) == IDENTIFIER_NODE) {
        //fprintf(stderr, "identifier node: %s\n", node_to_string(source, node));
        if(ts_node_symbol(ts_node_parent(node)) == CTE_NODE) {
            node = ts_node_parent(node);
        } else {
            node = ts_node_prev_sibling(ts_node_prev_sibling(node));
        }
    } else if(ts_node_symbol(node) == PROGRAM_NODE)
        return reflist;

    TSQueryMatch cur_match;

    // so I *think* depth is relative to the starting node of the query, not the root node.
    // Tried to read ts_query_cursor__* source code to figure this out but dang that's dense
    // Short of it is we want just the references from this context, not the subcontexts, BUT
    // if we're calling this from a subcontext (CTE or subquery, we ofc want to be able to see
    // references. So if this doesn't work from within subcontexts (depth is relative to root)
    // then we'll have to get hacky
    ts_query_cursor_set_max_start_depth(r->cursor, 5);
    ts_query_cursor_exec(r->cursor, r->REFERENCES_FROM_Q, node);
    while(ts_query_cursor_next_match(r->cursor, &cur_match)) {
        reflist.push_front(cur_match.captures[0].node);
    }
    //fprintf(stderr, "in refs from, %i captures captured\n", reflist.size());

    return reflist;
}

// returns all tables which are downstream of the requested table
std::list<TSNode> tables_downstream_of_table(card_runtime * r, const char * table) {
    // TODO: done. See old commits for thought process
    std::list<TSNode> reflist;
    std::list<TSNode> dfs = references_to_table(r, table);
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
        std::list<TSNode> next_up = references_to_table(r, node_to_string(r->source, next_table));
        // so turns out LIST1.merge(LIST2) is not a "pure function" - it deletes everything from LIST2
        // good news is merging with reflist wasn't even logically accurate - we add highlights when we 
        // get them by the create_table handle, not what's returned from refs_to_table, which is object_reference nodes
        dfs.merge(next_up, node_compare);
    }
    return reflist;
}

std::list<TSNode> contexts_upstream_of_context(card_runtime * r, TSNode parent, const char * context_name) {
    std::list<TSNode> reflist;
    std::set<std::string> visited;
    std::string code = r->source;
    std::list<TSNode> dfs = references_from_context(r, parent, context_name);
    while(dfs.size() > 0) {
        TSNode next_context_ref = dfs.front();
        dfs.pop_front();
        // don't double-add context definitions. Since this will look at just names it will fail if there
        // are multiple subcontexts with the same name in the same DDL document. TODO: fix this
        if(visited.find(node_to_string(r->source, next_context_ref)) != visited.end()) {
            printf("%s already seen, skipping\n", node_to_string(r->source, next_context_ref));
            break;
        }

        TSNode node_parent = parent_context(r->tree, ts_node_start_point(next_context_ref));

        fprintf(stderr, "parent context: %s, VISITED: [\n", node_to_string(r->source, node_parent));
        for(std::string v: visited)
            fprintf(stderr, "\t%s\n", v.c_str());
        fprintf(stderr, "]\n");

        reflist.push_front(node_parent);
        visited.insert(node_to_string(r->source, next_context_ref));

        if(ts_node_symbol(node_parent) == OBJ_REF_NODE)
            node_parent = ts_node_parent(node_parent);
        else if(ts_node_symbol(node_parent) == IDENTIFIER_NODE) {
            // this should only be reached when looking for references inside a subquery
            // BUT its getting called on cust_level - a CTE reference. That shouldn't happen
            // figured it out
            // references TO a CTE are object references, the *name* of a CTE where its created is an identifier
            // as compared to the *name* of create_table table which is itself an object_reference
            //fprintf(stderr, "identifier node: %s\n", node_to_string(r->source, node_parent));
            if(ts_node_symbol(ts_node_parent(node_parent)) == CTE_NODE) {
                node_parent = ts_node_parent(node_parent);
            } else {
                node_parent = ts_node_prev_sibling(ts_node_prev_sibling(node_parent));
                //printf("should be SUBQUERY, is %s (%i)\n", ts_language_symbol_name(tree_sitter_sql(), ts_node_symbol(node_parent)), ts_node_symbol(node_parent));
            }
        }
        //printf("parent node: %s\n", node_to_string(r->source, node_parent));
        std::list<TSNode> refs = references_from_context(
                                    r,
                                    node_parent,
                                    node_to_string(r->source, next_context_ref));
        if(refs.size() > 0)
            dfs.merge(refs, node_compare);
        else {
            TSNode cd = context_definition(r, node_parent, node_to_string(r->source, next_context_ref));
            /*printf("following context defintion node had %i references! line %i: %s\n"
                    ,refs.size()
                    ,ts_node_start_point(next_context_ref).row
                    ,node_to_string(r->source, next_context_ref));*/
            if(ts_node_symbol(cd) == 393) {
                printf("NOTICE: referenced context %s on line %i wasn't defined in this file!\n"
                        ,node_to_string(r->source, next_context_ref)
                        ,ts_node_start_point(next_context_ref).row);
            } else {
                reflist.push_front(cd);
                visited.insert(node_to_string(r->source, cd));
            }
        }
        //fprintf(stderr, "Merged\n");
    }
    //fprintf(stderr, "Returning from upstream of\n");
    return reflist;
}

extern "C" {
    typedef struct {
        int * points;
        int   size;
    } cd_nodelist;

    typedef struct {
        char ** fields;
        int     size;
    } cd_stringlist;

    cd_nodelist references_from_context_c(const char * code, const char * context_name, int cursor_row, int cursor_column) {
        cd_nodelist ret;
        std::string _code = code;
        card_runtime * r = card_runtime_init(code);

        // TODO: get fancy syntax to work... TSPoint cursor_point = (TSPoint) { .row = cursor_row, .column = cursor_column };
        TSPoint cursor_point;
        cursor_point.row = cursor_row;
        cursor_point.column = cursor_column;
        TSNode parent = parent_context(r->tree, cursor_point);
        // TODO: turn this into a function. Geez probably the 3rd place I use it in the codebase
        if(ts_node_symbol(parent) == OBJ_REF_NODE)
            parent = ts_node_parent(parent);
        else if(ts_node_symbol(parent) == IDENTIFIER_NODE) {
            if(ts_node_symbol(ts_node_parent(parent)) == CTE_NODE)
                parent = ts_node_parent(parent);
            else
                parent = ts_node_prev_sibling(ts_node_prev_sibling(parent));
        }

        std::list<TSNode> res = references_from_context(r, parent, context_name);
        ret.points = (int *)malloc(sizeof(int) * 4 * res.size());
        int i = 0;
        for(TSNode n: res) {
            ret.points[(i * 4) + 0] = ts_node_start_point(n).row;
            ret.points[(i * 4) + 1] = ts_node_start_point(n).column;
            ret.points[(i * 4) + 2] =   ts_node_end_point(n).row;
            ret.points[(i * 4) + 3] =   ts_node_end_point(n).column;
            i++;
        }
        ret.size = i;

        card_runtime_deinit(r);
        return ret;
    }

    cd_nodelist references_to_table_c(const char * code, const char * table) {
        cd_nodelist ret;
        card_runtime * r = card_runtime_init(code);

        std::list<TSNode> res = references_to_table(r, table);
        ret.points = (int *)malloc(sizeof(int) * 4 * res.size());
        int i = 0;
        for(TSNode n: res) {
            ret.points[(i * 4) + 0] = ts_node_start_point(n).row;
            ret.points[(i * 4) + 1] = ts_node_start_point(n).column;
            ret.points[(i * 4) + 2] =   ts_node_end_point(n).row;
            ret.points[(i * 4) + 3] =   ts_node_end_point(n).column;
            i++;
        }
        ret.size = i;

        card_runtime_deinit(r);
        return ret;
    }

    cd_nodelist tables_downstream_of_table_c(const char * code, const char * table) {
        cd_nodelist ret;
        card_runtime * r = card_runtime_init(code);

        std::list<TSNode> res = tables_downstream_of_table(r, table);
        ret.points = (int *)malloc(sizeof(int) * 4 * res.size());
        int i = 0;
        for(TSNode n: res) {
            ret.points[(i * 4) + 0] = ts_node_start_point(n).row;
            ret.points[(i * 4) + 1] = ts_node_start_point(n).column;
            ret.points[(i * 4) + 2] =   ts_node_end_point(n).row;
            ret.points[(i * 4) + 3] =   ts_node_end_point(n).column;
            i++;
        }
        ret.size = i;

        card_runtime_deinit(r);
        return ret;
    }

    cd_nodelist contexts_upstream_of_context_c(const char * code, const char * context_name, int cursor_row, int cursor_column) {
        cd_nodelist ret;
        std::string _code = code;
        card_runtime * r = card_runtime_init(code);

        TSPoint cursor_point;
        cursor_point.row = cursor_row;
        cursor_point.column = cursor_column;
        TSNode parent = parent_context(r->tree, cursor_point);
        if(ts_node_symbol(parent) == PROGRAM_NODE)
            exit(1);

        std::list<TSNode> res = contexts_upstream_of_context(r, parent, context_name);
        ret.points = (int *)malloc(sizeof(int) * 4 * res.size());
        int i = 0;
        for(TSNode n: res) {
            ret.points[(i * 4) + 0] = ts_node_start_point(n).row;
            ret.points[(i * 4) + 1] = ts_node_start_point(n).column;
            ret.points[(i * 4) + 2] =   ts_node_end_point(n).row;
            ret.points[(i * 4) + 3] =   ts_node_end_point(n).column;
            i++;
        }
        ret.size = i;

        card_runtime_deinit(r);
        return ret;
    }

    cd_stringlist result_columns_for_table_c(const char * code, const char * table) {
        cd_stringlist ret;

        std::string _code = code;
        card_runtime * r = card_runtime_init(code);

        // TODO: this will only find create table statements. Pass node from neovim so we can do CTEs too
        TSNode ddl = context_definition(r, ts_tree_root_node(r->tree), table);

        std::list<TSNode> res = result_columns_for_ddl(r, ddl);
        ret.fields = (char **)malloc(sizeof(char *) * res.size());
        int i = 0;
        for(TSNode n: res) {
            ret.fields[i] = (char *)malloc(ts_node_end_byte(n) - ts_node_start_byte(n));
            strcpy(ret.fields[i], node_to_string(code, n));
            i++;
        }
        ret.size = i;
        card_runtime_deinit(r);
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
        ret.points = (int *)malloc(sizeof(int) * 4);
        ret.points[0] = ts_node_start_point(n).row;
        ret.points[1] = ts_node_start_point(n).column;
        ret.points[2] =   ts_node_end_point(n).row;
        ret.points[3] =   ts_node_end_point(n).column;
        ret.size = 1;

        ts_tree_delete(tree);
        ts_parser_delete(parser);
        return ret;
    }

    cd_nodelist context_ddl_c(const char * code, TSPoint clicked) {
        cd_nodelist ret;
        card_runtime * r = card_runtime_init(code);
        // SOOOOOOO wasteful. Linus would cry
        TSNode parent = parent_context(r->tree, clicked);
        const char * context_name = node_to_string(code, parent);

        TSNode n = context_ddl(r, clicked, context_name);
        ret.points = (int *) malloc(sizeof(int) * 4);
        ret.points[0] = ts_node_start_point(n).row;
        ret.points[1] = ts_node_start_point(n).column;
        ret.points[2] =   ts_node_end_point(n).row;
        ret.points[3] =   ts_node_end_point(n).column;
        ret.size = 1;

        card_runtime_deinit(r);
        return ret;
    }

    cd_nodelist context_definition_c(const char * code, TSPoint clicked, const char * context_name) {
        cd_nodelist ret;
        card_runtime * r = card_runtime_init(code);
        TSNode parent = parent_context(r->tree, clicked);
        
        TSNode n = context_definition(r, parent, context_name);
        ret.points = (int *) malloc(sizeof(int) * 4);
        ret.points[0] = ts_node_start_point(n).row;
        ret.points[1] = ts_node_start_point(n).column;
        ret.points[2] =   ts_node_end_point(n).row;
        ret.points[3] =   ts_node_end_point(n).column;
        ret.size = 1;

        card_runtime_deinit(r);
        return ret;
    }
}

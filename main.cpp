#include "card.cpp"

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
                node_color_map_list to_highlights = reflist_to_highlights(to_reflist);
                printf("%s\n", format_term_highlights(all_sqls, to_highlights).c_str());
                free(to_highlights.ncms);
            } else if (!strcmp(argv[4], "--from")) {
                printf("in FROM references\n");
                // all tables this table references 
                printf("the following tables are referenced from %s\n", argv[5]);
                std::list<TSNode> from_reflist = references_from_table(tree, all_sqls, argv[5]);
                from_reflist.sort(node_compare);
                node_color_map_list from_highlights = reflist_to_highlights(from_reflist);
                std::string ret = format_term_highlights(all_sqls, from_highlights);
                printf("highlights formatted\n");
                printf("%s\n", ret.c_str());
                free(from_highlights.ncms);
                fprintf(stderr, "after printing full code\n");
            }
        } else if (!strcmp(argv[3], "downstream")) {
            if (argc != 6) { printf("used wrong"); exit(1); }
            if (!strcmp(argv[4], "--of")) {
                printf("in downstream of\n");
                std::list<TSNode> downstream_reflist = tables_downstream_of_table(tree, all_sqls, argv[5]);
                downstream_reflist.sort(node_compare);
                node_color_map_list downstream_highlights = reflist_to_highlights(downstream_reflist);
                printf("%s\n", format_term_highlights(all_sqls, downstream_highlights).c_str());
                free(downstream_highlights.ncms);
            } else {
                printf("used wrong"); exit(1);
            }
        } else if (!strcmp(argv[3], "fields")) {
            if (argc != 6) { printf("used wrong"); exit(1); }
            if (!strcmp(argv[4], "--in")) {
                // this is completely copied... code to get DDL node for a table give table name string. Should be a function
                TSNode ddl_node = create_table_node_for_table_name(tree, all_sqls, argv[5]);
                if(ts_node_start_byte(ddl_node) == 0) {
                    printf("ERROR: no table with the name '%s' exists\n", argv[5]);
                    exit(1);
                }
                printf("create_table node: %s\n", node_to_string(all_sqls.c_str(), ddl_node));

                std::list<TSNode> cols = result_columns_for_ddl(ddl_node, all_sqls.c_str());
                printf("Columns in '%s' table:\n", argv[5]);
                for(TSNode c : cols)
                    printf("%s\n", node_to_string(all_sqls.c_str(), c));
            } else {
                printf("used wrong");
                exit(1);
            }
        }
        fprintf(stderr, "still handling args\n");
    }

    fprintf(stderr, "back in body of main\n");
    return 0;
}

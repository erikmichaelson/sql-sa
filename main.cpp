#include "card.cpp"

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
    printf("root symbol: %i", ts_node_symbol(ts_tree_root_node(tree)));
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
                std::list<TSNode> from_reflist = references_from_context(tree, all_sqls, ts_tree_root_node(tree), argv[5]);
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
        } else if (!strcmp(argv[3], "upstream")) {
            if (argc != 6) { printf("used wrong"); exit(1); }
            if (!strcmp(argv[4], "--of")) {
                printf("in upstream of\n");
                std::list<TSNode> upstream_reflist = contexts_upstream_of_context(tree, all_sqls, ts_tree_root_node(tree), argv[5]);
                upstream_reflist.sort(node_compare);
                node_color_map_list upstream_highlights = reflist_to_highlights(upstream_reflist);
                printf("%s\n", format_term_highlights(all_sqls, upstream_highlights).c_str());
                free(upstream_highlights.ncms);
            } else {
                printf("used wrong"); exit(1);
            }
        } else if (!strcmp(argv[3], "fields")) {
            if (argc != 6) { printf("used wrong"); exit(1); }
            if (!strcmp(argv[4], "--in")) {
                // this is completely copied... code to get DDL node for a table give table name string. Should be a function
                TSNode ddl_node = context_definition(tree, all_sqls, ts_tree_root_node(tree), argv[5]);
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
        } else if (!strcmp(argv[3], "parentcontext")) {
            if (argc != 6) { printf("used wrong"); exit(1); }
            if (!strcmp(argv[4], "--at")) {
                TSPoint p;
                char * str = argv[5];
                p.row = std::stoi(strtok(str, ","));
                p.column = std::stoi(strtok(NULL, ","));
                fprintf(stderr, "point: %i:%i\n", p.row, p.column);
                TSNode ret = parent_context(tree, p);
                //fprintf(stderr, "PARENT CONTEXT:\n%s", node_to_string(all_sqls.c_str(), ret));
                node_color_map n;
                n.node = ret;
                n.color = PURPLE;

                node_color_map_list ncm;
                ncm.length = 1;
                ncm.ncms = (node_color_map *)malloc(sizeof(node_color_map));
                ncm.ncms[0] = n;
                printf("\n%s\n", format_term_highlights(all_sqls, ncm).c_str());
            }
        }
        fprintf(stderr, "still handling args\n");
    }

    fprintf(stderr, "back in body of main\n");
    return 0;
}

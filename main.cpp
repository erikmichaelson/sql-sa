#include "card.cpp"

typedef enum { PURPLE, RED } HIGHLIGHT_COLOR;

typedef struct {
    TSNode node;
    HIGHLIGHT_COLOR color;
} node_color_map;

typedef struct {
    node_color_map * ncms;
    uint32_t length;
} node_color_map_list;

void serialize_queries() {
    size_t len;
    FILE* fd = fopen("queries/all_queries.tsq", "w+");
    TSQueryError q_error;
    uint32_t q_error_offset;

    const char * refq = "[ (relation (object_reference schema: (identifier)? name: (identifier))@reference alias: (identifier)? @alias)\
                        ((subquery) (keyword_as)? (identifier) @reference) ]";
    TSQuery * REFERENCES_Q = ts_query_new(
        tree_sitter_sql(),
        refq,
        strlen(refq),
        &q_error_offset,
        &q_error
    );
    const char * buf1 = ts_query_serialize(REFERENCES_Q, &len);
    printf("references from query serialized, %lu bytes\n", len);
    fwrite(buf1, 1, len, fd);
    fflush(fd);

    const char * cdtq = "(create_table (object_reference schema: (identifier)? name: (identifier)) @definition)";
    TSQuery * CONTEXT_DEF_TABLE_Q = ts_query_new(
        tree_sitter_sql(),
        cdtq,
        strlen(cdtq),
        &q_error_offset,
        &q_error
    );
    const char * buf3 = ts_query_serialize(CONTEXT_DEF_TABLE_Q, &len);
    printf("context def table query serialized, %lu bytes\n", len);
    fwrite(buf3, 1, len, fd);
    fflush(fd);

    const char * cdsq = "[(cte (identifier) @definition) ((subquery) (keyword_as)? (identifier) @definition)]";
    TSQuery * CONTEXT_DEF_SUB_Q = ts_query_new(
        tree_sitter_sql(),
        cdsq,
        strlen(cdsq),
        &q_error_offset,
        &q_error
    );
    const char * buf4 = ts_query_serialize(CONTEXT_DEF_SUB_Q, &len);
    printf("context def sub query serialized, %lu bytes\n", len);
    fwrite(buf4, 1, len, fd);
    fflush(fd);

    const char * fldq = "(select (select_expression (term [\
              (all_fields) @d\
              (field (object_reference)? @cs\
                name: (identifier) @d)\
              alias: (identifier) @d ] )))";
    TSQuery * FIELD_DEF_Q = ts_query_new(
        tree_sitter_sql(),
        fldq,
        strlen(fldq),
        &q_error_offset,
        &q_error
    );
    const char * buf5 = ts_query_serialize(FIELD_DEF_Q, &len);
    printf("field def query serialized, %lu bytes\n", len);
    fwrite(buf5, 1, len, fd);
    fflush(fd);

    const char * colq = "(column_definitions (column_definition name: (identifier) @col_def))";
    TSQuery * COLUMN_DEF_Q = ts_query_new(
        tree_sitter_sql(),
        colq,
        strlen(colq),
        &q_error_offset,
        &q_error
    );
    const char * buf6 = ts_query_serialize(COLUMN_DEF_Q, &len);
    printf("column def query serialized, %lu bytes\n", len);
    fwrite(buf6, 1, len, fd);
    fflush(fd);

    const char * field_refq = "(field (object_reference)? @source name: (identifier) @field)";
    TSQuery * FIELD_REF_Q = ts_query_new(
        tree_sitter_sql(),
        field_refq,
        strlen(field_refq),
        &q_error_offset,
        &q_error
    );
    const char * buf7 = ts_query_serialize(FIELD_REF_Q, &len);
    printf("field reference query serialized, %lu bytes\n", len);
    fwrite(buf7, 1, len, fd);
    fflush(fd);

    fclose(fd);
    return;
}

void usage() {
    printf("\
card  [serialize]\n\
      [usage]\n\
                  [$SQL_FILE_PATH] [--show] [references] [--to|--from] [$ROW],[$COL] [$CONTEXT_NAME]\n\
                                            [upstream]   [--of]        [$ROW],[$COL] [$CONTEXT_NAME]\n\
                                            [fields]     [--in]        [$ROW],[$COL] [$CONTEXT_NAME]");
    return;
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
    if(!strcmp(argv[1], "serialize")) {
        printf("serializing all queries\n");
        serialize_queries();
        exit(0);
    } else if(!strcmp(argv[1], "serialize")) {
        usage();
        exit(0);
    }

    std::string files = "ALL";

    if(argc > 1)
        files = std::string(argv[1]);

    std::string all_sqls = open_sqls(files);
    //printf("files opened\n");
    card_runtime * r = card_runtime_init(all_sqls.c_str());
    //FILE * dg = fopen("dotgraph.dot", "w");
    //ts_tree_print_dot_graph(tree, fileno(dg));

    if(argc > 2) {
        // show either DDL or references for a table
        if(argc < 4 || strcmp(argv[2], "--show")) {
            usage(); // had a \n. REMOVED IT (yeah yeah)
            exit(1);
        }

        // what does "DDL" mean?
        if(!strcmp(argv[3], "ddl")) {
            printf("DDL not implemented");
            exit(1);
        } else if (!strcmp(argv[3], "references")) {
            if(argc != 7) { usage(); exit(1); }
            TSPoint p;
            char * str = argv[5];
            p.row = std::stoi(strtok(str, ","));
            p.column = std::stoi(strtok(NULL, ","));
            fprintf(stderr, "point: %i:%i\n", p.row, p.column);
            TSNode parent_ref = parent_context(r->tree, p);
            if(!strcmp(argv[4], "--to")) {
                printf("in TO references\n");
                // all places this table is referenced
                printf("the following contexts reference %s\n", argv[6]);
                std::list<TSNode> to_reflist = references_to_context(r, parent_ref, argv[6]);
                to_reflist.sort(node_compare);
                node_color_map_list to_highlights = reflist_to_highlights(to_reflist);
                printf("%s\n", format_term_highlights(all_sqls, to_highlights).c_str());
                free(to_highlights.ncms);
            } else if (!strcmp(argv[4], "--from")) {
                printf("in FROM references\n");
                // all tables this table references 
                std::list<TSNode> from_reflist = references_from_context(r, parent_ref, argv[6]);
                if(!from_reflist.size()) {
                    printf("Zero tables are referenced from %s\n", argv[5]);
                    exit(1);
                }
                printf("the following %lu tables are referenced from %s\n", from_reflist.size(), argv[6]);
                from_reflist.sort(node_compare);
                node_color_map_list from_highlights = reflist_to_highlights(from_reflist);
                std::string ret = format_term_highlights(all_sqls, from_highlights);
                printf("%s\n", ret.c_str());
                free(from_highlights.ncms);
            }
        } else if (!strcmp(argv[3], "downstream")) {
            if (argc != 7) { usage(); exit(1); }
            if (!strcmp(argv[4], "--of")) {
                TSPoint p;
                char * str = argv[5];
                p.row = std::stoi(strtok(str, ","));
                p.column = std::stoi(strtok(NULL, ","));
                fprintf(stderr, "point: %i:%i\n", p.row, p.column);
                TSNode parent_ref = parent_context(r->tree, p);
                std::list<TSNode> downstream_reflist = contexts_downstream_of_context(r, parent_ref, argv[6]);
                downstream_reflist.sort(node_compare);
                node_color_map_list downstream_highlights = reflist_to_highlights(downstream_reflist);
                printf("%s\n", format_term_highlights(all_sqls, downstream_highlights).c_str());
                free(downstream_highlights.ncms);
            } else {
                usage(); exit(1);
            }
        } else if (!strcmp(argv[3], "upstream")) {
            if (argc != 7) { usage(); exit(1); }
            TSPoint p;
            char * str = argv[5];
            p.row = std::stoi(strtok(str, ","));
            p.column = std::stoi(strtok(NULL, ","));
            fprintf(stderr, "point: %i:%i\n", p.row, p.column);
            TSNode parent_ref = parent_context(r->tree, p);
            if (!strcmp(argv[4], "--of")) {
                printf("in upstream of\n");
                std::list<TSNode> upstream_reflist = contexts_upstream_of_context(r, parent_ref, argv[6]);
                upstream_reflist.sort(node_compare);
                node_color_map_list upstream_highlights = reflist_to_highlights(upstream_reflist);
                printf("%lu contexts upstream of %s\n", upstream_reflist.size(), argv[6]);
                printf("%s\n", format_term_highlights(all_sqls, upstream_highlights).c_str());
                free(upstream_highlights.ncms);
            } else {
                usage(); exit(1);
            }
        } else if (!strcmp(argv[3], "fields")) {
            if (argc != 6) { usage(); exit(1); }
            if (!strcmp(argv[4], "--in")) {
                // this is completely copied... code to get DDL node for a table give table name string. Should be a function
                TSNode ddl_node = context_definition(r, ts_tree_root_node(r->tree), argv[5]);
                if(ts_node_start_byte(ddl_node) == 0) {
                    printf("ERROR: no table with the name '%s' exists\n", argv[5]);
                    exit(1);
                }
                printf("create_table node: %s\n", node_to_string(all_sqls.c_str(), ddl_node));

                std::list<TSNode> cols = result_columns_for_ddl(r, ddl_node);
                printf("Columns in '%s' table:\n", argv[5]);
                for(TSNode c : cols)
                    printf("%s\n", node_to_string(all_sqls.c_str(), c));
            } else {
                usage();
                exit(1);
            }
        } else if (!strcmp(argv[3], "parentcontext")) {
            if (argc != 6) { usage(); exit(1); }
            if (!strcmp(argv[4], "--at")) {
                TSPoint p;
                char * str = argv[5];
                p.row = std::stoi(strtok(str, ","));
                p.column = std::stoi(strtok(NULL, ","));
                fprintf(stderr, "point: %i:%i\n", p.row, p.column);
                TSNode ret = parent_context(r->tree, p);
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
        } else if (!strcmp(argv[3], "contextddl")) {
            if (argc != 6) { usage(); exit(1); }
            if (!strcmp(argv[4], "--at")) {
                TSPoint p;
                char * str = argv[5];
                p.row = std::stoi(strtok(str, ","));
                p.column = std::stoi(strtok(NULL, ","));
                TSNode pc = parent_context(r->tree, p);
                const char * context_name = node_to_string(all_sqls.c_str(), pc);
                TSNode ret = context_ddl(r, p, context_name);
                node_color_map n;
                n.node = ret;
                n.color = PURPLE;
                node_color_map_list ncm;
                ncm.length = 1;
                ncm.ncms = (node_color_map *)malloc(sizeof(node_color_map));
                ncm.ncms[0] = n;
                printf("\n%s\n", format_term_highlights(all_sqls, ncm).c_str());
            }
        } else if (!strcmp(argv[3], "oneup")) {
            if (argc != 7) { usage(); exit(1); }
            if (!strcmp(argv[4], "--of")) {
                TSPoint p;
                char * str = argv[6];
                p.row = std::stoi(strtok(str, ","));
                p.column = std::stoi(strtok(NULL, ","));
                TSNode pc = parent_context(r->tree, p);
                std::list<TSNode> reflist = columns_one_up_of_column(r, pc, argv[5]);
                reflist.sort(node_compare);
                node_color_map_list oneup_highlights = reflist_to_highlights(reflist);
                printf("%lu columns upstream of %s\n", reflist.size(), argv[5]);
                if(reflist.size() > 0)
                    printf("\n%s\n", format_term_highlights(all_sqls, oneup_highlights).c_str());
                free(oneup_highlights.ncms);
            }
        }
    }
    card_runtime_deinit(r);

    return 0;
}

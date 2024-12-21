#include "tree_sitter/api.h"

typedef struct {
    const char *    source;
    TSLanguage *    language;
    TSParser *      parser;
    TSTree *        tree;
    TSQueryCursor * cursor;
    TSQuery *       REFERENCES_Q;
    TSQuery *       CONTEXT_DEF_TABLE_Q;
    TSQuery *       CONTEXT_DEF_SUB_Q;
    TSQuery *       FIELD_DEF_Q;
    TSQuery *       COLUMN_DEF_Q;
    TSQuery *       FIELD_REF_Q;
} card_runtime;

card_runtime * card_runtime_init_c(const char * source);
void card_runtime_deinit_c(card_runtime * r);

char * node_to_string_c(const char * s, TSNode n);

typedef struct { int * points; int size; } cd_nodelist;
// interesting... this API is _WRONG_, but I can still call it. Missing the row and col
// to make the point, but that must just be using uninitialized memory. Shocked it doesn't crash
cd_nodelist references_from_context_c(card_runtime * r, const char * table, int row, int col);
cd_nodelist references_to_context_c(card_runtime * r, const char * table, int row, int col);
cd_nodelist contexts_downstream_of_context_c(card_runtime * r, const char * context_name
                                            ,int row, int col);
cd_nodelist contexts_upstream_of_context_c(card_runtime * r, const char * context_name
                                            ,int row, int col);
cd_nodelist columns_one_up_of_column_c(card_runtime * r, const char * column_name
                                            ,int row, int col);
cd_nodelist columns_one_down_of_column_c(card_runtime * r, const char * column_name
                                            ,int row, int col);

typedef struct { char ** fields; int size; } cd_stringlist;
cd_stringlist result_columns_for_table_c(card_runtime * r, const char * table);

// this will only ever return one node, but this makes the result easier to use
// TODO: fix to make less confusing
cd_nodelist parent_context_c(card_runtime * r, TSPoint clicked);
cd_nodelist context_ddl_c(card_runtime * r, TSPoint clicked);
cd_nodelist context_definition_c(card_runtime * r, TSPoint clicked, const char * context_name);
cd_nodelist field_definitions_in_context_c(card_runtime * r, const char * context_name, int cursor_row, int cursor_column);
cd_nodelist column_definition_c(card_runtime * r, int cursor_row, int cursor_column);

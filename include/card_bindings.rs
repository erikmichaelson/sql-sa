extern crate core;
extern crate tree_sitter;
use tree_sitter::*;

#[repr(C)]
pub struct card_runtime {
    pub source: *const ::core::ffi::c_char,
    pub language: *mut Language,
    pub parser: *mut Parser,
    pub tree: *mut Tree,
    pub cursor: *mut QueryCursor,
    pub REFERENCES_Q: *mut Query,
    pub CONTEXT_DEF_TABLE_Q: *mut Query,
    pub CONTEXT_DEF_SUB_Q: *mut Query,
    pub FIELD_DEF_Q: *mut Query,
    pub COLUMN_DEF_Q: *mut Query,
    pub FIELD_REF_Q: *mut Query,
}

//typedef struct { int * points; int size; } cd_nodelist;
#[repr(C)]
pub struct cd_nodelist {
    pub points : *const u32,
    pub size :  u32,
}

////typedef struct { char ** fields; int size; } cd_stringlist;
//#[repr(C)]
//pub struct cd_stringlist {
//    pub strings:**const ::core::ffi::c_char,
//    pub size:int,
//}

extern "C" {
    //card_runtime * card_runtime_init_c(const char * source);
    pub fn card_runtime_init_c(source:*const ::core::ffi::c_char) -> *mut card_runtime;

    //void card_runtime_deinit_c(card_runtime * r);
    pub fn card_runtime_deinit_c(r:*mut card_runtime) -> ::core::ffi::c_void;

    //char * node_to_string_c(const char * s, TSNode n);
    pub fn node_to_string_c(source:*const ::core::ffi::c_char, n:Node) -> *const ::core::ffi::c_char;

    //cd_nodelist references_from_context_c(card_runtime * r, const char * table, int row, int col);
    pub fn references_from_context_c(r:*mut card_runtime, context_name:*const ::core::ffi::c_char
                                  ,row: u32, col: u32) -> cd_nodelist;
    //cd_nodelist references_to_context_c(card_runtime * r, const char * table, int row, int col);
    pub fn references_to_context_c(r:*mut card_runtime, context_name:*const ::core::ffi::c_char
                                ,row: u32, col: u32) -> cd_nodelist;

    //cd_nodelist contexts_downstream_of_context_c(card_runtime * r, const char * context_name
    //                                            ,int row, int col);
    pub fn contexts_downstream_of_context_c(r:*mut card_runtime, context_name:*const ::core::ffi::c_char
                                         ,row: u32, col: u32) -> cd_nodelist;
    //cd_nodelist contexts_upstream_of_context_c(card_runtime * r, const char * context_name
    //                                            ,int row, int col);
    pub fn contexts_upstream_of_context_c(r:*mut card_runtime, context_name:*const ::core::ffi::c_char
                                        ,row: u32, col: u32) -> cd_nodelist;
    //cd_nodelist references_from_column_c(card_runtime * r, const char * column_name
    //                                            ,int row, int col);
    pub fn references_from_column_c(r:*mut card_runtime, column_name:*const ::core::ffi::c_char
                                 ,row: u32, col: u32) -> cd_nodelist;
    //cd_nodelist columns_one_down_of_column_c(card_runtime * r, const char * column_name
    //                                            ,int row, int col);
    pub fn references_to_column_c(r:*mut card_runtime, column_name:*const ::core::ffi::c_char
                               ,row: u32, col: u32) -> cd_nodelist;

    //cd_stringlist result_columns_for_table_c(card_runtime * r, const char * table);
//    pub fn result_columns_for_table_c(r:*mut card_runtime, table_name:*const ::core::ffi::c_char) -> cd_stringlist;
//
    //cd_nodelist parent_context_c(card_runtime * r, TSPoint clicked);
    pub fn parent_context_c(r:*mut card_runtime, clicked:Point) -> cd_nodelist;
    //cd_nodelist context_ddl_c(card_runtime * r, TSPoint clicked);
    pub fn context_ddl_c(r:*mut card_runtime, clicked:Point) -> cd_nodelist;
    //cd_nodelist context_definition_c(card_runtime * r, TSPoint clicked, const char * context_name);
    pub fn context_definition_c(r:*mut card_runtime, clicked:Point, context_name:*const ::core::ffi::c_char) -> cd_nodelist;
    //cd_nodelist field_definitions_in_context_c(card_runtime * r, const char * context_name, int cursor_row, int cursor_column);
    pub fn field_definitions_in_context_c(r:*mut card_runtime, context_name:*const ::core::ffi::c_char
                                         ,row: u32, col: u32) -> cd_nodelist;
    //cd_nodelist column_definition_c(card_runtime * r, int cursor_row, int cursor_column);
    pub fn column_definition_c(r:*mut card_runtime, row: u32, col: u32) -> cd_nodelist;
}

-------  CARD  -------
local api = vim.api
local ffi = require('ffi')

--print "Hello from nvim_card"
ffi.cdef[[
    typedef struct TSTree TSTree;
    typedef struct {
      uint32_t context[4];
      const void *id;
      const TSTree *tree;
    } TSNode;
    typedef struct { int * points; int size; } cd_nodelist;
    cd_nodelist references_from_table_c(const char * source, const char * table);
    cd_nodelist references_to_table_c(const char * source, const char * table);
    cd_nodelist tables_downstream_of_table_c(const char * source, const char * table);
    typedef struct { char ** fields; int size; } cd_stringlist;
    cd_stringlist result_columns_for_table_c(const char * source, const char * table);
]]

-- hopefully it can figure out card = card.so, not cardlib.so
-- ^ above is WRONG. Not sure how it worked for zlib, but it need to be force fed
--   exactly libcard.dylib (NOT libcard.so, and it needs to be compiled with .dylib flag)
local card = ffi.load("card")

cns = api.nvim_create_namespace('card')
function highlight_card_references_from_table(table_name)
    api.nvim_buf_clear_namespace(0, cns, 0, -1)
    vim.cmd('syntax off')
    -- TODO: this is probably not the most efficient way. Honestly, C is fun and all
    -- but probably *faster* to do this all from lua since Treesitter is native and the
    -- buffer is already in memory. Whoop
    local source = api.nvim_buf_get_lines(0, 0, -1, true)
    source = table.concat(source, "\n")
    local points = ffi.new("cd_nodelist[1]")
    points = card.references_from_table_c(source, table_name)
    for p = 0, (points.size - 1) do
        api.nvim_buf_add_highlight(0, cns, 'WildMenu'
            ,tonumber(points.points[(p * 3) + 0])
            ,tonumber(points.points[(p * 3) + 1])
            ,tonumber(points.points[(p * 3) + 2]))
    end
    print "mission accomplished"
end

function highlight_card_references_to_table(table_name)
    api.nvim_buf_clear_namespace(0, cns, 0, -1)
    vim.cmd('syntax off')
    local source = api.nvim_buf_get_lines(0, 0, -1, true)
    source = table.concat(source, "\n")
    local points = ffi.new("cd_nodelist[1]")
    points = card.references_to_table_c(source, table_name)
    for p = 0, (points.size - 1) do
        api.nvim_buf_add_highlight(0, cns, 'WildMenu'
            ,tonumber(points.points[(p * 3) + 0])
            ,tonumber(points.points[(p * 3) + 1])
            ,tonumber(points.points[(p * 3) + 2]))
    end
end

function highlight_card_downstream_of_table(table_name)
    api.nvim_buf_clear_namespace(0, cns, 0, -1)
    vim.cmd('syntax off')
    local source = api.nvim_buf_get_lines(0, 0, -1, true)
    source = table.concat(source, "\n")
    local points = ffi.new("cd_nodelist[1]")
    points = card.tables_downstream_of_table_c(source, table_name)
    for p = 0, (points.size - 1) do
        api.nvim_buf_add_highlight(0, cns, 'WildMenu'
            ,tonumber(points.points[(p * 3) + 0])
            ,tonumber(points.points[(p * 3) + 1])
            ,tonumber(points.points[(p * 3) + 2]))
    end
end

function print_card_columns_in_table(table_name)
    api.nvim_buf_clear_namespace(0, cns, 0, -1)
    vim.cmd('syntax off')
    local source = api.nvim_buf_get_lines(0, 0, -1, true)
    source = table.concat(source, "\n")
    local fields = ffi.new("cd_stringlist[1]")
    fields = card.result_columns_for_table_c(source, table_name)
    for i = 0, (fields.size - 1) do
        print ("\t"..ffi.string(fields.fields[i]))
    end
end

function card_reset()
    api.nvim_buf_clear_namespace(0, cns, 0, -1)
    vim.cmd('syntax on')
end

print "try 'highlight_card_references_from_table(table_name)' "

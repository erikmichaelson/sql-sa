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
]]

-- hopefully it can figure out card = card.so, not cardlib.so
-- ^ above is WRONG. Not sure how it worked for zlib, but it need to be force fed
--   exactly libcard.dylib (NOT libcard.so, and it needs to be compiled with .dylib flag)
local card = ffi.load("card")

cns = api.nvim_create_namespace('card')
function highlight_card_references_from_table(table_name)
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

print "try 'highlight_card_references_from_table(table_name)' "
--highlight_card_references_from_table("throwaway")

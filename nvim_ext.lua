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
    typedef struct { TSNode * nodes; size_t size; } cd_nodelist;
    cd_nodelist references_from_table_c(TSTree * tree, const char * source, const char * table);
    int multiply_two_numbers(int a, int b);
]]

-- hopefully it can figure out card = card.so, not cardlib.so
-- ^ above is WRONG. Not sure how it worked for zlib, but it need to be force fed
--   exactly libcard.dylib (NOT libcard.so, and it needs to be compiled with .dylib flag)
local card = ffi.load("card")

cns = api.nvim_create_namespace('card')
print "new language, same debug tactic"
function highlight_card_references_from_table(table_name)
    print "new language, same debug tactic"
    local tree = api.TSTree
    -- TODO: this is probably not the most efficient way. Honestly, C is fun and all
    -- but probably *faster* to do this all from lua since Treesitter is native and the
    -- buffer is already in memory. Whoop
    local source = api.nvim_buf_get_lines(0, 0, -1, true)
    source = table.concat(source, "\n")
    print ("source is " .. #source .. " characters long")
    print ("tree obj = " .. tree .. " mult two nums: " .. card.multiply_two_numbers(3, 8))
--    local points = card.references_from_table_c(tree, source, table_name)
--    for p in points.size do
--        api.nvim_buf_add_highlights(0, cns, 'WildMenu', p[0], p[1], p[3])
--    end
end

highlight_card_references_from_table("throwaway")

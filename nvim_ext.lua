------- HELPER -------
function get_visual_selection()
  local s_start = vim.fn.getpos("'<")
  local s_end = vim.fn.getpos("'>")
  local n_lines = math.abs(s_end[2] - s_start[2]) + 1
  local lines = vim.api.nvim_buf_get_lines(0, s_start[2] - 1, s_end[2], false)
  lines[1] = string.sub(lines[1], s_start[3], -1)
  if n_lines == 1 then
    lines[n_lines] = string.sub(lines[n_lines], 1, s_end[3] - s_start[3] + 1)
  else
    lines[n_lines] = string.sub(lines[n_lines], 1, s_end[3])
  end
  return table.concat(lines, '\n')
end

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
    cd_nodelist tables_upstream_of_table_c(const char * source, const char * table);

    typedef struct { char ** fields; int size; } cd_stringlist;
    cd_stringlist result_columns_for_table_c(const char * source, const char * table);

    typedef struct { uint32_t row; uint32_t column; } TSPoint;
    // this will only ever return one node, but this makes the result easier to use
    // TODO: fix to make less confusing
    cd_nodelist parent_context_c(const char * code, TSPoint clicked);
]]

-- hopefully it can figure out card = card.so, not cardlib.so
-- ^ above is WRONG. Not sure how it worked for zlib, but it need to be force fed
--   exactly libcard.dylib (NOT libcard.so, and it needs to be compiled with .dylib flag)
local card = ffi.load("card")

cns = api.nvim_create_namespace('card')

-- must be called from Neovim obv so there's a point to be found
function highlight_card_parent_context()
    local point = ffi.new('TSPoint[1]')
    point[0].row    = api.nvim_win_get_cursor(0)[1]
    point[0].column = api.nvim_win_get_cursor(0)[2]
    --print(point[0].row..', '..point[0].column)
    local source = api.nvim_buf_get_lines(0, 0, -1, true)
    source = table.concat(source, '\n')
    local res = card.parent_context_c(source, point[0]);
    --print (tonumber(res.points[0])..','..tonumber(res.points[1])..','..tonumber(res.points[2]))
    api.nvim_buf_add_highlight(0, cns, 'WildMenu'
        ,tonumber(res.points[0])
        ,tonumber(res.points[1])
        ,tonumber(res.points[2]))
end

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

function highlight_card_upstream_of_table(table_name)
    api.nvim_buf_clear_namespace(0, cns, 0, -1)
    vim.cmd('syntax off')
    local source = api.nvim_buf_get_lines(0, 0, -1, true)
    source = table.concat(source, "\n")
    local points = ffi.new("cd_nodelist[1]")
    points = card.tables_upstream_of_table_c(source, table_name)
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

vim.cmd("let mapleader=',' ")
vim.keymap.set('v', '<Leader>t'
    ,':lua highlight_card_references_to_table(get_visual_selection())<CR>')
vim.keymap.set('v', '<Leader>f'
    ,':lua highlight_card_references_from_table(get_visual_selection())<CR>')
vim.keymap.set('v', '<Leader>j'
    ,':lua highlight_card_downstream_of_table(get_visual_selection())<CR>')
vim.keymap.set('v', '<Leader>k'
    ,':lua highlight_card_upstream_of_table(get_visual_selection())<CR>')
vim.keymap.set('n', '<Leader>r', ':lua card_reset()<CR>')

print "try 'highlight_card_references_from_table(table_name)' "

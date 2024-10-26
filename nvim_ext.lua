------- GLOBALS ------
local api = vim.api

------- HELPER -------
function multiline_highlight(start_row, start_col, end_row, end_col)
    --print("in ml_hl: ["..start_row..","..start_col.."],["..end_row..":"..end_col.."]")
    if (end_row - start_row) > 0 then
        api.nvim_buf_add_highlight(0, cns, 'WildMenu'
            ,start_row
            ,start_col
            ,-1)
        for r = start_row+1, end_row-1 do
            api.nvim_buf_add_highlight(0, cns, 'WildMenu' ,r ,0 ,-1)
        end
        api.nvim_buf_add_highlight(0, cns, 'WildMenu'
            ,end_row
            ,0
            ,end_col)
    else
        api.nvim_buf_add_highlight(0, cns, 'WildMenu'
            ,start_row
            ,start_col
            ,end_col)
    end
end

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
local ffi = require('ffi')

ffi.cdef[[
    typedef struct card_runtime card_runtime;

    card_runtime * card_runtime_init_c(const char * source);

    typedef struct TSTree TSTree;
    typedef struct {
      uint32_t context[4];
      const void *id;
      const TSTree *tree;
    } TSNode;
    typedef struct { int * points; int size; } cd_nodelist;
    // interesting... this API is _WRONG_, but I can still call it. Missing the row and col
    // to make the point, but that must just be using uninitialized memory. Shocked it doesn't crash
    cd_nodelist references_from_context_c(card_runtime * r, const char * table, int row, int col);
    cd_nodelist references_to_context_c(card_runtime * r, const char * table, int row, int col);
    cd_nodelist contexts_downstream_of_context_c(card_runtime * r, const char * context_name
                                                ,int row, int col);
    cd_nodelist contexts_upstream_of_context_c(card_runtime * r, const char * context_name
                                                ,int row, int col);
    cd_nodelist references_from_column_c(card_runtime * r, const char * column_name
                                                ,int row, int col);
    cd_nodelist references_to_column_c(card_runtime * r, const char * column_name
                                                ,int row, int col);

    typedef struct { char ** fields; int size; } cd_stringlist;
    cd_stringlist result_columns_for_table_c(card_runtime * r, const char * table);

    typedef struct { uint32_t row; uint32_t column; } TSPoint;
    // this will only ever return one node, but this makes the result easier to use
    // TODO: fix to make less confusing
    cd_nodelist parent_context_c(card_runtime * r, TSPoint clicked);
    cd_nodelist context_ddl_c(card_runtime * r, TSPoint clicked);
    cd_nodelist context_definition_c(card_runtime * r, TSPoint clicked, const char * context_name);
    cd_nodelist field_definitions_in_context_c(card_runtime * r, const char * context_name, int cursor_row, int cursor_column);
    cd_nodelist column_definition_c(card_runtime * r, int cursor_row, int cursor_column);
]]

-- hopefully it can figure out card = card.so, not cardlib.so
-- ^ above is WRONG. Not sure how it worked for zlib, but it need to be force fed
--   exactly libcard.dylib (NOT libcard.so, and it needs to be compiled with .dylib flag)
local card = ffi.load("card")

cns = api.nvim_create_namespace('card')

r = ffi.new('card_runtime *[1]')
source = api.nvim_buf_get_lines(0, 0, -1, true)
source = table.concat(source, '\n')
r = card.card_runtime_init_c(source)

function card_get_parent_context()
    api.nvim_buf_clear_namespace(0, cns, 0, -1)
    local point = ffi.new('TSPoint[1]')
    point[0].row    = api.nvim_win_get_cursor(0)[1] - 1
    point[0].column = api.nvim_win_get_cursor(0)[2]
    local res = card.parent_context_c(source, point[0])
    local lines = vim.api.nvim_buf_get_text(0
                            ,res.points[0]
                            ,res.points[1]
                            ,res.points[2]
                            ,res.points[3]
                            ,{})
    return lines[1]
end

function highlight_card_context_definition(context_name)
    api.nvim_buf_clear_namespace(0, cns, 0, -1)
    local point = ffi.new('TSPoint[1]')
    point[0].row    = api.nvim_win_get_cursor(0)[1] - 1
    point[0].column = api.nvim_win_get_cursor(0)[2]
    local res = card.context_definition_c(r, point[0], context_name)
    api.nvim_buf_add_highlight(0, cns, 'WildMenu'
        ,res.points[0]
        ,res.points[1]
        ,res.points[3])
end

function highlight_card_context_ddl()
    api.nvim_buf_clear_namespace(0, cns, 0, -1)
    local point = ffi.new('TSPoint[1]')
    point[0].row    = api.nvim_win_get_cursor(0)[1] - 1
    point[0].column = api.nvim_win_get_cursor(0)[2]
    local res = card.context_ddl_c(r, point[0])

    multiline_highlight(tonumber(res.points[0]), tonumber(res.points[1])
                        ,tonumber(res.points[2]), tonumber(res.points[3]))
end

-- must be called from Neovim obv so there's a point to be found
function highlight_card_parent_context()
    api.nvim_buf_clear_namespace(0, cns, 0, -1)
    local point = ffi.new('TSPoint[1]')
    point[0].row    = api.nvim_win_get_cursor(0)[1] - 1
    point[0].column = api.nvim_win_get_cursor(0)[2]
    --print(point[0].row..', '..point[0].column)
    local res = card.parent_context_c(r, point[0]);
    --print ('['..tonumber(res.points[0])..':'..tonumber(res.points[1])
    --        ..'],['..tonumber(res.points[2])..':'..tonumber(res.points[3])..']')

    api.nvim_buf_add_highlight(0, cns, 'WildMenu'
        ,res.points[0]
        ,res.points[1]
        ,res.points[3])
end

function highlight_card_field_defs_in_context(context_name, row, col)
    api.nvim_buf_clear_namespace(0, cns, 0, -1)
    vim.cmd('syntax off')
    local points = card.field_definitions_in_context_c(r, context_name, row, col)
    for p = 0, (points.size - 1) do
        multiline_highlight(
             tonumber(points.points[(p * 4) + 0])
            ,tonumber(points.points[(p * 4) + 1])
            ,tonumber(points.points[(p * 4) + 2])
            ,tonumber(points.points[(p * 4) + 3]))
    end
    print (points.size.." fields defined in "..context_name)
end

function highlight_card_references_from_context(context_name, row, col)
    api.nvim_buf_clear_namespace(0, cns, 0, -1)
    vim.cmd('syntax off')
    -- TODO: this is probably not the most efficient way. Honestly, C is fun and all
    -- but probably *faster* to do this all from lua since Treesitter is native and the
    -- buffer is already in memory. Whoop
    local points = ffi.new("cd_nodelist[1]")
    points = card.references_from_context_c(r, context_name, row, col)
    for p = 0, (points.size - 1) do
        api.nvim_buf_add_highlight(0, cns, 'WildMenu'
            ,tonumber(points.points[(p * 4) + 0])
            ,tonumber(points.points[(p * 4) + 1])
            ,tonumber(points.points[(p * 4) + 3]))
    end
    print (points.size.." references from "..context_name)
end

function highlight_card_references_to_context(context_name, row, col)
    api.nvim_buf_clear_namespace(0, cns, 0, -1)
    vim.cmd('syntax off')
    local points = ffi.new("cd_nodelist[1]")
    points = card.references_to_context_c(r, context_name, row, col)
    for p = 0, (points.size - 1) do
        api.nvim_buf_add_highlight(0, cns, 'WildMenu'
            ,tonumber(points.points[(p * 4) + 0])
            ,tonumber(points.points[(p * 4) + 1])
            ,tonumber(points.points[(p * 4) + 3]))
    end
    print(points.size.." references to "..context_name)
end

function highlight_card_downstream_of_context(context_name, row, col)
    api.nvim_buf_clear_namespace(0, cns, 0, -1)
    vim.cmd('syntax off')
    local points = ffi.new("cd_nodelist[1]")
    points = card.tables_downstream_of_context_c(r, context_name, row, col)
    for p = 0, (points.size - 1) do
        api.nvim_buf_add_highlight(0, cns, 'WildMenu'
            ,tonumber(points.points[(p * 4) + 0])
            ,tonumber(points.points[(p * 4) + 1])
            ,tonumber(points.points[(p * 4) + 3]))
    end
    print(points.size.." tables downstream of "..table_name)
end

function highlight_card_upstream_of_context(context_name, row, col)
    api.nvim_buf_clear_namespace(0, cns, 0, -1)
    vim.cmd('syntax off')
    local points = ffi.new("cd_nodelist[1]")
    points = card.contexts_upstream_of_context_c(r, context_name, row, col)
    for p = 0, (points.size - 1) do
        api.nvim_buf_add_highlight(0, cns, 'WildMenu'
            ,tonumber(points.points[(p * 4) + 0])
            ,tonumber(points.points[(p * 4) + 1])
            ,tonumber(points.points[(p * 4) + 3]))
    end
    print(points.size.." contexts upstream of "..context_name)
end

function highlight_card_references_from_column(column_name, row, col)
    api.nvim_buf_clear_namespace(api.nvim_get_current_buf(), cns, 0, -1)
    vim.cmd('syntax off')
    local points = ffi.new("cd_nodelist[1]")
    points = card.references_from_column_c(r, column_name, row, col)
    for p = 0, (points.size - 1) do
        api.nvim_buf_add_highlight(api.nvim_get_current_buf(), cns, 'WildMenu'
            ,tonumber(points.points[(p * 4) + 0])
            ,tonumber(points.points[(p * 4) + 1])
            ,tonumber(points.points[(p * 4) + 3]))
    end
    print(points.size.." columns upstream of "..column_name)
end

function highlight_card_references_to_column(column_name, row, col)
    api.nvim_buf_clear_namespace(0, cns, 0, -1)
    vim.cmd('syntax off')
    local points = ffi.new("cd_nodelist[1]")
    points = card.references_to_column_c(r, column_name, row, col)
    for p = 0, (points.size - 1) do
        api.nvim_buf_add_highlight(0, cns, 'WildMenu'
            ,tonumber(points.points[(p * 4) + 0])
            ,tonumber(points.points[(p * 4) + 1])
            ,tonumber(points.points[(p * 4) + 3]))
    end
    print(points.size.." columns downstream of "..column_name)
end

function card_columns_in_table(table_name)
    ret = card_sa_columns_in_table(table_name)
    if sa_cit == 'CATALOG' then
        local usr_res = vim.fn.input("This requires a catalog query. Enter Y to use to allow this")
        if usr_res == 'Y' then
            ret = card_cat_columns_in_table(table_name)
        else
            return "GRANT PERMISSION TO SEE ALL COLUMNS"
        end
    end
    return ret
end

function print_card_columns_in_table(table_name)
    -- api.nvim_buf_clear_namespace(0, cns, 0, -1)
    -- vim.cmd('syntax off')
    local fields = ffi.new("cd_stringlist[1]")
    fields = card.result_columns_for_table_c(r, table_name)
    for i = 0, (fields.size - 1) do
        print ("\t"..ffi.string(fields.fields[i]))
    end
end

function card_jump_to_context_definition()
    local context_name = vim.fn.input('')
    local point = ffi.new('TSPoint[1]')
    point[0].row    = api.nvim_win_get_cursor(0)[1] - 1
    point[0].column = api.nvim_win_get_cursor(0)[2]
    local def = ffi.new("cd_nodelist[1]")
    print()
    def = card.context_definition_c(r, point[0], context_name)
    if def.points[0] == 1 then
        print('No context named '..context_name..' found')
    end
    vim.cmd('call cursor('..(def.points[0] + 1)..','..(def.points[1] + 1)..')')
end

function card_jump_to_column_definition()
    local row    = tonumber(api.nvim_win_get_cursor(0)[1] - 1)
    local column = tonumber(api.nvim_win_get_cursor(0)[2])
    local def = ffi.new("cd_nodelist[1]")
    print()
    def = card.column_definition_c(r, row, column)
    if def.size == 0 then
        print('No column named found')
        return
    elseif def.size > 1 then
        print('ERROR: more than one possible definitions of that column.')
    else
        vim.cmd('call cursor('..(def.points[0] + 1)..','..(def.points[1] + 1)..')')
    end
end

function card_reset()
    api.nvim_buf_clear_namespace(0, cns, 0, -1)
    vim.cmd('syntax on')
end

vim.cmd("let mapleader=',' ")
-- visual mode shortcuts
vim.keymap.set('v', '<Leader>t'
    ,':lua highlight_card_references_to_context(get_visual_selection(), vim.fn.getpos("\'<")[2], vim.fn.getpos("\'<")[1])<CR>')
vim.keymap.set('v', '<Leader>f'
    ,':lua highlight_card_references_from_context(get_visual_selection(), vim.fn.getpos("\'<")[2], vim.fn.getpos("\'<")[1])<CR>')
vim.keymap.set('v', '<Leader>j'
    ,':lua highlight_card_downstream_of_context(get_visual_selection(), vim.fn.getpos("\'<")[2], vim.fn.getpos("\'<")[1])<CR>')
vim.keymap.set('v', '<Leader>k'
    ,':lua highlight_card_upstream_of_context(get_visual_selection(), vim.fn.getpos("\'<")[2], vim.fn.getpos("\'<")[1])<CR>')
vim.keymap.set('v', '<Leader>u'
    ,':lua highlight_card_references_from_column(get_visual_selection(), vim.fn.getpos("\'<")[2], vim.fn.getpos("\'<")[1])<CR>')
vim.keymap.set('v', '<Leader>d'
    ,':lua highlight_card_references_to_column(get_visual_selection(), vim.fn.getpos("\'<")[2], vim.fn.getpos("\'<")[1])<CR>')
vim.keymap.set('v', '<Leader>c'
    ,':lua highlight_card_field_defs_in_context(get_visual_selection(), vim.fn.getpos("\'<")[2], vim.fn.getpos("\'<")[1])<CR>')

-- normal mode shortcust
vim.keymap.set('n', '<Leader>h'
    ,':lua highlight_card_references_from_context(card_get_parent_context(), vim.fn.getpos("\'<")[2], vim.fn.getpos("\'<")[1])<CR>')
vim.keymap.set('n', '<Leader>p',':lua highlight_card_parent_context()<CR>')
vim.keymap.set('n', '<Leader>D',':lua highlight_card_context_ddl()<CR>')
vim.keymap.set('n', '<Leader>/',':lua card_jump_to_context_definition();<CR>')
vim.keymap.set('n', '<Leader>^',':lua card_jump_to_column_definition();<CR>')
vim.keymap.set('n', '<Leader>r', ':lua card_reset()<CR>')

-- query shortcuts
vim.keymap.set('n', '<Leader>e', ":normal V<CR> :'<,'>%DB duckdb:///<CR>")
vim.keymap.set('v', '<Leader>e', ":'<,'>%DB duckdb:///<CR>")

print "CARD loaded"

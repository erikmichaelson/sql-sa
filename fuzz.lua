require "nvim_ext.lua"
require "math"
require "os"

math.randomseed(os.time)

LINES = api.nvim_buf_get_lines(0, 0, -1, true)
SOURCE = table.concat(LINES, "\n")
ROWS = len(LINES)

-- don't even do anything with the output just burn it
for i in 0..100 do
    local row = math.random(0, #ROWS)
    local col = math.random(0, #LINES[randrow])
    card.parent_context_c(source, point[0])
    card.context_definition_c(source, point[0], context_name)
    card.context_ddl_c(source, point[0])
    card.context_definition_c(source, point[0], context_name)
    card.field_definitions_in_context_c(source, context_name, row, col)
    card.references_from_context_c(source, context_name, row, col)
    card.references_to_context_c(source, context_name, row, col)
    card.tables_downstream_of_context_c(source, context_name, row, col)
    card.contexts_upstream_of_context_c(source, context_name, row, col)
    card.columns_one_up_of_column_c(source, column_name, row, col)
    card.columns_one_down_of_column_c(source, column_name, row, col)
    card.result_columns_for_table_c(source, table_name)
    card.context_definition_c(source, point[0], context_name)
    card.column_definition_c(source, row, column)
end

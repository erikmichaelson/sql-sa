require "nvim_ext"

---https://stackoverflow.com/questions/20325332/how-to-check-if-two-tablesobjects-have-the-same-value-in-lua
---@param o1 any|table First object to compare
---@param o2 any|table Second object to compare
---@param ignore_mt boolean True to ignore metatables (a recursive function to tests tables inside tables)
function tbl_equals(o1, o2, ignore_mt)
    if o1 == o2 then return true end
    local o1Type = type(o1)
    local o2Type = type(o2)
    if o1Type ~= o2Type then return false end
    if o1Type ~= 'table' then return false end

    if not ignore_mt then
        local mt1 = getmetatable(o1)
        if mt1 and mt1.__eq then
            --compare using built in method
            return o1 == o2
        end
    end

    local keySet = {}

    for key1, value1 in pairs(o1) do
        local value2 = o2[key1]
        if value2 == nil or tbl_equals(value1, value2, ignore_mt) == false then
            return false
        end
        keySet[key1] = true
    end

    for key2, _ in pairs(o2) do
        if not keySet[key2] then return false end
    end
    return true
end

vim.cmd(":view test/dag.sql")
vim.cmd(":bp")
--print(vim.cmd(":buffers"))

function test_card_parent_context()
    fails = 0
    vim.api.nvim_buf_call(1,
            function() 
                vim.cmd('call cursor(37, 0)') -- hacky but functional
                highlight_card_parent_context()
            end)
    res = vim.api.nvim_buf_get_extmarks(1, cns, 0, -1, {})
    exp = { { 1, 27, 13 } }
    if(not tbl_equals(res, exp, false)) then
        print("FAIL: parent context of dag.new_table. Expected "..vim.inspect(exp)..", got "..vim.inspect(res))
        fails = fails + 1
    end

    -- test a CTE
    vim.api.nvim_buf_call(1,
            function() 
                vim.cmd('call cursor(31, 0)') -- hacky but functional
                highlight_card_parent_context()
            end)
    res = vim.api.nvim_buf_get_extmarks(1, cns, 0, -1, {})
    exp = { { 1, 28, 9 } }
    if(not tbl_equals(res, exp, false)) then
        print("FAIL: parent context of dag.new_table > cust_level. Expected "..vim.inspect(exp)..", got "..vim.inspect(res))
        fails = fails + 1
    end

    -- test a subquery
    vim.api.nvim_buf_call(1,
            function() 
                vim.cmd('call cursor(38, 30)')
                highlight_card_parent_context()
            end)
    res = vim.api.nvim_buf_get_extmarks(1, cns, 0, -1, {})
    exp = { { 1, 38, 11 } }
    if(not tbl_equals(res, exp, false)) then
        print("FAIL: parent context of dag.new_table > tnaa. Expected "..vim.inspect(exp)..", got "..vim.inspect(res))
        fails = fails + 1
    end

    -- test the start of a create_table statement
    vim.api.nvim_buf_call(1,
            function() 
                vim.cmd('call cursor(1, 1)')
                highlight_card_parent_context()
            end)
    res = vim.api.nvim_buf_get_extmarks(1, cns, 0, -1, {})
    exp = { { 1, 0, 13 } }
    if(not tbl_equals(res, exp, false)) then
        print("FAIL: parent context of etl.leads. Expected "..vim.inspect(exp)..", got "..vim.inspect(res))
        fails = fails + 1
    end

    -- test a create table that's not a create_query
    vim.api.nvim_buf_call(1,
            function() 
                vim.cmd('call cursor(6, 10)')
                highlight_card_parent_context()
            end)
    res = vim.api.nvim_buf_get_extmarks(1, cns, 0, -1, {})
    exp = { { 1, 3, 13 } }
    if(not tbl_equals(res, exp, false)) then
        print("FAIL: parent context of etl.leads. Expected "..vim.inspect(exp)..", got "..vim.inspect(res))
        fails = fails + 1
    end

    return fails
end

function test_card_references_to_table()
    highlight_card_references_to_table('dag.new_table')
    res = vim.api.nvim_buf_get_extmarks(1, cns, 0, -1, {})
    exp = { { 1, 50, 9 } }
    if(not tbl_equals(res, exp, false)) then
        print("FAIL: refs to dag.new_table")
        fails = fails + 1
    end

    highlight_card_references_to_table('etl.leads')
    res = vim.api.nvim_buf_get_extmarks(1, cns, 0, -1, {})
    exp = { { 4, 24, 14 }, { 3, 31, 13 }, { 2, 35, 9 }, { 1, 44, 14 } }
    if(not tbl_equals(res, exp, false)) then
        print("FAIL: refs to etl.leads. Expected "..vim.inspect(exp)..", got "..vim.inspect(res))
        fails = fails + 1
    end

    -- unreferenced
    highlight_card_references_to_table('dag.three_deep')
    res = vim.api.nvim_buf_get_extmarks(1, cns, 0, -1, {})
    exp = { }
    if(not tbl_equals(res, exp, false)) then
        print("FAIL: refs to dag.three_deep. Expected "..vim.inspect(exp)..", got "..vim.inspect(res))
        fails = fails + 1
    end

    return fails
end

function test_card_references_from_context()
    fails = 0
    vim.api.nvim_buf_call(1,
            function() 
                highlight_card_references_from_context('dag.three_deep', 0, 0)
            end)
    res = vim.api.nvim_buf_get_extmarks(1, cns, 0, -1, {})
    exp = { { 2, 65, 9 }, { 1, 66, 15 } }
    if(not tbl_equals(res, exp, false)) then
        print("FAIL: refs from table [dag.three_deep]. Expected "
                ..vim.inspect(exp)..", got "..vim.inspect(res))
        fails = fails + 1
    end

    vim.api.nvim_buf_call(1,
            function() 
                highlight_card_references_from_context('dag.annotated_snapshot', 0, 0)
            end)
    res = vim.api.nvim_buf_get_extmarks(1, cns, 0, -1, {})
    exp = { { 2, 23, 9 }, { 1, 24, 14 } }
    if(not tbl_equals(res, exp, false)) then
        print("FAIL: refs from table [dag.annotated_snapshot]. Expected "
                ..vim.inspect(exp)..", got "..vim.inspect(res))
        fails = fails + 1
    end

    -- test a top level (table) context with a reference to a table, a CTE and a subquery
    vim.api.nvim_buf_call(1,
            function() 
                highlight_card_references_from_context('dag.new_table', 0, 0)
            end)
    res = vim.api.nvim_buf_get_extmarks(1, cns, 0, -1, {})
    exp = { { 3, 35, 9 }, { 2, 36, 14 }, { 1, 38, 11 } }
    if(not tbl_equals(res, exp, false)) then
        print("FAIL: refs from context [dag.new_table]. Expected "
                ..vim.inspect(exp)..", got "..vim.inspect(res))
        fails = fails + 1
    end

    return fails
end

function test_card_columns_in_table()

    -- test a fully specified (no *) table
    vim.api.nvim_buf_call(1, 
            function()
                card_columns_in_table('dag.annotated_snapshot')
            end)
    vim.api.nvim_buf_get_extmarks(1, cns, 0, -1, {})
    exp = { 'current_owner','first_owner','last_change','first_change_dt'
           ,'first_change','num_changes','lead_id' }
    if(not tbl_equals(res, exp, false)) then
        print("FAIL: columns in table [dag.annotated_snapshot]. Expected"
                ..vim.inspect(exp)..", got "..vim.inspect(res))
        fails = fails + 1
    end

    -- test a DDL (non-query) table
    vim.api.nvim_buf_call(1, 
            function()
                card_columns_in_table('etl.snapshot')
            end)
    vim.api.nvim_buf_get_extmarks(1, cns, 0, -1, {})
    exp = { 'change_by','new_value','old_value','field'
           ,'updated','lead_id','id' }
    if(not tbl_equals(res, exp, false)) then
        print("FAIL: columns in table [etl.snapshot]. Expected"
                ..vim.inspect(exp)..", got "..vim.inspect(res))
        fails = fails + 1
    end

    -- test select star
    vim.api.nvim_buf_call(1, 
            function()
                card_columns_in_table('dag.two_deep')
            end)
    vim.api.nvim_buf_get_extmarks(1, cns, 0, -1, {})
    exp = { 'num_leads','cust_nm' }
    if(not tbl_equals(res, exp, false)) then
        print("FAIL: columns in table [dag.two_deep]. Expected"
                ..vim.inspect(exp)..", got "..vim.inspect(res))
        fails = fails + 1
    end

    -- test aliased select star
    vim.api.nvim_buf_call(1, 
            function()
                card_columns_in_table('dag.another')
            end)
    vim.api.nvim_buf_get_extmarks(1, cns, 0, -1, {})
    exp = { '','num_leads' }
    if(not tbl_equals(res, exp, false)) then
        print("FAIL: columns in table [dag.another]. Expected"
                ..vim.inspect(exp)..", got "..vim.inspect(res))
        fails = fails + 1
    end

    return fails
end

fails =         test_card_references_from_context()
fails = fails + test_card_parent_context()
fails = fails + test_card_references_to_table()
card_reset()
print(fails.." test failures")

require "os"
--os.exit()

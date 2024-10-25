require "nvim_ext"
require "os"

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

------ QUERY TESTS ------
vim.cmd(":view test/cll_test.sql")
cll_buf = vim.api.nvim_get_current_buf()

function test_col_def_query()
    vim.api.nvim_buf_call(cll_buf,
        function()
            highlight_card_field_defs('dag.new_table', 91, 41)
        end
    )
end

------ INTEGRATION TESTS ------
vim.cmd(":view test/dag.sql")
dag_buf = vim.api.nvim_get_current_buf()

function test_card_parent_context()
    fails = 0
    print("parent context of dag.new_table")
    vim.api.nvim_buf_call(dag_buf,
            function() 
                vim.cmd('call cursor(37, 0)') -- hacky but functional
                highlight_card_parent_context()
            end)
    res = vim.api.nvim_buf_get_extmarks(dag_buf, cns, 0, -1, {})
    exp = { { 1, 27, 13 } }
    if(not tbl_equals(res, exp, false)) then
        print("FAIL: Expected "..vim.inspect(exp)..", got "..vim.inspect(res).."\n============")
        fails = fails + 1
    end

    -- test a CTE
    print("parent context of dag.new_table > cust_level")
    vim.api.nvim_buf_call(dag_buf,
            function() 
                vim.cmd('call cursor(31, 0)') -- hacky but functional
                highlight_card_parent_context()
            end)
    res = vim.api.nvim_buf_get_extmarks(dag_buf, cns, 0, -1, {})
    exp = { { 1, 28, 9 } }
    if(not tbl_equals(res, exp, false)) then
        print("FAIL: Expected "..vim.inspect(exp)..", got "..vim.inspect(res).."\n============")
        fails = fails + 1
    end

    -- test a subquery
    print("parent context of dag.new_table > tnaa")
    vim.api.nvim_buf_call(dag_buf,
            function() 
                vim.cmd('call cursor(38, 30)')
                highlight_card_parent_context()
            end)
    res = vim.api.nvim_buf_get_extmarks(dag_buf, cns, 0, -1, {})
    exp = { { 1, 38, 11 } }
    if(not tbl_equals(res, exp, false)) then
        print("FAIL: Expected "..vim.inspect(exp)..", got "..vim.inspect(res).."\n============")
        fails = fails + 1
    end

    -- test the start of a create_table statement
    print("parent context of etl.leads")
    vim.api.nvim_buf_call(dag_buf,
            function() 
                vim.cmd('call cursor(1, 1)')
                highlight_card_parent_context()
            end)
    res = vim.api.nvim_buf_get_extmarks(dag_buf, cns, 0, -1, {})
    exp = { { 1, 0, 13 } }
    if(not tbl_equals(res, exp, false)) then
        print("FAIL: Expected "..vim.inspect(exp)..", got "..vim.inspect(res).."\n============")
        fails = fails + 1
    end

    -- test a create table that's not a create_query
    print("parent context of etl.leads")
    vim.api.nvim_buf_call(dag_buf,
            function() 
                vim.cmd('call cursor(6, 10)')
                highlight_card_parent_context()
            end)
    res = vim.api.nvim_buf_get_extmarks(dag_buf, cns, 0, -1, {})
    exp = { { 1, 3, 13 } }
    if(not tbl_equals(res, exp, false)) then
        print("FAIL: Expected "..vim.inspect(exp)..", got "..vim.inspect(res).."\n============")
        fails = fails + 1
    end

    return fails
end

function test_card_references_to_context()
    print("refs to dag.new_table")
    highlight_card_references_to_context('dag.new_table', 38, 45)
    res = vim.api.nvim_buf_get_extmarks(dag_buf, cns, 0, -1, {})
    exp = { { 1, 50, 9 } }
    if(not tbl_equals(res, exp, false)) then
        print("FAIL: Expected "..vim.inspect(exp)..", got "..vim.inspect(res).."\n============")
        fails = fails + 1
    end

    print("refs to etl.leads")
    highlight_card_references_to_context('etl.leads', 24, 5)
    res = vim.api.nvim_buf_get_extmarks(dag_buf, cns, 0, -1, {})
    exp = { { 4, 24, 14 }, { 3, 31, 13 }, { 2, 35, 9 }, { 1, 44, 14 } }
    if(not tbl_equals(res, exp, false)) then
        print("FAIL: Expected "..vim.inspect(exp)..", got "..vim.inspect(res).."\n============")
        fails = fails + 1
    end

    -- unreferenced
    print("refs to dag.three_deep")
    highlight_card_references_to_context('dag.three_deep', 5, 10)
    res = vim.api.nvim_buf_get_extmarks(dag_buf, cns, 0, -1, {})
    exp = { }
    if(not tbl_equals(res, exp, false)) then
        print("FAIL: Expected "..vim.inspect(exp)..", got "..vim.inspect(res).."\n============")
        fails = fails + 1
    end

    return fails
end

function test_card_references_from_context()
    fails = 0
    vim.api.nvim_buf_call(dag_buf,
            function() 
                highlight_card_references_from_context('dag.three_deep', 0, 0)
            end)
    res = vim.api.nvim_buf_get_extmarks(dag_buf, cns, 0, -1, {})
    exp = { { 2, 65, 9 }, { 1, 66, 15 } }
    if(not tbl_equals(res, exp, false)) then
        print("FAIL: refs from table [dag.three_deep]. Expected "
                ..vim.inspect(exp)..", got "..vim.inspect(res))
        fails = fails + 1
    end

    vim.api.nvim_buf_call(dag_buf,
            function() 
                highlight_card_references_from_context('dag.annotated_snapshot', 0, 0)
            end)
    res = vim.api.nvim_buf_get_extmarks(dag_buf, cns, 0, -1, {})
    exp = { { 2, 23, 9 }, { 1, 24, 14 } }
    if(not tbl_equals(res, exp, false)) then
        print("FAIL: refs from table [dag.annotated_snapshot]. Expected "
                ..vim.inspect(exp)..", got "..vim.inspect(res))
        fails = fails + 1
    end

    -- test a top level (table) context with a reference to a table, a CTE and a subquery
    vim.api.nvim_buf_call(dag_buf,
            function() 
                highlight_card_references_from_context('dag.new_table', 0, 0)
            end)
    res = vim.api.nvim_buf_get_extmarks(dag_buf, cns, 0, -1, {})
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
    vim.api.nvim_buf_call(dag_buf, 
            function()
                card_columns_in_table('dag.annotated_snapshot')
            end)
    res = vim.api.nvim_buf_get_extmarks(dag_buf, cns, 0, -1, {})
    exp = { 'current_owner','first_owner','last_change','first_change_dt'
           ,'first_change','num_changes','lead_id' }
    if(not tbl_equals(res, exp, false)) then
        print("FAIL: columns in table [dag.annotated_snapshot]. Expected"
                ..vim.inspect(exp)..", got "..vim.inspect(res))
        fails = fails + 1
    end

    -- test a DDL (non-query) table
    vim.api.nvim_buf_call(dag_buf, 
            function()
                card_columns_in_table('etl.snapshot')
            end)
    res = vim.api.nvim_buf_get_extmarks(dag_buf, cns, 0, -1, {})
    exp = { 'change_by','new_value','old_value','field'
           ,'updated','lead_id','id' }
    if(not tbl_equals(res, exp, false)) then
        print("FAIL: columns in table [etl.snapshot]. Expected"
                ..vim.inspect(exp)..", got "..vim.inspect(res))
        fails = fails + 1
    end

    -- test select star
    vim.api.nvim_buf_call(dag_buf, 
            function()
                card_columns_in_table('dag.two_deep')
            end)
    res = vim.api.nvim_buf_get_extmarks(dag_buf, cns, 0, -1, {})
    exp = { 'num_leads','cust_nm' }
    if(not tbl_equals(res, exp, false)) then
        print("FAIL: columns in table [dag.two_deep]. Expected"
                ..vim.inspect(exp)..", got "..vim.inspect(res))
        fails = fails + 1
    end

    -- test aliased select star
    vim.api.nvim_buf_call(dag_buf,
            function()
                card_columns_in_table('dag.another')
            end)
    res = vim.api.nvim_buf_get_extmarks(dag_buf, cns, 0, -1, {})
    exp = { '','num_leads' }
    if(not tbl_equals(res, exp, false)) then
        print("FAIL: columns in table [dag.another]. Expected"
                ..vim.inspect(exp)..", got "..vim.inspect(res))
        fails = fails + 1
    end

    -- test subcontexts
    vim.api.nvim_buf_call(dag_buf,
            function()
                card_columns_in_table('dag.new_table')
            end)
    res = vim.api.nvim_buf_get_extmarks(dag_buf, cns, 0, -1, {})
    exp = { 'start_dt','end_dt','cust_ssn','num_booked','num_leads' }
    if(not tbl_equals(res, exp, false)) then
        print("FAIL: columns in table [dag.new_table]. Expected"
                ..vim.inspect(exp)..", got "..vim.inspect(res))
        fails = fails + 1
    end

    return fails
end

function test_card_contexts_upstream_of()
    vim.api.nvim_buf_call(dag_buf,
            function()
                highlight_card_upstream_of_context('dag.three_deep',1,1)
            end)
    res = vim.api.nvim_buf_get_extmarks(dag_buf, cns, 0, -1, {})
    exp = { { 3, 0, 13 }, { 2, 1, 13 }, { 6, 27, 13 }, { 4, 28, 9 }
           ,{ 5, 38, 11 }, { 7, 48, 13 }, { 1, 53, 13 }, { 8, 63, 13 } }
    if(not tbl_equals(res, exp, false)) then
        print("FAILS: contexts upstream of [dag.three_deep]. Expected"
                ..vim.inspect(exp)..", got "..vim.inspect(res))
        fails = fails + 1
    end

    vim.api.nvim_buf_call(dag_buf,
            function()
                highlight_card_upstream_of_context('tnaa',38,11)
            end)
    res = vim.api.nvim_buf_get_extmarks(dag_buf, cns, 0, -1, {})
    exp = { { 1, 1, 13 }, { 2, 38, 11 } }
    if(not tbl_equals(res, exp, false)) then
        print("FAILS: contexts upstream of [dag.new_table > tnaa]. Expected"
                ..vim.inspect(exp)..", got "..vim.inspect(res))
        fails = fails + 1
    end

    return fails
end

function test_card_columns_one_up_of()
    --vim.cmd(":view test/cll_test.sql")
    -- get two basic references from a column 
    vim.api.nvim_buf_call(cll_buf,
            function()
                highlight_card_one_up_of_column('hmda_county_code',32,57)
            end)
    res = vim.api.nvim_buf_get_extmarks(cll_buf, cns, 0, -1, {})
    exp = { { 2, 13, 31 }, { 1, 23, 20 } }
    if(not tbl_equals(res, exp, false)) then
        print("FAILS: columns one up of [combined.hmda_county_code]. Expected"
                ..vim.inspect(exp)..", got "..vim.inspect(res))
        fails = fails + 1
    end

    -- get column where upstream definition has an alias
    vim.api.nvim_buf_call(cll_buf,
            function()
                highlight_card_one_up_of_column('start_dt',92,65)
            end)
    res = vim.api.nvim_buf_get_extmarks(cll_buf, cns, 0, -1, {})
    exp = { { 1, 95, 30 } }
    if(not tbl_equals(res, exp, false)) then
        print("FAILS: columns one up of [dag.new_table.tnaa.start_dt]. Expected"
                ..vim.inspect(exp)..", got "..vim.inspect(res))
        fails = fails + 1
    end

    -- get column where reference has an alias
    vim.api.nvim_buf_call(cll_buf,
            function()
                highlight_card_one_up_of_column('cust_ssn',92,14)
            end)
    res = vim.api.nvim_buf_get_extmarks(cll_buf, cns, 0, -1, {})
    exp = { { 1, 86, 15 } }
    if(not tbl_equals(res, exp, false)) then
        print("FAILS: columns one up of [new_table.cust_ssn]. Expected"
                ..vim.inspect(exp)..", got "..vim.inspect(res))
        fails = fails + 1
    end

    -- get column defined in CTE
    vim.api.nvim_buf_call(cll_buf,
            function()
                highlight_card_one_up_of_column('num_leads',92,33)
            end)
    res = vim.api.nvim_buf_get_extmarks(cll_buf, cns, 0, -1, {})
    exp = { { 1, 86, 38 } }
    if(not tbl_equals(res, exp, false)) then
        print("FAILS: columns one up of [dag.new_table.num_leads]. Expected"
                ..vim.inspect(exp)..", got "..vim.inspect(res))
        fails = fails + 1
    end

    -- get column defined in table *and* CTE (only return table)
    vim.api.nvim_buf_call(cll_buf,
            function()
                highlight_card_one_up_of_column('num_leads',100,21)
            end)
    res = vim.api.nvim_buf_get_extmarks(cll_buf, cns, 0, -1, {})
    exp = { { 1, 91, 32 } }
    if(not tbl_equals(res, exp, false)) then
        print("FAILS: columns one up of [dag.anonymized.num_leads]. Expected"
                ..vim.inspect(exp)..", got "..vim.inspect(res))
        fails = fails + 1
    end

    -- pseudo bind column with an alias but is defined in two tables
    vim.api.nvim_buf_call(cll_buf,
            function()
                highlight_card_one_up_of_column('last_name',107,14)
            end)
    res = vim.api.nvim_buf_get_extmarks(cll_buf, cns, 0, -1, {})
    exp = { { 1, 105, 44 } }
    if(not tbl_equals(res, exp, false)) then
        print("FAILS: columns one up of [take_your_pick.last_name]. Expected"
                ..vim.inspect(exp)..", got "..vim.inspect(res))
        fails = fails + 1
    end

    -- pseudo bind with two tables (other column - coming from other table)
    vim.api.nvim_buf_call(cll_buf,
            function()
                highlight_card_one_up_of_column('first_name',107,14)
            end)
    res = vim.api.nvim_buf_get_extmarks(cll_buf, cns, 0, -1, {})
    exp = { { 1, 104, 32 } }
    if(not tbl_equals(res, exp, false)) then
        print("FAILS: columns one up of [take_your_pick.first_name]. Expected"
                ..vim.inspect(exp)..", got "..vim.inspect(res))
        fails = fails + 1
    end

    -- get column from column_definition DDL (not create_query)
    vim.api.nvim_buf_call(cll_buf,
            function()
                highlight_card_one_up_of_column('cust_ssn',87,16)
            end)
    res = vim.api.nvim_buf_get_extmarks(cll_buf, cns, 0, -1, {})
    exp = { { 1, 78, 27 } }
    if(not tbl_equals(res, exp, false)) then
        print("FAILS: columns one up of [dag.new_table > cust_level.cust_ssn]. Expected"
                ..vim.inspect(exp)..", got "..vim.inspect(res))
        fails = fails + 1
    end
 
    -- this segfaulted it for some reason
    vim.api.nvim_buf_call(cll_buf,
            function()
                highlight_card_one_up_of_column('owner_id',92,50)
            end)
    res = vim.api.nvim_buf_get_extmarks(cll_buf, cns, 0, -1, {})
    exp = { { 1, 78, 89 } }
    if(not tbl_equals(res, exp, false)) then
        print("FAILS: columns one up of [dag.new_table.cust_ssn]. Expected"
                ..vim.inspect(exp)..", got "..vim.inspect(res))
        fails = fails + 1
    end

    -------- JUMP TO DEFINITION --------
    -- make sure this doesn't match the substring of "c" to "c|ust_nm" in etl.leads
    vim.api.nvim_buf_call(cll_buf,
            function()
                vim.cmd(":call cursor(92,12)")
                card_jump_to_column_definition()
            end)
    res = vim.api.nvim_win_get_cursor(0)
    exp = { { 1, 78, 89 } }
    if(not tbl_equals(res, exp, false)) then
        print("FAILS: jump to column definition at 92,12 of [dag.new_table.cust_ssn]. Expected"
                ..vim.inspect(exp)..", got "..vim.inspect(res))
        fails = fails + 1
    end

    -- Needs to error / return root_node.
    vim.api.nvim_buf_call(cll_buf,
            function()
                vim.cmd(":call cursor(109,39)")
                card_jump_to_column_definition()
            end)
    res = vim.api.nvim_win_get_cursor(0)
    exp = { { 0, 0, 1 } }
    if(not tbl_equals(res, exp, false)) then
        print("FAILS: jump to column definition at 109,39 of [take_your_pick.agent]. Expected"
                ..vim.inspect(exp)..", got "..vim.inspect(res))
        fails = fails + 1
    end

    -- this diverged from one up of behavior... whoops. Not good. Needs to error
    -- Make sure the source code here has cust_ssn unaliased and defined in 2 places
    vim.api.nvim_buf_call(cll_buf,
            function()
                vim.cmd(":call cursor(92,10)")
                card_jump_to_column_definition()
            end)
    res = vim.api.nvim_win_get_cursor(0)
    exp = { { 0, 0, 1 } }
    if(not tbl_equals(res, exp, false)) then
        print("FAILS: jump to column definition at 93,10 of [dag.new_table.cust_ssn]. Expected"
                ..vim.inspect(exp)..", got "..vim.inspect(res))
        fails = fails + 1
    end

    --vim.cmd(":view test/dag_test.sql")
    vim.api.nvim_buf_call(dag_buf,
            function()
                highlight_card_upstream_of_context('tnaa',38,11)
            end)
    res = vim.api.nvim_buf_get_extmarks(dag_buf, cns, 0, -1, {})
    exp = { { 1, 1, 13 }, { 2, 38, 11 } }
    if(not tbl_equals(res, exp, false)) then
        print("FAILS: columns one up of [dag.new_table > tnaa]. Expected"
                ..vim.inspect(exp)..", got "..vim.inspect(res))
        fails = fails + 1
    end

    return fails
end

fails =         test_card_references_from_context()
fails = fails + test_card_parent_context()
fails = fails + test_card_references_to_context()
--fails = fails + test_card_columns_in_table()
fails = fails + test_card_contexts_upstream_of()
fails = fails + test_card_columns_one_up_of()
card_reset()
print(fails.." test failures")

--os.exit()

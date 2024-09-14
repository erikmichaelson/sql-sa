require "nvim_ext"

vim.cmd(":view test/dag.sql")
vim.cmd(":bp")
--print(vim.cmd(":buffers"))

vim.api.nvim_buf_call(2,
        function() 
            highlight_card_references_from_context('dag.three_deep', 65, 12)
        end)

vim.print(vim.api.nvim_buf_get_extmarks(2, cns, 0, -1, {}))

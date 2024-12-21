events = {}
vim.api.nvim_buf_attach(1,false, {
    on_lines = function(...)
        table.insert(events, {...})
    end,
})

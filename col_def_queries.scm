(column_definitions 
  (column_definition name: (identifier) @col_def))

(select
  (select_expression
    (term [value: (field 
                    . name: (identifier) .
                  )
                    alias: (identifier)
                    (all_fields)
                  ] @old_def)))

(select (select_expression (term [value: (field . (object_reference)?
                            @col_source name: (identifier) @definition)
                            alias: (identifier) (all_fields) @definition ] )))

(select (select_expression (term [
              (all_fields) @d
              (field (object_reference)? @cs
                name: (identifier) @d)
              alias: (identifier) @d ] )))

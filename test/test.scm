1=====
dag.sql --show references from 'dag.new_table'
-----
29:cust_level [CTE]
1:etl.leads [table]
1:etl.leads [table]
2:etl.salesmen [table]

2=====
dag.sql --show references to 'dag.new_table'
-----

3=====
dag.sql --show references to 'etl.snapshot'
-----
14:dag.annotated_snapshot [table]

create table etl.leads as (select * from 'leads.parquet');
create table etl.salesmen as (select * from 'salesmen.parquet');
create table etl.listings as (select * from 'listings.parquet');
create table etl.snapshot (
    id      int autoincrement,
    lead_id int foreign key source.leads("id"),
    updated date,
    field   text,
    old_value text,
    new_value text,
    change_by int
);

create table dag.annotated_snapshot as (
    lead_id,
    count(*) as num_changes,
    first_value(new_value) over (partition by lead_id order by updated) as first_change,
    min(updated) as first_change_dt,
    last_value(new_value) over (partition by lead_id order by updated) as last_change,
    first_value(case when field = 'OWNER' then old_value else null end) over (partition by lead_id order by updated) as first_owner,
    l.owner as current_owner

    from etl.snapshots sn
    left join etl.leads l on (sn.lead_id = l.id) 
);

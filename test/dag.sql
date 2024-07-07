create table etl.leads as (select * from 'leads.parquet');
create table etl.salesmen as (select * from 'salesmen.parquet');
create table etl.listings as (select * from 'listings.parquet');
create table etl.snapshot (
    id      primary key,
    lead_id int references source.leads("id"),
    updated date,
    field   text,
    old_value text,
    new_value text,
    change_by int
);

create table dag.annotated_snapshot as (
    select
    lead_id,
    count(*) as num_changes,
    first_value(new_value) over (partition by lead_id order by updated) as first_change,
    min(updated) as first_change_dt,
    last_value(new_value) over (partition by lead_id order by updated) as last_change,
    first_value(case when field = 'OWNER' then old_value else null end) over (partition by lead_id order by updated) as first_owner,
    l.owner as current_owner

    from etl.snapshot sn
    left join etl.leads l on (sn.lead_id = l.id) 
);

create table dag.new_table as (
    with cust_level as (
        select cust_ssn, count(*) as num_leads
              ,sum(case when booked then 1 else 0 end) as num_booked
        from elt.leads
        group by cust_ssn
    )
    select * 
    from etl.leads l
    left join cust_level c on l.cust_ssn = c.cust_ssn
    left join (select s.start_dt, s.end_dt) from etl.salesmen s on (l.owner_id = s.id)) as tnaa
);

create table dag.another as (
    select e.*, count(l.id)
    from etl.employee
    left join etl.leads l on (l.owner_id = e.id)
    group by e.*
);

create table dag.anonymized as (
    select cust_nm, num_leads
    from dag.new_table
)

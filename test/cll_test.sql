INSTALL spatial;
drop table if exists central_county_parcels;
create table central_county_parcels as (select * from '~/CS/data/metrics_project/actually.shp');

select * from central_county_parcels limit 1000;

select CityName, count(*)
from central_county_parcels
group by CityName

create table counties as (
    select
        trim(NAME)          as cnty_nm,
        rpad(ID, '0', 3)    as county_fips_cde,
        -- alias is entirely for testing purposes
        rc.ST_ID            as state
    from raw_counties rc
);

create table fipsize as (
    select
        case when stnm = 'CA' then '06'
             when stnm = 'MN' then '27'
             end as state_fips
    from states
    -- part two is having some understanding of the filters too
    where stnm in ('CA', 'MN')
);

create table combined as (
    select
        state_fips || county_fips_cde as hmda_county_code
    from fipsize f
    left join counties c on f.state_fips = c.state
);

create table parcels_per_county as (
    select
     hmda_county_code
    ,case when hmda_county_code = '27053' then 'Hennepin'
          when hmda_county_code = '' then 'Ramsey' 
          else 'ERROR' end as county_name
    ,count(*) as num_parcels

    from central_county_parcels ccp
    left join combined c on (c.FIPS_CODE = ccp.hmda_county_code)
    group by hmda_county_code
);

drop table if exists first_half;
create table first_half as (
    select 
        'h' as hey,
        'l' as l,
        'w' as w,
        'r' as r
);

drop table if exists second_half;
create table second_half as (
    select 
        'l' as l,
        'o' as o,
        'd' as d
);

drop table if exists freaked;
create table freaked as (
    select
        hey || 'e' || l || l || o || w || o || r || l || d as singleton
    from first_half fh
    join second_half sh
);


create schema etl;
create schema dag;
drop table if exists etl.leads;
create table etl.leads    (cust_ssn text, cust_nm text, booked smallint, create_dt date, owner_id int);
drop table if exists etl.salesmen; 
create table etl.salesmen (id int, start_dt date, end_dt date);

select e.* from etl.leads
--- copied from dag.sql to check regression on matching num_leads in cust_level instead of new_table
create table dag.new_table as (
    with cust_level as (
        select cust_ssn, count(id) as num_leads
              ,sum(case when booked then 1 else 0 end) as num_booked
        from etl.leads
        group by cust_ssn
    )
    select c.cust_ssn, cust_nm, num_leads, owner_id, create_dt, start_dt
        ,cust_ssn || l.owner_id as smush
    from etl.leads as l
    left join cust_level as c on l.cust_ssn = c.cust_ssn
    left join (select s.id, s.start_dt, s.end_dt from etl.salesmen s)
        as tnaa on tnaa.start_dt = l.create_dt and l.owner_id = tnaa.id
);

create table dag.anonymized as (
    select cust_nm, num_leads
    from dag.new_table
);

create table ballers as (select first_name, last_name, agent, team     from 'ballers.csv');
create table rappers as (select first_name, last_name, agent, hometown from 'rappers.csv');

create table take_your_pick as (
    select b.first_name, r.last_name, agent /* supposed to error */, team
    from ballers b
    left join rappers as r on (b.first_name = r.first_name)
);


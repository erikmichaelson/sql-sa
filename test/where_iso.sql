create table dulu_hmda as (select * from '/Users/erikmichaelson/CS/data/hmda/2023_combined_mlar.parquet' where county_code = '27137');

select * from dulu_hmda

create view dhmda_clean as (select * exclude(loan_amount)
                                    ,case when loan_amount not in ('NA','Exempt') then loan_amount::int else null end as loan_amount
                            from dulu_hmda)

select census_tract, count(*), avg(loan_amount) from dhmda_clean 
where action_taken = '6'
group by census_tract order by count(*) desc

load spatial;
drop table if exists dulu_tracts;
create table dulu_tracts as (
    select * 
    from ST_Read('/Users/erikmichaelson/CS/data/mn_census_2020_population/Census2020PopulationTract.dbf') t
    inner join ST_Read('/Users/erikmichaelson/CS/data/tl_2020_27_tract/tl_2020_27_tract.shp') shp
        on shp.GEOID = t.GEOG_UNIT
    inner join dhmda_clean hmda on t.GEOG_UNIT = hmda.census_tract
);

select * from dulu_tracts

select GEONAME
,sum(case when action_taken = '1' then 1 else 0 end) as originated
,sum(case when action_taken = '6' then 1 else 0 end) as purchased
,avg(loan_amount)::int as loan_amount
,count(*) 
from dulu_tracts group by 1 order by 2 desc

select * from pg_views

select * from duckdb_databases

load spatial;
select * from st_drivers()


select * from dulu_tracts t
inner join mn_shapes shp on (shp.GEOID = t.GEOG_UNIT)

load spatial;
copy dulu_tracts to '/Users/erikmichaelson/CS/data/dulu/dulu_hmda.shp' with (format gdal, driver 'ESRI Shapefile')

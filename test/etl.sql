create table state as (select * from 'states.csv');
create table fips as (select * from 'fips.csv');
alter table fips add primary key ("GEO_CDE");
alter table fips add foreign key ("ST") references state("abr");

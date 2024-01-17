create table blobs (
    id      primary key,
    name    text not null,
    age     int not null,
    favorite_color text check(favorite_color = 'orange'),
    kill_count  int
);

create table lamas (
    id      primary key,
    name    text not null
    age     int not null,
    squishiness float,
    stim        float
);

create table regions (
    id          int, -- not a PK - unique *within* nation (stage 2)
    nation      text check(nations in ('Lamatia', 'Blobatia'),
    name        text not null,
    population  int not null check(population > 0),
    area        int not null check(area > 0),
    gdp         int check(gdp > 0)
);

create table staging.gd_regions as (
    select
         nation
        ,id
        ,name
        ,population / area as population_density
        ,gdp / population as gdp_per_capita
    
    from regions
);

create table staging.all_mystical_animals as (
    (select id, 'lama' as species, name, age from lamas)
    union all
    (select id, 'blob' as species, name, age from blobs)
);

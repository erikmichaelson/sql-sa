create table one as (
    with row_number as (

    )
    select c.*, rn.row_number, rn.num_cars
    from cars c
    left join row_number rn on (rn.car_id = c.id)
    where rn.row_number = 1
);

create table two as (
    with row_number as (
        select ssn
            ,home_id
            ,row_number() over (parition by ssn order by purchase_date)
            ,count(*) over (partition by ssn) as num_homes
        from homes h
    )
    select h.*, rn.row_number, rn.num_homes
    from homes h
    left join row_number rn on (rn.home_id = h.home_id and rn.row_number = 1)
);

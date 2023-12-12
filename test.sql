select * from state;

select a.id
      ,a.name
      ,a.tract_cde
      ,b.id
from tract a
left join state b on (a.st = b.id);

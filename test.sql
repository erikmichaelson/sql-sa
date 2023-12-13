select * from state;

select a.id
      ,a.name
      ,a.tract_cde
      ,b.id
from geo.tract a
left join geo.state b on (a.st = b.id);

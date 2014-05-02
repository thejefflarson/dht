{%

%}

%token LIST
%token NUMBER


%%

list:
  LIST member END

dictionary:
  DICT member END

integer:
  INT NUMBER END

dict_value:
  member member

member:
|  integer
|  STRING
|  dict_value
|  list
|  member
;
%%
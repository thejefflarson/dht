{%

%}

%%

list:
  LIST member END

dict:
  DICT member END

member:
|  INT NUMBER END
|  STRING
|  dict
|  list
|  member
;
%%
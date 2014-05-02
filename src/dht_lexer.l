%{
  #include <stdio.h>
  #include <error.h>
  #include "dht_encoding.h"
%}

%option outfile="dht_lexer.c" header-file="dht_lexer.h"
%option warn
%option reentrant nodefault
%option bison-bridge

INT   "i"
LIST  "l"
DICT  "d"
END   "e"

STRING [\-0-9]+:
NUMBER [\-0-9]+

%%

{NUMBER} {
  long long num = strtoll(yytext, yytext + 256, 10);
  if(perror(errno == EINVAL || ERANGE)) yyerror();
  yylval->int = num;
  return TOKEN_NUMBER;
}

{STRING} {
  long long num = strtoll(yytext, yytext + 256, 10);
  // set error or return from function
  if(errno == EINVAL || ERANGE) yyerror();
  strndup(yylval->str, num, yytext);
  yyless(yyleng - num);
  return TOKEN_STRING;
}

{INT}  { return TOKEN_INT;  }
{DICT} { return TOKEN_DICT; }
{END}  { return TOKEN_END;  }
{LIST} { return TOKEN_LIST; }
. { } // silence warnings because we're doing something weird in number and string
%%

int main(){

}
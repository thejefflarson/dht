%{
  #include <stdio.h>
  #include <error.h>
%}

%option outfile="dht_lexer.c" header-file="dht_lexer.h"
%option warn nodefault

%option reentrant noyywrap never-interactive nounistd
%option bison-bridge

INT   "i"
LIST  "l"
DICT  "d"
END   "e"

NUMBER [\-0-9]+
STRING [\-0-9]+:

%%

{INT} {
  long long num = strtoll(yytext, yytext + 256, 10);
  // set error or return from function
  yylval->value = num;
  return TOKEN_INT;
}

{STRING} {
  long long num = strtoll(yytext, yytext + 256, 10);
  // set error or return from function
  yylval->value = num;
  return TOKEN_STRING;
}

{DICT} { return TOKEN_DICT; }
{END}  { return TOKEN_END; }
{LIST} { return TOKEN_LIST; }
. { }

%%

int main(){

}
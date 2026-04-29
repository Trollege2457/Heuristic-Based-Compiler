%{
#include "tokens.h"
#include <stdlib.h>
%}

%%

"if"        { return IF; }
"else"      { return ELSE; }

"+"         { return PLUS; }
"-"         { return MINUS; }
"*"         { return MUL; }
"/"         { return DIV; }
"%"         { return REM; }

"("         { return LPAREN; }
")"         { return RPAREN; }

";"         { return SEMI; }
"->"        { return ARROW; }
"|"         { return PIPE; }

[a-zA-Z][a-zA-Z0-9_]*   { return ID; }
[0-9]+                  { return NUM; }

[ \t\n]     ;

.           { return -1; }

%%

int yywrap() { return 1; }

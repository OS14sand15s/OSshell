%{
    #include "global.h"
    #include "bison.tab.h"
   
static size_t
myinput(char *s, int size)
{
    static char *last;
    prompt();
    if (fgets(s, size, yyin)) {
        if (last)
            addHistory(last);
        last = strdup(s);
        return strlen(s);
    }
    return 0;
}
#define YY_INPUT(buf, result, maxsize) result = myinput(buf, maxsize)

%}

%%
"cd"([ \t]+[^ \t\n]+)* 		{inputBuff=yytext;  return CDCMD;}
"exit"       		             {inputBuff=yytext;  return EXIT;}
"history"  		            {inputBuff=yytext;  return HISTORY;}
"jobs"      		{inputBuff=yytext;  return JOBS;}
"fg"[ \t]+"%"[0-9]+ {inputBuff=yytext;  return FGCMD;}
"bg"[ \t]+"%"[0-9]+ {inputBuff=yytext;  return BGCMD;}

[^ \t\n]+([ \t]+[^ \t\n]+)?("|"[ \t]?[^ \t\n]+([ \t]+[^ \t\n]+)?)+("&")?			{inputBuff=yytext;  return PIPECMD;}
[^ \t\n]+"<"[ \t]*[^ \t\n]+([ \t]*">"[ \t]*[^ \t\n]+)?			{inputBuff=yytext;  return INRE;}
[^ \t\n]+">"[ \t]*[^ \t\n]+([ \t]*"<"[ \t]*[^ \t\n]+)?                  {inputBuff=yytext;  return OUTRE;}
[^ \t\n]+([ \t]+[^ \t\n]+)?         {inputBuff=yytext; return CMD;}
[ \t]+             {/*empty*/}
"\n"                {return EOL;}

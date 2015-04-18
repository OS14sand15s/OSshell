%{
    enum yytokentype{
    	CDCMD=258,
    	EXIT=259,
    	HISTORY=260,
    	JOBS=261,
    	FGCMD=262,
    	BGCMD=263,
    	BACKCMD=264,
    	PIPECMD=265,
    	INRE=266,
    	OUTRE=267,
    	CMD=268,
    	EOL=269
    };
    char * inputBuff;
%}

%%
"cd"([ \t]+[^ \t\n]+)* 		{inputBuff=yytext;  return CDCMD;}
"exit"       		{inputBuff=yytext;  return EXIT;}
"history"  		{inputBuff=yytext;  return HISTORY;}
"jobs"      		{inputBuff=yytext;  return JOBS;}
"fg"[ \t]+"%"[0-9]+ {inputBuff=yytext;  return FGCMD;}
"bg"[ \t]+"%"[0-9]+ {inputBuff=yytext;  return BGCMD;}
[^ \t\n]+[ \t]?"&" 				{inputBuff=yytext;  return BACKCMD;}
[^ \t\n]+([ \t]+[^ \t\n]+)?("|"[ \t]?[^ \t\n]+([ \t]+[^ \t\n]+)?)+			{inputBuff=yytext;  return PIPECMD;}
[^ \t\n]+"<"[^\n]+ 			{inputBuff=yytext;  return INRE;}
[^ \t\n]+">"[^\n]+             {inputBuff=yytext;  return OUTRE;}
[^ \t\n]+([ \t]+[^ \t\n]+)?         {inputBuff=yytext;  return CMD;}
[ \t]+             {/*empty*/}
"\n"                {return EOL;}

/*
%%
int main(int argc,char **args){
	int tok;
	inputBuff=(char *)malloc(sizeof(char)*100);
	while(tok=yylex()){
		printf("%d\n%s\n",tok,inputBuff);

	}
	return 0;
}
*/
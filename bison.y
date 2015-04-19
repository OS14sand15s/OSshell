%{
    #define _GNU_SOURCE
    #include <unistd.h>
    #include "global.h"
    int last=0;
    int yylex ();
    void yyerror ();
    void execCd();
    void execExit();
    void execHistory();
    void prompt(void);
    //int  commandDone;
%}
%token CDCMD EXIT HISTORY JOBS FGCMD BGCMD BACKCMD PIPECMD INRE OUTRE CMD EOL

%%

cmd_list: /* empty */
| cmd_list cmd
;
cmd: CDCMD  {last=0;execCd();}
| EXIT  {last=0;execExit();}
| HISTORY  {last=0;execHistory();}
| JOBS {last=0;execJobs();}
| FGCMD  {last=0;execFgCmd();}
| BGCMD  {last=0;execBgCmd();}
| PIPECMD  {last=1;execPipeCmd();}
| CMD   {last=0;execSimpleCmd();}
| INRE   {last=0;execSimpleCmd();}
| OUTRE {last=0;execSimpleCmd();}
| EOL
;



%%



/****************************************************************
                  错误信息执行函数
****************************************************************/
void yyerror()
{
    fputs("你输入的命令不正确，请重新输入！\n", stderr);
}

/****************************************************************
				 打印提示符信息
****************************************************************/
void
prompt(void)
{
    char *wd = get_current_dir_name();
    char * user=getlogin();
    char computer[256];
    gethostname(computer,255);

    fprintf(stderr, "%s@%s:%s$ ",user,computer, wd); //打印提示符信息
    free(wd);
    //free(user);
}

/****************************************************************
                  main主函数
****************************************************************/
int main(int argc, char** argv) {

    init(); //初始化环境
    while(1){

        yyparse();
        //printf("finish a back ground command!\n");

        
    }
    return (EXIT_SUCCESS);
}

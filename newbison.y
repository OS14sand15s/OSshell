%{
    #include "global.h"
    void finishCmd();
    int commandDone;
%}

%token CDCMD EXIT HISTORY JOBS FGCMD BGCMD BACKCMD PIPECMD INRE OUTRE CMD EOL

%%
cd: CMCMD {execCd();finishCmd();}
;

exit: EXIT {execExit();finishCmd();}
;

history:HISTORY {execHistory();finishCmd();}
;

jobs:JOBS {execJobs();finishCmd();}
;

fgCommand:FGCMD {execFgCmd();finishCmd();}
;

bgCommand:BGCMD {execBgCmd();finishCmd();}
;

pipecmd:PIPECMD {execPipeCmd();finishCmd();}
;

inputRedirect:INRE {execInputRedirect();finishCmd();}
;

outputRedirect:OUTRE {execOutputRedirect();finishCmd();}
;

cmd:CMD {execSimpleCmd();finishCmd();}
;

%%

void finishCmd(){
    commandDone=1;
}
/****************************************************************
                  main主函数
****************************************************************/
int main(int argc, char** argv) {
    int i;
    char c;
    char * user;
    char computer[256];

    init(); //初始化环境
    commandDone = 0;
    
    user=getlogin();
    gethostname(computer,255);

    printf("%s@%s:%s$ ",user,computer, get_current_dir_name()); //打印提示符信息

    while(1){
        
        yyparse(); //调用语法分析函数，该函数由yylex()提供当前输入的单词符号

        if(commandDone == 1){ //命令已经执行完成后，添加历史记录信息
            commandDone = 0;
            addHistory(inputBuff);
        }
        printf("%s@%s:%s$ ",user,computer, get_current_dir_name()); //打印提示符信息
    }

    return (EXIT_SUCCESS);
}

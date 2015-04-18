void execExit(){
    exit(0);
}

void execHistory(){
    if(history.end == -1){
        printf("尚未执行任何命令\n");
            return;
    }
    i = history.start;
    do {
        printf("%s\n", history.cmds[i]);
            i = (i + 1)%HISTORY_LEN;
    } while(i != (history.end + 1)%HISTORY_LEN);
}

void execJobs(){
    if(head == NULL){
        printf("尚无任何作业\n");
     } else {
        printf("index\tpid\tstate\t\tcommand\n");
        for(i = 1, now = head; now != NULL; now = now->next, i++){
            printf("%d\t%d\t%s\t\t%s\n", i, now->pid, now->state, now->cmd);
        }
    }
}

void execCd(){
    char temp[MAX_ARGU_LENTH];
    int i=2;
    int k=0;
    if(inputBuff[2]=='\0'){
        chdir("~");
        return ;
    }
    while(inputBuff[i]==' '||inputBuff[i]=='\t')
        i++;
    for(;k<MAXARGU_LENTH&&inputBuff[i]!=' '&&inputBuff[i]!='\t'&&inputBuff[i]!='\0';k++)
        temp[k]=inputBuff[i];
    if(hasWildcard(temp)){
            p=(StrList*)malloc(sizeof(StrList));
            p->link=NULL;
            head=p;
            split(temp);
            completeArg("/",0);
            p=head->link;
            temp=p->str;
    }
    if(chdir(temp) < 0){
        printf("cd; %s 错误的文件名或文件夹名！\n", temp);
    }
}

void execFgCmd(){
    int i=0;
    int pid;
    int start,end;
    for(;i<strlen(inputBuff);i++){
        if(inputBuff[i]=='%'){
            start=i+1;
            break;
        }

    }
    end=strlen(inputBuff);
    pid=str2Pid(inputBuff,start,end);
    if(pid != -1){
       fg_exec(pid);
    }
}

void execBgCmd(){
        int i=0;
    int pid;
    int start,end;
    for(;i<strlen(inputBuff);i++){
        if(inputBuff[i]=='%'){
            start=i+1;
            break;
        }

    }
    end=strlen(inputBuff);
    pid=str2Pid(inputBuff,start,end);
    if(pid != -1){
      bg_exec(pid);
    }
}

void execSimpleCmd(){
    SimpleCmd *cmd = handleSimpleCmdStr(0, strlen(inputBuff));
    int i;
    pid_t pid;

    if(exists(cmd->args[0])){ //命令存在

        if((pid = fork()) < 0){
            perror("fork failed");
            return;
        }

        if(pid == 0){ //子进程
            if(cmd->input != NULL){ //存在输入重定向
                if((pipeIn = open(cmd->input, O_RDONLY, S_IRUSR|S_IWUSR)) == -1){
                    printf("不能打开文件 %s！\n", cmd->input);
                    return;
                }
                if(dup2(pipeIn, 0) == -1){
                    printf("重定向标准输入错误！\n");
                    return;
                }
            }

            if(cmd->output != NULL){ //存在输出重定向
                if((pipeOut = open(cmd->output, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR)) == -1){
                    printf("不能打开文件 %s！\n", cmd->output);
                    return ;
                }
                if(dup2(pipeOut, 1) == -1){
                    printf("重定向标准输出错误！\n");
                    return;
                }
            }
            /*修改控制方式=========================*/
            if(!cmd->isBack){ //若是不是后台运行命令，
          tcsetpgrp(0,pid);//将子进程组设置为前台进程组
            }
            signal(SIGINT,SIG_DFL);//信号que xing 处理
        signal(SIGQUIT,SIG_DFL);
        signal(SIGTSTP,SIG_DFL);
        signal(SIGTTIN,SIG_DFL);
        signal(SIGTTOU,SIG_DFL);
            justArgs(cmd->args[0]);
            if(execv(cmdBuff, cmd->args) < 0){ //执行命令
                printf("execv failed!\n");
                return;
            }
        }
        else{ //父进程
            if(cmd ->isBack){ //后台命令             
          // fgPid = 0; //pid置0，为下一命令做准备
                addJob(pid); //增加新的作业
        // kill(pid, SIGUSR1); //子进程发信号，表示作业已加入
                
                //等待子进程输出
        // signal(SIGUSR1, setGoon);
        // while(goon == 0) ;
        // goon = 0;
            }else{ //非后台命令
          setpgid(pid,pid);      
 // fgPid = pid;
        //           waitpid(pid, NULL, 0);
            }
        }
    }else{ //命令不存在
        printf("找不到命令%s\n", inputBuff);
    }

}
#define _GNU_SOURCE
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <errno.h>
#include <signal.h>
#include <stddef.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/termios.h>
#include "global.h"
#define DEBUG

int goon = 0;

#define S_INPUT 0//标准输入
#define S_OUTPUT 1//标准输出
int ingnore = 0;       //用于设置signal信号量
char *envPath[10], cmdBuff[40];  //外部命令的存放路径及读取外部命令的缓冲空间
History history;                 //历史命令
Job *head = NULL;                //作业头指针
pid_t fgPid;                     //当前前台作业的进程号
typedef struct PipeCmd{
  int isBack;//是否后台运行
  char *input;//输入重定向
  char *output;//输出重定向
  int simpleCmdNum;//管道所包含的简单命令的数目；
  SimpleCmd * temp[101];//管道所包含的简单命令按照输入顺序存放在结构中
  }PipeCmd;
/*******************************************************
                  工具以及辅助方法
********************************************************/
void setGoon(){
    goon = 1;
}
/*判断命令是否存在*/
int exists(char *cmdFile){
    int i = 0;
    if((cmdFile[0] == '/' || cmdFile[0] == '.') && access(cmdFile, F_OK) == 0){ //命令在当前目录
        strcpy(cmdBuff, cmdFile);
        return 1;
    }else{  //查找ysh.conf文件中指定的目录，确定命令是否存在
        while(envPath[i] != NULL){ //查找路径已在初始化时设置在envPath[i]中
            strcpy(cmdBuff, envPath[i]);
            strcat(cmdBuff, cmdFile);

            if(access(cmdBuff, F_OK) == 0){ //命令文件被找到
                return 1;
            }

            i++;
        }
    }

    return 0;
}

/*将字符串转换为整型的Pid*/
int str2Pid(char *str, int start, int end){
    int i, j;
    char chs[20];

    for(i = start, j= 0; i < end; i++, j++){
        if(str[i] < '0' || str[i] > '9'){
            return -1;
        }else{
            chs[j] = str[i];
        }
    }
    chs[j] = '\0';

    return atoi(chs);
}



/*释放环境变量空间*/
void release(){
    int i;
    for(i = 0; strlen(envPath[i]) > 0; i++){
        free(envPath[i]);
    }
}

/*******************************************************
                  信号以及jobs相关
********************************************************/
/*添加新的作业*/
Job* addJob(pid_t pid){
    Job *now = NULL, *last = NULL, *job = (Job*)malloc(sizeof(Job));

	//初始化新的job
    job->pid = pid;
    strcpy(job->cmd, inputBuff);
    strcpy(job->state, RUNNING);
    job->next = NULL;

    if(head == NULL){ //若是第一个job，则设置为头指针
        head = job;
    }else{ //否则，根据pid将新的job插入到链表的合适位置
		now = head;
		while(now != NULL && now->pid < pid){
			last = now;
			now = now->next;
		}
        last->next = job;
        job->next = now;
    }

    return job;
}

/*移除一个作业*/
void rmJob(int sig, siginfo_t *sip, void* noused){
    pid_t pid;
    Job *now = NULL, *last = NULL;

    if(ingnore == 1){
        ingnore = 0;
        return;
    }

    pid = sip->si_pid;

    now = head;
	while(now != NULL && now->pid < pid){
		last = now;
		now = now->next;
	}

    if(now == NULL){ //作业不存在，则不进行处理直接返回
        return;
    }

	//开始移除该作业
    if(now == head){
        head = now->next;
    }else{
        last->next = now->next;
    }

    free(now);
}

/*组合键命令ctrl+z*/
void ctrl_Z(){
    Job *now = NULL;

    if(fgPid == 0){ //前台没有作业则直接返回
        return;
    }

    //SIGCHLD信号产生自ctrl+z
    ingnore = 1;

	now = head;
	while(now != NULL && now->pid != fgPid)
		now = now->next;

    if(now == NULL){ //未找到前台作业，则根据fgPid添加前台作业
        now = addJob(fgPid);
    }

	//修改前台作业的状态及相应的命令格式，并打印提示信息
    strcpy(now->state, STOPPED);
    now->cmd[strlen(now->cmd)] = '&';
    now->cmd[strlen(now->cmd) + 1] = '\0';
    printf("[%d]\t%s\t\t%s\n", now->pid, now->state, now->cmd);

	//发送SIGSTOP信号给正在前台运作的工作，将其停止
    kill(fgPid, SIGSTOP);
    fgPid = 0;
}

/*组合键命令ctrl+c*/
void ctrl_C(){
	Job *now = NULL;
    //前台没有作业直接返回
	if(fgPid == 0){
	    return;
	}
    //SIGNCHLD信号产生自此函数
	ingnore = 1;

	now = head;
	while (now != NULL && now -> pid != fgPid)
		now = now -> next;

	if (now == NULL){
		now = addJob(fgPid);    //没有前台作业则根据fgPid 添加
	}

	strcpy(now -> state, DONE);
	now -> cmd[strlen(now -> cmd)] = '&';
	now -> cmd[strlen(now -> cmd) + 1] = '\0';
	printf("[%d]\t%s\t\t%s\n",now -> pid, now -> state, now -> cmd);

	kill (fgPid, SIGKILL);             //sigstop -> sigkill
	fgPid = 0;

	//需要删除在作业链表中的当前作业
	
	head = now->next;
	
	free(now);
}

/*fg命令*/
void fg_exec(int pid){
    Job *now = NULL;
	int i;

    //SIGCHLD信号产生自此函数
    ingnore = 1;

	//根据pid查找作业
    now = head;
	while(now != NULL && now->pid != pid)
		now = now->next;

    if(now == NULL){ //未找到作业
        printf("pid为7%d 的作业不存在！\n", pid);
        return;
    }

    //记录前台作业的pid，修改对应作业状态
    fgPid = now->pid;
    strcpy(now->state, RUNNING);

    signal(SIGTSTP, ctrl_Z); //设置signal信号，为下一次按下组合键Ctrl+Z做准备
    signal(SIGINT,ctrl_C);
    i = strlen(now->cmd) - 1;
    while(i >= 0 && now->cmd[i] != '&')
		i--;
    now->cmd[i] = '\0';

    printf("%s\n", now->cmd);
    kill(now->pid, SIGCONT); //向对象作业发送SIGCONT信号，使其运行
    waitpid(fgPid, NULL, 0); //父进程等待前台进程的运行
}

/*bg命令*/
void bg_exec(int pid){
    Job *now = NULL;

    //SIGCHLD信号产生自此函数
    ingnore = 1;

	//根据pid查找作业
	now = head;
    while(now != NULL && now->pid != pid)
		now = now->next;

    if(now == NULL){ //未找到作业
        printf("pid为7%d 的作业不存在！\n", pid);
        return;
    }

    strcpy(now->state, RUNNING); //修改对象作业的状态
    printf("[%d]\t%s\t\t%s\n", now->pid, now->state, now->cmd);

    kill(now->pid, SIGCONT); //向对象作业发送SIGCONT信号，使其运行
}

/*******************************************************
                    命令历史记录
********************************************************/
void addHistory(char *cmd){
    if(history.end == -1){ //第一次使用history命令
        history.end = 0;
        strcpy(history.cmds[history.end], cmd);
        return;
	}

    history.end = (history.end + 1)%HISTORY_LEN; //end前移一位
    strcpy(history.cmds[history.end], cmd); //将命令拷贝到end指向的数组中

    if(history.end == history.start){ //end和start指向同一位置
        history.start = (history.start + 1)%HISTORY_LEN; //start前移一位
    }
}

/*******************************************************
                     初始化环境
********************************************************/
/*通过路径文件获取环境路径*/
void getEnvPath(int len, char *buf){
    int i, j, last = 0, pathIndex = 0, temp;
    char path[40];

    for(i = 0, j = 0; i < len; i++){
        if(buf[i] == ':'){ //将以冒号(:)分隔的查找路径分别设置到envPath[]中
            if(path[j-1] != '/'){
                path[j++] = '/';
            }
            path[j] = '\0';
            j = 0;

            temp = strlen(path);
            envPath[pathIndex] = (char*)malloc(sizeof(char) * (temp + 1));
            strcpy(envPath[pathIndex], path);

            pathIndex++;
        }else{
            path[j++] = buf[i];
        }
    }

    envPath[pathIndex] = NULL;
}

/*初始化操作*/
void init(){
    int fd, n, len;
    char c, buf[80];

	//打开查找路径文件ysh.conf
    if((fd = open("ysh.conf", O_RDONLY, 660)) == -1){
        perror("init environment failed\n");
        exit(1);
    }

	//初始化history链表
    history.end = -1;
    history.start = 0;

    len = 0;
	//将路径文件内容依次读入到buf[]中
    while(read(fd, &c, 1) != 0){
        buf[len++] = c;
    }
    buf[len] = '\0';

    //将环境路径存入envPath[]
    getEnvPath(len, buf);

    //注册信号
    struct sigaction action;
    action.sa_sigaction = rmJob;
    sigfillset(&action.sa_mask);
    action.sa_flags = SA_SIGINFO;
    sigaction(SIGCHLD, &action, NULL);
    signal(SIGTSTP, ctrl_Z);
    signal(SIGINT, ctrl_C);
}

/*******************************************************
                      命令解析
********************************************************/

/*******************************************************
                      命令执行
********************************************************/
/*执行外部命令*/



static void
free_simple_cmd(SimpleCmd *cmd)
{
    int i;
    for(i = 0; cmd->args[i] != NULL; i++)
        free(cmd->args[i]);
    free(cmd->args);
    free(cmd->input);
    free(cmd->output);
    free(cmd);
}

int checkWildCard(char * str1,char * name){
    int i,j;
    i=0;
    j=0;
    while(str1[i]!='\0'){
        switch(str1[i]){
        case '?':
            i++;
            j++;
            break;
        case '*':
            while(str1[i]=='*'||str1[i]=='?'){
                if(str1[i]=='?')
                    j++;
                i++;
            }
            while(name[j]!=str1[i]&&name[j]!='\0'){
                j++;
            }
            if(name[j]=='\0'&&str1[i]!='\0')
                return 0;
            break;
        default:
            if(str1[i]!=name[j])
                return 0;
            else{
                i++;
                j++;
                break;
            }
        }
    }
    if(name[j]!='\0')
        return 0;
    return 1;
}



typedef struct string{
    char *str;
    struct string * link;
}StrList;
StrList * heads,*p;
char * strs[50];
int depth;

void completeArg(char * dire,int dep){
    DIR * dir;
    struct dirent *dirp;
    char * temp;
    //char temp[MAX_ARGU_LENGTH];
    int i;

    dir=opendir(dire);
    if(dir==NULL)
        return;
    while((dirp=readdir(dir))!=NULL){
        if(checkWildCard(strs[dep]+1,dirp->d_name)){
            //printf("%s %s \n",strs[dep]+1,dirp->d_name);
            if(dep!=depth){
                temp=(char *)malloc(sizeof(char)*(3+strlen(dire)+strlen(dirp->d_name)));
                temp[0]='\0';
                strcat(temp,dire);
                strcat(temp,dirp->d_name);
                strcat(temp,"/");
                completeArg(temp,dep+1);
            }else{
                //dire+d_name :valid!
                temp=(char *)malloc(sizeof(char)*(3+strlen(dire)+strlen(dirp->d_name)));
                temp[0]='\0';
                strcat(temp,dire);
                strcat(temp,dirp->d_name);
                p->link=(StrList*)malloc(sizeof(StrList));
                p=p->link;
                p->str=temp;
                p->link=NULL;
            }
        }
    }
    closedir(dir);
}
    
void split(char * arg){

    char * current;
    char  * temp;
    int i=0,k=0,j=0;
    if(arg[0]!='/'){
        current=get_current_dir_name();
        strcat(current,"/");
        strcat(current,arg);
    }else{
        current=arg;
    }
    while(current[i]!='\0'){
        
        
        temp=(char *)malloc(sizeof(char)*1000);
        j=1;
        temp[0]='/';
        i++;
        for(;current[i]!='/'&&current[i]!='\0';i++,j++)
            temp[j]=current[i];
        temp[j]='\0';
        strs[k++]=temp;
        //printf("%s %d\n",strs[k-1],k);

    }
    depth=k-1;

}


int hasWildcard(char * s){
    int i=0;
    for(;i<strlen(s);i++){
        if(s[i]=='*'||s[i]=='?')
            return 1;
    }
    return 0;
}
SimpleCmd * handleSimpleCmd(int begin,int end){

    int i, j, k;
    int fileFinished; //记录命令是否解析完毕
    char c, buff[50][200], inputFile[30], outputFile[30], *temp = NULL;
    SimpleCmd *cmd = (SimpleCmd*)malloc(sizeof(SimpleCmd));
    #ifdef DEBUG
    printf("%s!\n",inputBuff);
    #endif
    //默认为非后台命令，输入输出重定向为null
    cmd->isBack = 0;
    cmd->input = cmd->output = NULL;
    
    //初始化相应变量
    for(i = begin; i<50; i++){
        buff[i][0] = '\0';
    }
    inputFile[0] = '\0';
    outputFile[0] = '\0';
    
    i = begin;
    //跳过空格等无用信息
    while(i < end && (inputBuff[i] == ' ' || inputBuff[i] == '\t')){
        i++;
    }
    
    k = 0;
    j = 0;
    fileFinished = 0;
    temp = buff[k]; //以下通过temp指针的移动实现对buff[i]的顺次赋值过程
    while(i < end){
        /*根据命令字符的不同情况进行不同的处理*/
        switch(inputBuff[i]){ 
            case ' ':
            case '\t': //命令名及参数的结束标志
                temp[j] = '\0';
                j = 0;
                if(!fileFinished){
                    k++;
                    temp = buff[k];
                }
                break;

            case '<': //输入重定向标志
                if(j != 0){
            //此判断为防止命令直接挨着<符号导致判断为同一个参数，如果ls<sth
                    temp[j] = '\0';
                    j = 0;
                    if(!fileFinished){
                        k++;
                        temp = buff[k];
                    }
                }
                temp = inputFile;
                fileFinished = 1;
                i++;
                break;
                
            case '>': //输出重定向标志
                if(j != 0){
                    temp[j] = '\0';
                    j = 0;
                    if(!fileFinished){
                        k++;
                        temp = buff[k];
                    }
                }
                temp = outputFile;
                fileFinished = 1;
                i++;
                break;
                
            case '&': //后台运行标志
                if(j != 0){
                    temp[j] = '\0';
                    j = 0;
                    //printf("temp:%s\n",temp);
                    if(!fileFinished){
                        k++;
                        temp = buff[k];
                    }
                }
                cmd->isBack = 1;
                fileFinished = 1;
                i++;
                break;
                
            default: //默认则读入到temp指定的空间
                temp[j++] = inputBuff[i++];
                continue;
        }
        
        //跳过空格等无用信息
        while(i < end && (inputBuff[i] == ' ' || inputBuff[i] == '\t')){
            i++;
        }
    }
    
    if(inputBuff[end-1] != ' ' && inputBuff[end-1] != '\t' ){
        temp[j] = '\0';
        if(!fileFinished){
            k++;
        }
    }
//printf("k%d\n",k);
    //依次为命令名及其各个参数赋值
    cmd->args = (char**)malloc(sizeof(char*) * (MAX_ARGU_NUM));
    j=0;
    for(i = 0; i<k; i++){
        
        if(hasWildcard(buff[i])){
            p=(StrList*)malloc(sizeof(StrList));
            p->link=NULL;
            heads=p;
            split(buff[i]);
            completeArg("/",0);
            p=heads->link;
            while(p!=NULL){
                cmd->args[j]=(char *)malloc(sizeof(char)*(1+strlen(p->str)));
                strcpy(cmd->args[j++], p->str);
                p=p->link;
            }
           
        }else{
            cmd->args[j] = (char*)malloc(sizeof(char) * (strlen(buff[i]) + 1));
            strcpy(cmd->args[j++], buff[i]);
        }
        //printf("%s\n",cmd->args[i]);
    }

   cmd->args[j]=NULL;
    //如果有输入重定向文件，则为命令的输入重定向变量赋值
    if(strlen(inputFile) != 0){
        j = strlen(inputFile);
        cmd->input = (char*)malloc(sizeof(char) * (j + 1));
        strcpy(cmd->input, inputFile);
    }

    //如果有输出重定向文件，则为命令的输出重定向变量赋值
    if(strlen(outputFile) != 0){
        j = strlen(outputFile);
        cmd->output = (char*)malloc(sizeof(char) * (j + 1));   
        strcpy(cmd->output, outputFile);
    }
    #ifdef DEBUG
    printf("****\n");
    printf("isBack: %d\n",cmd->isBack);
        for(i = 0; i<j; i++){
            printf("args[%d]: %s\n",i,cmd->args[i]);
    }
    printf("input: %s\n",cmd->input);
    printf("output: %s\n",cmd->output);
    printf("****\n");
    #endif

    return cmd;
}


PipeCmd* handlePipeCmdStr(int begin, int end)
{
    int i, j, k;
    int fileFinished; //记录命令是否解析完毕
    char c, buff[10][40], inputFile[30], outputFile[30], *temp = NULL;
    PipeCmd *cmd = malloc(sizeof *cmd);

	//默认为非后台命令，输入输出重定向为null，如果有管道命令 指令数默认值为1
    cmd->isBack = 0;
    cmd->input = cmd->output = NULL;
	cmd->simpleCmdNum=1;
	fileFinished=0;

    //初始化相应变量
    for(i = begin; i<10; i++)
	{
        buff[i][0] = '\0';
    }
    inputFile[0] = '\0';
    outputFile[0] = '\0';
	
    i = begin;
	//跳过空格等无用信息
    while(i < end && (inputBuff[i] == ' ' || inputBuff[i] == '\t'))
	{
        i++;
    }

    k = 0;
    j = 0;
    fileFinished = 0;
    temp = buff[k]; //以下通过temp指针的移动实现对buff[i]的顺次赋值过程
    while(i < end)
	{
		/*根据命令字符的不同情况进行不同的处理*/
        switch(inputBuff[i])
		{
            case ' ':
           case '\t': //命令名及参数的结束标志
                temp[j] = '\0';
                j = 0;
                if(!fileFinished)//没完成
				{
                   // k++;
                    temp = buff[k];
                }
                break;

            case '<': //输入重定向标志
                if(j != 0)
				{
		    //此判断为防止命令直接挨着<符号导致判断为同一个参数，如果ls<sth
                    temp[j] = '\0';
                    j = 0;
                    if(!fileFinished)
					{
                       // k++;
                        temp = buff[k];
                    }
                }
                temp = inputFile;
              //  fileFinished = 1;
                i++;
                break;

            case '>': //输出重定向标志
                if(j != 0)
				{
                    temp[j] = '\0';
                    j = 0;
                    if(!fileFinished)
					{
                       // k++;
                        temp = buff[k];
                    }
                }
                temp = outputFile;
                //fileFinished = 1;
                i++;
              break;
			  
            case '&': //后台运行标志
                if(j != 0){
                    temp[j] = '\0';
                    j = 0;
                    if(!fileFinished)
					{
                        k++;
                        temp = buff[k];
                    }
                }
                cmd->isBack = 1;
                //fileFinished = 1;
                i++;
                break;
	
			case'|'://管道标志
				if(j != 0){
                    temp[j] = '\0';
                    j = 0;
                    if(!fileFinished)
					{
                        k++;
                        temp = buff[k];
                    }
                }
				cmd->simpleCmdNum++;
				
                //fileFinished = 1;
				i++;
                break;


            default: //默认则读入到temp指定的空间
                temp[j++] = inputBuff[i++];
                continue;
		}

		//跳过空格等无用信息
        while(i < end && (inputBuff[i] == ' ' || inputBuff[i] == '\t'))
		{
            i++;
        }
	}
	fileFinished=1;

	
    if(inputBuff[end-1] != ' ' && inputBuff[end-1] != '\t' && inputBuff[end-1] != '&')
	{
        temp[j] = '\0';
        if(!fileFinished)
		{
            k++;
        }
    }

	//如果有输入重定向文件，则为命令的输入重定向变量赋值
    if(strlen(inputFile) != 0)
	{
        j = strlen(inputFile);
        cmd->input = (char*)malloc(sizeof(char) * (j + 1));
        strcpy(cmd->input, inputFile);
    }
	

    //如果有输出重定向文件，则为命令的输出重定向变量赋值
    if(strlen(outputFile) != 0){
        j = strlen(outputFile);
        cmd->output = (char*)malloc(sizeof(char) * (j + 1));
        strcpy(cmd->output, outputFile);
    }
	
	
	for(k=0;k<cmd->simpleCmdNum;k++)
	{
		cmd->temp[k]= handleSimpleCmd(0, strlen(buff[k]));
		if(k>0&&k<cmd->simpleCmdNum)
			cmd->temp[k]->input=cmd->temp[k-1]->output;
	}


    #ifdef DEBUG
    printf("****\n");
    printf("isBack: %d\n",cmd->isBack);
    	for(i = 0; cmd->temp[i] != NULL; i++)
		{
    		printf("cmds[%d]: %s\n",i,cmd->temp[i]->args[0]);
		}
    printf("input: %s\n",cmd->input);
    printf("output: %s\n",cmd->output);
    printf("****\n");
    #endif
    return cmd;
}
/*******************************************************
                     命令执行接口
********************************************************/
void execExit(){
    exit(0);
}

void execHistory(){
    int i;
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
    int i;
    Job *now = NULL;
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
    char temp[MAX_ARGU_LENGTH];
    int i=2;
    int k=0;
    if(inputBuff[2]=='\0'){
        chdir("/home");
        return ;
    }
    while(inputBuff[i]==' '||inputBuff[i]=='\t')
        i++;
    for(;k<MAX_ARGU_LENGTH&&inputBuff[i]!=' '&&inputBuff[i]!='\t'&&inputBuff[i]!='\0';k++,i++)
        temp[k]=inputBuff[i];
    temp[k]='\0';
    if(hasWildcard(temp)){
            p=(StrList*)malloc(sizeof(StrList));
            p->link=NULL;
            heads=p;
            split(temp);
            completeArg("/",0);
            p=heads->link;
            if(chdir(p->str)<0)
                printf("cd; %s 错误的文件名或文件夹名！\n", p->str);
    }
    else if(chdir(temp) < 0){
        printf("cd; %s 错误的文件名或文件夹名！\n", temp);
    }
    printf("%s|\n",temp);
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
    SimpleCmd *cmd = handleSimpleCmd(0, strlen(inputBuff));
    int i;
    pid_t pid;
    int pipeIn, pipeOut;
    
    if(exists(cmd->args[0])){ //命令存在

        if((pid = fork()) < 0){
            perror("fork failed");
            return;
        }

        if(pid == 0){ //子进程
            if(cmd->isBack){ //若是后台运行命令，等待父进程增加作业
                //printf("& !!\n");
                signal(SIGUSR1, setGoon); //收到信号，setGoon函数将goon置1，以跳出下面的循环
                while(goon == 0) ; //等待父进程SIGUSR1信号，表示作业已加到链表中
                goon = 0; //置0，为下一命令做准备
                
                printf("[%d]\t%s\t\t%s\n", getpid(), RUNNING, inputBuff);
                kill(getppid(), SIGUSR1);
            }

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
    signal(SIGTSTP, ctrl_Z); //设置signal信号，为下一次按下组合键Ctrl+Z做准备
    signal(SIGINT,ctrl_C);
            if(execv(cmdBuff, cmd->args) < 0){ //执行命令
                printf("execv failed!\n");
                return;
            }

        }
        else{ //父进程
            if(cmd ->isBack){ //后台命令            
            
                fgPid = 0; //pid置0，为下一命令做准备
                addJob(pid); //增加新的作业
                sleep(2);
                kill(pid, SIGUSR1); //子进程发信号，表示作业已加入
                
                //等待子进程输出
                signal(SIGUSR1, setGoon);
                while(goon == 0) ;//printf("fu jin cheng xun huan ing \n");
                goon =0;
            }else{ //非后台命令
                fgPid = pid;
                waitpid(pid, NULL, 0);
            }
        }
    }else{ //命令不存在
        printf("找不到命令%s\n", inputBuff);
    }

}
void execOuterPipeCmd(SimpleCmd *cmd){
    pid_t pid;
    int pipeIn, pipeOut;
    if(exists(cmd->args[0])){ //命令存在
      if(execv(cmdBuff, cmd->args) < 0){ //执行命令
                printf("execv failed!\n");
                return;
            }
        }
    else
      { //命令不存在
        printf("找不到命令 %15s\n", inputBuff);
      }
}

/*执行命令*/
void execPPCmd(SimpleCmd *cmd){
    int i, pid;
    char *temp;
    Job *now = NULL;
    
    if(strcmp(cmd->args[0], "exit") == 0) { //exit命令
        exit(0);
    } else if (strcmp(cmd->args[0], "history") == 0) { //history命令
        if(history.end == -1){
            printf("尚未执行任何命令\n");
            return;
        }
        i = history.start;
        do {
            printf("%s\n", history.cmds[i]);
            i = (i + 1)%HISTORY_LEN;
        } while(i != (history.end + 1)%HISTORY_LEN);
    } else if (strcmp(cmd->args[0], "jobs") == 0) { //jobs命令
        if(head == NULL){
            printf("尚无任何作业\n");
        } else {
            printf("index\tpid\tstate\t\tcommand\n");
            for(i = 1, now = head; now != NULL; now = now->next, i++){
                printf("%d\t%d\t%s\t\t%s\n", i, now->pid, now->state, now->cmd);
            }
        }
    } else if (strcmp(cmd->args[0], "cd") == 0) { //cd命令
        temp = cmd->args[1];
        if(temp != NULL){
            if(chdir(temp) < 0){
                printf("cd; %s 错误的文件名或文件夹名！\n", temp);
            }
        }
    } else if (strcmp(cmd->args[0], "fg") == 0) { //fg命令
        temp = cmd->args[1];
        if(temp != NULL && temp[0] == '%'){
            pid = str2Pid(temp, 1, strlen(temp));
            if(pid != -1){
                fg_exec(pid);
            }
        }else{
            printf("fg; 参数不合法，正确格式为：fg %<int>\n");
        }
    } else if (strcmp(cmd->args[0], "bg") == 0) { //bg命令
        temp = cmd->args[1];
        if(temp != NULL && temp[0] == '%'){
            pid = str2Pid(temp, 1, strlen(temp));
            
            if(pid != -1){
                bg_exec(pid);
            }
        }
		else{
            printf("bg; 参数不合法，正确格式为：bg %<int>\n");
        }
    } else{ //外部命令
        execOuterPipeCmd(cmd);
    }
    
    //释放结构体空间
    for(i = 0; cmd->args[i] != NULL; i++){
        free(cmd->args[i]);
        free(cmd->input);
        free(cmd->output);
    }
}
/*PipeCmd* handlePipeCmdStr(int begin,int end)崔崔的工作*/
/*******************************************************
                     命令执行接口
********************************************************/

/*
***********************************************8]
*/
void exePiPCmd(PipeCmd *pipecmd){
  int pipeIn,pipeOut,pipenum,i,j,singlecmdnum=pipecmd->simpleCmdNum;
  pid_t pid[singlecmdnum],shell_pid,Gpid;
  int pipes[singlecmdnum][2];//管道们
  shell_pid=getpid();
  for(i=0;i<singlecmdnum;i++)
    {
      if(pipe(pipes[i])<0){
	printf("Can not creat a pipe");
      }
      if(pid[i]=fork()<0)
	{
	  printf("Can not fork()in pipe");
	}
      else
	{
	  /*创建成功*/
	  if(!pid[i])//child process
	    {
	      if(i==0)//第一个子进程
		{
		  Gpid=getpid();
		  setpgid(Gpid,Gpid);//新建一个进程，把管道的第一个进程作为组长。
		  if(!pipecmd->isBack)
		    {
		      tcsetpgrp(0,Gpid);
		    }//把他设置为当前进程组
		   signal(SIGINT,SIG_DFL);//信号que xing 处理
		   signal(SIGQUIT,SIG_DFL);
		   signal(SIGTSTP,SIG_DFL);
		   signal(SIGTTIN,SIG_DFL);
		   signal(SIGTTOU,SIG_DFL);
		  if(pipecmd->input!=NULL)//存在输入重定向
		    {
		      if((pipeIn=open(pipecmd->input, O_RDONLY, S_IRUSR|S_IWUSR)) == -1)
			{
			  printf("can not open file%s!\n",pipecmd->input);
			}
		      if(dup2(pipeIn,S_INPUT)==-1)
			{
			  printf("dup2 failed!");
			}
		    }
		  close(pipes[i][0]);//关闭第一个管道的管道读；
		  if(dup2(pipes[i][1],S_OUTPUT)<0)
		    {
		      printf("dup2 failed!");
		    }
		  execPPCmd(pipecmd->temp[i]);
		}
	      else if(i==singlecmdnum-1)//这是最后一个进程
		{
		  setpgid(pid[i],Gpid);
		  if(pipecmd->output!=NULL)
		    {
		      if((pipeOut=open(pipecmd->output, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR)) == -1)
			{
			  printf("open failed!\n");
			}
		      if(dup2(pipeOut,S_OUTPUT)==-1)
			{
			  printf("dup2 failed!\n");
			}
		    }
		  close(pipes[i-1][1]);//关掉管道写
		  if(dup2(pipes[i-1][0],S_INPUT)<0)
		    {
		      printf("dup2 failed!\n");
		    }
		  for(j=0;j<i-1;j++)
		    {
		      close(pipes[j][0]);
		      close(pipes[j][1]);
		    }
		  close(pipes[i][0]);
		  close(pipes[i][1]);//其实最后一个管道本来不需要建立
		  execPPCmd(pipecmd->temp[i]);
		}
	      else//中间进程
		{
		  setpgid(pid[i],Gpid);
		  close(pipes[i-1][1]);//关掉前一个管道写；
		  close(pipes[i][0]);//关掉当前管道读；
		  if(dup2(pipes[i-1][0],S_INPUT)==-1)//输入重定向
		    {
		      printf("dup2 failed!\n");
		    }
		  if(dup2(pipes[i][1],S_OUTPUT)==-1)//输出重定向
		    {
		      printf("dup2 failed!\n");
		    }
		  for(j=0;j<i-1;i++)
		    {
		      close(pipes[j][0]);
		      close(pipes[j][1]);
		    }//关闭不用的管道。
		  execPPCmd(pipecmd->temp[i]);
		}
	    }
	  else//父进程
	    {
	      if(pipecmd->isBack)
		addJob(pid[0]);
	      else
		{
		setpgid(pid[0],pid[0]);//设置成当前进程组
		tcsetpgrp(0, pid[0]);
		}
	    }
	}
    }
}

void execPipeCmd(){
    PipeCmd *pipecmd = handlePipeCmdStr(0, strlen(inputBuff));
    exePiPCmd(pipecmd);
}

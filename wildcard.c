/**
* wildcard function.
* @author zhangtianyu
**/
#define  _GNU_SOURCE
#include <stdio.h>  
#include <stdlib.h>
#include <dirent.h> 
#include <string.h>
#include <unistd.h>
#include "global.h"


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
StrList * head,*p;
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
    
    char c, buff[MAX_ARGU_NUM][MAX_ARGU_LENGTH], inputFile[MAX_ARGU_LENGTH], outputFile[MAX_ARGU_LENGTH], *temp = NULL;
    SimpleCmd *cmd = malloc(sizeof *cmd);

    //默认为非后台命令，输入输出重定向为null
    cmd->isBack = 0;
    cmd->input = cmd->output = NULL;

    //初始化相应变量
    for(i = begin; i<MAX_ARGU_NUM; i++){
        buff[i][0] = '\0';
    }
    inputFile[0] = '\0';
    outputFile[0] = '\0';

    i=begin;

    k = 0;
    j = 0;
   
    temp = buff[k]; //以下通过temp指针的移动实现对buff[i]的顺次赋值过程
    while(i < end){
        /*根据命令字符的不同情况进行不同的处理*/
        switch(inputBuff[i]){
            case ' ':
            case '\t': //命令名及参数的结束标志
                temp[j] = '\0';
                j = 0;
              
                    k++;
                    temp = buff[k];
                
                break;

            case '<': //输入重定向标志
                if(j != 0){
            //此判断为防止命令直接挨着<符号导致判断为同一个参数，如果ls<sth
                    temp[j] = '\0';
                    j = 0;
                   
                        k++;
                        temp = buff[k];
                    
                }
                if(hasWildcard(temp)){
                     p=(StrList*)malloc(sizeof(StrList));
                    p->link=NULL;
                   head=p;
                    split(temp);
                    completeArg("/",0);
                    p=head->link;
                     temp=p->str;
                }
                temp = inputFile;
                
                i++;
                break;

            case '>': //输出重定向标志
                if(j != 0){
                    temp[j] = '\0';
                    j = 0;
                    
                        k++;
                        temp = buff[k];
                    
                }
                if(hasWildcard(temp)){
                     p=(StrList*)malloc(sizeof(StrList));
                    p->link=NULL;
                   head=p;
                    split(temp);
                    completeArg("/",0);
                    p=head->link;
                     temp=p->str;
                }
                temp = outputFile;
                
                i++;
                break;
            case '&':
            cmd->isBack=1;
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

    if(inputBuff[end-1] != ' ' && inputBuff[end-1] != '\t'){
        temp[j] = '\0';
        if(!fileFinished){
            k++;
        }
    }

    //依次为命令名及其各个参数赋值
    cmd->args = (char**)malloc(sizeof(char*) * (MAX_ARGU_NUM));
    
    for(i = 0; i<k; i++){
        
        if(hasWildcard(buff[i])){
            p=(StrList*)malloc(sizeof(StrList));
            p->link=NULL;
            head=p;
            split(buff[i]);
            completeArg("/",0);
            p=head->link;
            while(p!=NULL){
                cmd->args[i]=(char *)malloc(sizeof(char)*(1+strlen(p->str)));
                strcpy(cmd->args[i++], p->str);
                p=p->link;
            }
            i--;
        }else{
            cmd->args[i] = (char*)malloc(sizeof(char) * (j + 1));
            strcpy(cmd->args[i], buff[i]);
        }
    }


    #ifdef DEBUG
    printf("****\n");
    printf("isBack: %d\n",cmd->isBack);
        for(i = 0; cmd->args[i] != NULL; i++){
            printf("args[%d]: %s\n",i,cmd->args[i]);
    }
    printf("input: %s\n",cmd->input);
    printf("output: %s\n",cmd->output);
    printf("****\n");
    #endif

    return cmd;
}
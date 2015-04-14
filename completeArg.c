/**
* wildcard function.
* if the argument is completed successfully,return the argument
* else return NULL.
* @author zhangtianyu
**/
#include <stdio.h>  
#include <stdlib.h>
#include <dirent.h> 
#include <string.h>
void handleCommand(){
	//deal inputBuff

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
    return 1;
}
char * completeArg(char * arg){
	DIR *dir;
    struct dirent *dirp;
    char * result;
    int i=1;
    int length=1;
    char space[2];
    result=(char *)malloc(sizeof(char));
    space[0]=' ';
    space[1]='\0';

	if((dir=opendir(get_current_dir_name()))==NULL)
		return NULL;

    while((dirp=readdir(dir))!=NULL){
    	if(checkWildCard(arg,dirp->d_name)){
            length+=strlen(dirp->d_name)+1;
            result=(char *)realloc(result,sizeof(char)*length);
            strcat(result,dirp->d_name);
            strcat(result,space);
    	}
    }
    result[strlen(result)-1]='\0';
    closedir(dir);
    return result;
}

int main(){
	printf("%s\n",completeArg("n*b*.*?*"));
}
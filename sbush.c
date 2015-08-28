#include<stdio.h>
#include<stdlib.h>
void execute(char *cmd);
int
main(int argc,char* argv[]){
	char *cmd;
	cmd =(char*) malloc( 100 * sizeof(char));
	printf("sbush$ ");
	gets(cmd);
	while(*cmd){
		execute(cmd);
		printf("sbush$ ");
		gets(cmd);
	}
	return 0;
}

void execute(char *cmd){
	printf("%s\n",cmd);
}

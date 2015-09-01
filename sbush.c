#include<stdio.h>
#include<stdlib.h>
#include<stdarg.h>
#include<unistd.h>
#include<string.h>

#define MAX_CMD_SIZE 256 // max sbush cmd size
#define MAX_ARG_NUM 8    // max argument number in a sbush cmd
#define MAX_SH_VAR_NUM 2 // max shell variable number
#define MAX_ENV_VAR_NUM 256 // max environment variable number
#define MAX_VAR_SIZE 128 // max shell variable size
#define MAX_DIR_SIZE 128 // max directory/path size

#define DEBUG

char** parse_cmds(char* cmd); // parse user cmds into a list of strings
char* get_var(char** var_list, char* var_name); // get variable value by its name in var_list
void set_var(char** var_list, char* var_name, char* var_value); // set variable value in var_list
void set_path(char** var_list, char* path_value); // set path value, e.g. PATH=$PATH:/usr/bin
char* parse_PS1(char* ps1); // parse PS1 symbols into readable string
int min(int a, int b);
int max(int a, int b);
void execute(char** arg_list);

#if defined(DEBUG) // code borrowed from kernel code piece with little modification
static void debug_printf(char *fmt, ...)
{
        char buffer[128];
        va_list ap;

        va_start(ap, fmt);
        vsprintf(buffer, fmt, ap);
        va_end(ap);

        printf("%s",buffer);
}
#else
#define debug_printf(x...) do { } while (0)
#endif

int main(int argc, char* argv[], char* envp[]){
//	int i;
	char *cmd, *tmp, *prompt;
	char** arg_list;
	char** SH_VAR_LIST;
	
	tmp = (char*) malloc(MAX_DIR_SIZE * sizeof(char));
	tmp = getcwd(NULL, MAX_DIR_SIZE);
	debug_printf("szc:main:cwd:tmp is %s\n",tmp);

	SH_VAR_LIST = (char**) malloc(MAX_SH_VAR_NUM * sizeof(char*));
	tmp = get_var(envp,"PATH=");
	debug_printf("szc:main:envp:path is %s\n",tmp);
	set_var(SH_VAR_LIST, "PATH=", "/abc:/hjx");
	tmp = get_var(SH_VAR_LIST,"PATH=");
	debug_printf("szc:main:sh:path is %s\n",tmp);
	set_path(SH_VAR_LIST,"/usr/szc1:$PATH");
	debug_printf("szc:main:after set_path\n");
	tmp = get_var(SH_VAR_LIST,"PATH=");
	debug_printf("szc:main:sh:path after set_path1 is %s\n",tmp);
	set_path(SH_VAR_LIST,"/usr/szc1:$PATH:/usr/szc2");
	tmp = get_var(SH_VAR_LIST,"PATH=");
	debug_printf("szc:main:sh:path after set_path2 is %s\n",tmp);

/*	
	i = 0;
	while(envp[i]){
		printf("%s\n", envp[i]);
		i += 1;
	}	
*/
	cmd =(char*) malloc(MAX_CMD_SIZE * sizeof(char));
	while(1){
		printf("sbush$ ");
		//scanf("%[^\n]%*c", cmd);
		fgets(cmd, MAX_CMD_SIZE, stdin);
		fflush(stdin);
		arg_list = parse_cmds(cmd);
		if(!arg_list)
			continue;
		execute(arg_list);
	}
	return 0;
}

char* get_var(char** var_list, char* var_name){//cut var_value if length > MAX_VAR_SIZE
	char *var_value;
	int i, j, nlen, vlen;

	var_value = (char*) malloc(MAX_VAR_SIZE * sizeof(char));

	i = 0;
	nlen = min(strlen(var_name), MAX_VAR_SIZE);
	while(i < MAX_ENV_VAR_NUM && var_list[i]!=NULL && strncmp(var_name, var_list[i], nlen)!=0)
		i++;

	if (var_list[i] != NULL && 0 == strncmp(var_name, var_list[i], nlen)){
		debug_printf("get_var:i = %d, var_list[i] = %s\n",i, var_list[i]);
		vlen = min(strlen(var_list[i]) - nlen, MAX_VAR_SIZE - 1);
		debug_printf("get_var:nlen = %d, vlen= %d var_list[i] = %s\n",nlen, vlen, var_list[i]);
		//strncpy(var_value, var_list[i] + nlen, vlen); 
		for(j = 0; j < vlen; j++){
			var_value[j] = var_list[i][nlen+j];
		}
		debug_printf("get_var:after strncpy: var_value = %s\n",var_value);
		var_value[vlen] = '\0';
		return var_value;
	}

	free(var_value);
	return NULL;
}

void set_path(char** var_list, char* new_path){
	char *old_path;
	char *new_path_entry;
	int i, j, len;

	new_path_entry = (char*) malloc(MAX_VAR_SIZE * sizeof(char));

	i = 0; j = 0;
	old_path = get_var(var_list, "PATH=");
	while(new_path[i] != '\0' && i < MAX_VAR_SIZE && j < MAX_VAR_SIZE){
		if(new_path[i] != '$')
			new_path_entry[j++] = new_path[i++];
		else if(new_path[i+1] == 'P' &&
			   new_path[i+2] == 'A' &&
			   new_path[i+3] == 'T' &&
			   new_path[i+4] == 'H') {
				if(old_path != NULL){
					len = strlen(old_path);
					strncpy(new_path_entry + j, old_path, len);
					debug_printf("set_path:len = %d, old_path =%s\n",len,old_path);	
					j += len;
					i += 5;
					continue;	
				}else{
					i += 5;
					continue;
				}
			}
		else{
			printf("error in set_path: unaccepted PATH format!\n");
			free(new_path_entry);
			return;
		}
	}

	if( j < MAX_VAR_SIZE && i < MAX_VAR_SIZE){
		new_path_entry[j] = '\0';
		set_var(var_list, "PATH=", new_path_entry);
	}else{
		printf("error in set_path: PATH value too long!\n");
		free(new_path_entry);
	}
	
	return;
}

void set_var(char** var_list, char* var_name, char* var_value){
	char *var_entry;
	int i, len, vlen;

	// copy variable name to var_entry
	var_entry = (char*) malloc(MAX_VAR_SIZE * sizeof(char));
	len = min(strlen(var_name), MAX_VAR_SIZE - 1);
	strncpy(var_entry, var_name, len);

	// copy variable value to var_entry
	vlen = min(strlen(var_value),MAX_VAR_SIZE - len - 1);
	strncpy(var_entry + len, var_value, vlen);
	var_entry[len+vlen] = '\0';

	// find existing variable
	i = 0;
	while(i < MAX_SH_VAR_NUM && var_list[i]!=NULL && strncmp(var_name, var_list[i], len)!=0)
		i++;

	if (MAX_SH_VAR_NUM == i){
		free(var_entry);
		printf("error in set_var: no more room for new shell variables!");
		return;
	}

	var_list[i] = var_entry; // no matter it exists or not i points to a usable location,just fill in it!
	return;
}

char* parse_PS1(char* ps1){
	char *ps1_value;
	ps1_value = (char*) malloc(MAX_VAR_SIZE * sizeof(char));
	//TODO
	return ps1_value;
}
char** parse_cmds(char* cmd){
	char** arg_list;//TODO:assume initialized to NULLs
	char *p, *tp, *tmp;
	int i, j;
	
	arg_list = (char**) malloc(MAX_ARG_NUM * sizeof(char*));	

	j = 0;// arg number, < MAX_ARG_NUM
	p = cmd;
	while(*p && *p != '\n' && j<MAX_ARG_NUM){
		while(*p == ' ')
			p++;// skip spaces

		i = 0;
		tmp = (char*) malloc(MAX_CMD_SIZE * sizeof(char));//TODO:suppose malloc always success
		while(*p != '\n' && *p != ' ' && i < MAX_CMD_SIZE - 1)
			tmp[i++] = *p++;
		
		//take one arg
		if(i > 0 && i < MAX_CMD_SIZE){
			tmp[i] = '\0';
			arg_list[j++] = tmp;
		}else if(i >= MAX_CMD_SIZE){
			printf("error: argument too long!\n");
			return NULL;
		}else if(*p == '\n')
			break;		
		
		debug_printf("szc:parse_cmds:tmp is %s , j = %d\n", tmp, j);
	}
	
	if(j >= MAX_ARG_NUM){
			printf("error: too many arguments!\n");
			return NULL;
	}

	return arg_list;
}

void execute(char** arg_list){
	while(*arg_list)
		printf("%s\n",*arg_list++);
}

int min(int a, int b){
	return a>b ? b:a;
}

int max(int a, int b){
	return a<b ? b:a;
}

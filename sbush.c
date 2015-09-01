#include<stdio.h>
#include<stdlib.h>
#include<stdarg.h>
#include<unistd.h>
#include<string.h>

#define MAX_CMD_SIZE 256 // max sbush cmd size
#define MAX_ARG_NUM 8    // max argument number in a sbush cmd
#define MAX_SH_VAR_NUM 2 // max shell variable number
#define MAX_ENV_VAR_NUM 256 // max environment variable number
#define MAX_VAR_SIZE 256 // max shell variable size
#define MAX_DIR_SIZE 256 // max directory/path size
#define MAX_BUFFER_SIZE 256 // hostname size

#define DEBUG

char** parse_cmds(char* cmd, int* arg_size); // parse user cmds into a list of strings
char* get_var(char** var_list, char* var_name); // get variable value by its name in var_list
void set_var(char** var_list, char* var_name, char* var_value); // set variable value in var_list
void set_path(char** var_list, char* path_value); // set path value, e.g. PATH=$PATH:/usr/bin
char* parse_PS1(char* ps1); // parse PS1 symbols into readable string
int min(int a, int b);
int max(int a, int b);
void execute(char** arg_list, int arg_size, char** sh_var, char** prompt);

#if defined(DEBUG) // code borrowed from kernel code piece with little modification
static void debug_printf(char *fmt, ...)
{
        char buffer[512];
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
	int arg_size;
	
	tmp = (char*) malloc(MAX_DIR_SIZE * sizeof(char));
	tmp = getcwd(NULL, MAX_DIR_SIZE);
	debug_printf("szc:main:cwd:tmp is %s\n",tmp);

	SH_VAR_LIST = (char**) malloc(MAX_SH_VAR_NUM * sizeof(char*));
	tmp = get_var(envp,"PATH=");
	debug_printf("szc:main:envp:path is %s\n",tmp);
	set_var(SH_VAR_LIST, "PATH=", tmp);
	tmp = get_var(SH_VAR_LIST,"PATH=");
	debug_printf("szc:main:sh:path is %s\n",tmp);
	set_path(SH_VAR_LIST,tmp);
	debug_printf("szc:main:after set_path\n");
	tmp = get_var(SH_VAR_LIST,"PATH=");
	debug_printf("szc:main:sh:path after set_path1 is %s\n",tmp);
	set_path(SH_VAR_LIST,"/usr/szc1:$PATH:/usr/szc2");
	tmp = get_var(SH_VAR_LIST,"PATH=");
	debug_printf("szc:main:sh:path after set_path2 is %s\n",tmp);
	set_var(SH_VAR_LIST, "PS1=", "\\u@\\h:\\w\\$ ");
	tmp = get_var(SH_VAR_LIST,"PS1=");
	debug_printf("szc:main:sh:ps1 after set ps1 is %s\n",tmp);
	prompt = parse_PS1(tmp);
	debug_printf("szc:main:sh:ps1 value after parse ps1 is %s\n",tmp);


/*	
	i = 0;
	while(envp[i]){
		printf("%s\n", envp[i]);
		i += 1;
	}	
*/
	cmd =(char*) malloc(MAX_CMD_SIZE * sizeof(char));
	while(1){
		printf("%s",prompt);
		//scanf("%[^\n]%*c", cmd);
		fgets(cmd, MAX_CMD_SIZE, stdin);
		fflush(stdin);
		arg_list = parse_cmds(cmd, &arg_size);
		if(!arg_list)
			continue;
		execute(arg_list, arg_size, SH_VAR_LIST, &prompt);
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
		//debug_printf("get_var:i = %d, var_list[i] = %s\n",i, var_list[i]);
		vlen = min(strlen(var_list[i]) - nlen, MAX_VAR_SIZE - 1);
		//debug_printf("get_var:nlen = %d, vlen= %d var_list[i] = %s\n",nlen, vlen, var_list[i]);
		//strncpy(var_value, var_list[i] + nlen, vlen); 
		for(j = 0; j < vlen; j++){
			var_value[j] = var_list[i][nlen+j];
		}
		//debug_printf("get_var:after strncpy: var_value = %s\n",var_value);
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
		else if(   new_path[i+1] == 'P' &&
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
	/** here we provide a very limitted PS1 settings, we only support
	\h: hostname
	\u: user name
	\w: working directory
	\$: the prompt
	*/
	char *ps1_value, *tmp, *username;
	int i, j, len;

	ps1_value = (char*) malloc(MAX_VAR_SIZE * sizeof(char));
	tmp = (char*) malloc(MAX_BUFFER_SIZE * sizeof(char));
	tmp[MAX_BUFFER_SIZE - 1] = '\0';

	i = 0; j = 0;
	while(ps1[i]!='\0' && i < MAX_VAR_SIZE && j < MAX_VAR_SIZE){
		if(ps1[i] != '\\'){
			ps1_value[j++] = ps1[i++];
		}else{
			switch(ps1[i + 1]){
				case 'h':
					gethostname(tmp, MAX_BUFFER_SIZE - 1);
					break;
				case 'u':
					username=getenv("USER");
					if(username != NULL)
						strncpy(tmp,username,strlen(username));
					else
						tmp = NULL;
					break;
				case 'w':
					getcwd(tmp, MAX_BUFFER_SIZE - 1);
					username=getenv("USER");
					if(0 == strncmp(tmp+6,username,strlen(username))){
						tmp[0]='~';
						strncpy(tmp+1, tmp+strlen(username)+6, strlen(tmp) - strlen(username) - 6);
						tmp[strlen(tmp) - strlen(username) - 6 + 1] = '\0';
					}
					break;
				case '$':
					tmp[0] = '$';
					tmp[1] = '\0';
					break;
				default:
					free(tmp);
					free(ps1_value);
					printf("error in parse_PS1: not supported PS1 format!\n");
					return NULL;
			}

			if(tmp != NULL) {
				len = min(strlen(tmp), MAX_VAR_SIZE - j - 1);
				strncpy(ps1_value+j, tmp, len);
				j += len; i += 2;
			} else {
				i += 2;
			}
			continue;
		}
	}

	if(j < MAX_VAR_SIZE)
		ps1_value[j] = '\0';
	else
		ps1_value[MAX_VAR_SIZE - 1] = '\0';

	free(tmp);

	return ps1_value;
}

char** parse_cmds(char* cmd, int* arg_size){
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

	*arg_size = j;
	arg_list[j] = NULL;
	return arg_list;
}

void execute(char** arg_list, int arg_size, char** sh_var_list, char** prompt){
	char *buf, *ptr;
	int err, len, len1;

	buf = (char*) malloc(MAX_BUFFER_SIZE * sizeof(char));
	buf[MAX_BUFFER_SIZE - 1] = '\0';

	if(0 == strcmp(arg_list[0],"cd")){
		if(arg_size == 1)
			err = chdir(getenv("HOME"));
		else if(arg_size == 2){
			if(arg_list[1][0] != '/' && arg_list[1][0] != '~'){
				ptr = getcwd(NULL,MAX_BUFFER_SIZE - 1);
				if(ptr != NULL){
					len = min(MAX_BUFFER_SIZE - 1, strlen(ptr));
					strncpy(buf, ptr, len);
					buf[len] = '/';
					len1 = min(MAX_BUFFER_SIZE - len - 1, strlen(arg_list[1]));
					strncpy(buf + len + 1, arg_list[1], len1);
					buf[len1 + len + 1] = '\0';
				} else {
					printf("error in execute: can't get current working directory!\n");
					return;
				}
			}else if(arg_list[1][0] == '~'){
				ptr = getenv("HOME");
				if(ptr != NULL){
					len = min(MAX_BUFFER_SIZE - 1, strlen(ptr));
					strncpy(buf, ptr, len);
					buf[len] = '/';
					if(strlen(arg_list[1]) > 1){
						len1 = min(MAX_BUFFER_SIZE - len - 1, strlen(arg_list[1]) - 1);
						strncpy(buf + len + 1, arg_list[1] + 1, len1);
						buf[len1 + len + 1] = '\0';
					} else
						buf[len + 1] = '\0';
				} else {
					printf("error in execute: can't get env HOME!\n");
					return;
				}
			} else {
				strncpy(buf, arg_list[1],strlen(arg_list[1]));
			}

			debug_printf("execute:buf = %s\n", buf);
			err = chdir(buf);

			if (err != 0){
				printf("error in execute: wrong path given in cd!\n");
				return;
			}

		}
		*prompt = parse_PS1(get_var(sh_var_list, "PS1="));
	}

	while(*arg_list)
		printf("%s\n",*arg_list++);
}

int min(int a, int b){
	return a>b ? b:a;
}

int max(int a, int b){
	return a<b ? b:a;
}

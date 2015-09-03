#include<stdio.h>
#include<stdlib.h>
#include<stdarg.h>
#include<unistd.h>
#include<string.h>
#include<dirent.h>
#include<sys/types.h>
#include<sys/stat.h>

#define MAX_CMD_SIZE 256 // max sbush cmd size
#define MAX_ARG_NUM 8    // max argument number in a sbush cmd
#define MAX_SH_VAR_NUM 8 // max shell variable number
#define MAX_ENV_VAR_NUM 256 // max environment variable number
#define MAX_VAR_SIZE 256 // max shell variable size
#define MAX_DIR_SIZE 256 // max directory/path size
#define MAX_BUFFER_SIZE 256 // hostname size

//#define DEBUG

char** parse_cmds(char* cmd, int* arg_size); // parse user cmds into a list of strings
char* get_var(char** var_list, char* var_name); // get variable value by its name in var_list
void set_var(char** var_list, char* var_name, char* var_value); // set variable value in var_list
void set_path(char** var_list, char* path_value); // set path value, e.g. PATH=$PATH:/usr/bin
void set_PS1(char** var_list, char* PS1_value); // set PS1 value, e.g. PS1=\u@\h:\w\$
char* parse_PS1(char* ps1); // parse PS1 symbols into readable string
int min(int a, int b);
int max(int a, int b);
int is_regular_file(const char *path); // borrowed online
int does_file_exist(const char *filename);
int is_file_executable(const char *filename);
char* locate_executable_file(char** var_list, char* filename);
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
	char *cmd, *tmp, *prompt;
	char** arg_list;
	char** SH_VAR_LIST;
	int arg_size;
	
	FILE* fd;
	char *line;
	int len;

	tmp = (char*) malloc(MAX_DIR_SIZE * sizeof(char));
	tmp = getcwd(NULL, MAX_DIR_SIZE);

	SH_VAR_LIST = (char**) malloc(MAX_SH_VAR_NUM * sizeof(char*));

	tmp = get_var(envp,"PATH=");
	set_var(SH_VAR_LIST, "PATH=", tmp);

	set_var(SH_VAR_LIST, "PS1=", "\\u@\\h:\\w\\$ ");
	tmp = get_var(SH_VAR_LIST,"PS1=");

	prompt = parse_PS1(tmp);
/*
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

	int i;
	i = 0;
	printf("argv is %d\n", argc);
	while(argv[i]){
		printf("%s\n", envp[i]);
		i += 1;
	}
*/

	// read cmds from sbush scripts!
	if(argc == 2){
		len = strlen(argv[1]);
		if(len > 3 && 0 == strncmp(argv[1]+len-3,".sh", 3) && is_file_executable(argv[1])){
			fd = fopen(argv[1], "r");

			if(fd == NULL){
				printf("error in main: file does not exists %s\n", argv[1]);
				return 0;
			}

			line = (char*) malloc(MAX_BUFFER_SIZE * sizeof(char));
			while(fgets(line, MAX_BUFFER_SIZE - 1, fd) != NULL) {
				//printf("%s", line);
				arg_list = parse_cmds(line, &arg_size);
				if(!arg_list)
					continue;
				execute(arg_list, arg_size, SH_VAR_LIST, &prompt);
			}
			free(line);
			fclose(fd);
		}

		return 0;
	}

	if(argc > 2){
		printf("OK, I'm still a baby! Don't play too much with me, or I will cry(crush)!\n");
		return 0;
	}

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

void set_PS1(char** var_list, char* new_PS1){
	if (NULL == parse_PS1(new_PS1)){
		printf("error in set_PS1: unsupported PS1 format!\n");
		return;
	}else
		set_var(var_list, "PS1=", new_PS1);
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
	int quote_flag;
	
	arg_list = (char**) malloc(MAX_ARG_NUM * sizeof(char*));	

	j = 0;// arg number, < MAX_ARG_NUM
	p = cmd;
	while(*p && *p != '#' && *p != '\n' && j<MAX_ARG_NUM){
		while(*p == ' ')
			p++;// skip spaces

		i = 0;
		tmp = (char*) malloc(MAX_CMD_SIZE * sizeof(char));//TODO:suppose malloc always success
		quote_flag=0;
		while(*p != '\n' && *p != '#' && i < MAX_CMD_SIZE - 1){
			if(*p == '"' && quote_flag == 0)
				quote_flag = 1;
			else if(*p == '"' && quote_flag == 1){
				if(*(p+1) != ' ' && *(p+1) != '\n'){//ugly input like ABC="ab c"d
					printf("error in parse_cmds: ugly input!\n");
					free(tmp);//TODO maybe we should free the whole arg_list
					return NULL;
				} else {
					tmp[i++] = *p++;
					break;
				}
			}

			if(*p == ' ' && quote_flag == 0)
				break;
			else
				tmp[i++] = *p++;
		}
		
		//take one arg
		if(i > 0 && i < MAX_CMD_SIZE){
			tmp[i] = '\0';
			arg_list[j++] = tmp;
		}else if(i >= MAX_CMD_SIZE){
			printf("error: argument too long!\n");
			return NULL;
		}else if(*p == '\n' || *p == '#')
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
	struct dirent *entry;
	DIR *dir;
	char *buf, *buf1, *ptr;
	int err, len, len1;
	int *status_ptr;
	pid_t pid;

	buf = (char*) malloc(MAX_BUFFER_SIZE * sizeof(char));
	buf[MAX_BUFFER_SIZE - 1] = '\0';
	buf1 = (char*) malloc(MAX_BUFFER_SIZE * sizeof(char));
	buf1[MAX_BUFFER_SIZE - 1] = '\0';

	if(arg_list[0] == NULL)
		return;

	if(0 == strcmp(arg_list[0],"cd") || 0 == strcmp(arg_list[0], "ls")){
		if(arg_size == 1){
			if(0 == strcmp(arg_list[0], "cd"))
				err = chdir(getenv("HOME"));
			else{
					dir = opendir(getcwd(NULL, MAX_BUFFER_SIZE - 1));

					while((entry = readdir(dir)) != NULL){
						printf("%s\t", entry->d_name);
					}
					printf("\n");
			}
		} else if(arg_size == 2){
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
			if(0 == strcmp(arg_list[0],"cd")){ // cd
				err = chdir(buf);

				if (err != 0){
					printf("error in execute: wrong path given in cd!\n");//TODO:need to differentiate the err code
					free(buf);
					free(buf1);
					return;
				}
			}else{// ls
				if(is_regular_file(buf)){
					printf("%s\n",buf);
				}else{
					dir = opendir(buf);

					while((entry = readdir(dir)) != NULL){
						printf("%s	", entry->d_name);
					}
					printf("\n");
				}
			}

		}

	} else if(ptr=strchr(arg_list[0], '=')) {// handles set sh variables

		if(arg_size > 1){
			printf("error in execute: unaccepted format!\n");
			free(buf);
			free(buf1);
			return;
		}

		len = strlen(ptr);// string length from '=' to the end, e.g. VAR="var_value"
		len1 = strlen(arg_list[0]);

		if(len1 > len){// get the var_name, cmd not begin with '='
			strncpy(buf, arg_list[0], len1 - len + 1);
			buf[len1 - len + 1] = '\0';
		}else{
			printf("error in execute: unknown commands!\n");
			free(buf);
			free(buf1);
			return;
		}

		// check value, "abc" equals abc, note *ptr='='
		if(ptr[1] == '"' && len >= 3 && ptr[len - 1] == '"'){
			strncpy(buf1, ptr + 2, len - 3);
			buf1[len - 3] = '\0';
		}else if(ptr[1] != '"' && ptr[len - 1] != '"'){
			strncpy(buf1, ptr + 1, len - 1);
			buf1[len - 1] = '\0';
		}else{
			printf("error in execute: unaccepted var values!\n");
			free(buf);
			free(buf1);
			return;
		}

		if(0 == strncmp(buf,"PATH=", 5))
			set_path(sh_var_list, buf1);
		else if(0 == strncmp(buf, "PS1=", 4))
			set_PS1(sh_var_list, buf1);
		else
			set_var(sh_var_list, buf, buf1);
	} else {
		ptr = locate_executable_file(sh_var_list, arg_list[0]);
		if(ptr){
			// TODO: execute this file with args from arg_list 1..
			debug_printf("executable file is %s\n",ptr);
			pid = fork();
			if(pid == 0){
				// child process
				if(0 == strcmp(arg_list[arg_size - 1], "&"))
					arg_size = arg_size - 1;

				switch(arg_size){
					case 1: execlp(ptr, ptr, (char*)NULL); break;
					case 2: execlp(ptr, ptr, arg_list[1], (char*)NULL); break;
					case 3: execlp(ptr, ptr, arg_list[1], arg_list[2], (char*)NULL); break;
					case 4: execlp(ptr, ptr, arg_list[1], arg_list[2], arg_list[3], (char*)NULL); break;
					case 5: execlp(ptr, ptr, arg_list[1], arg_list[2], arg_list[3], arg_list[4], (char*)NULL); break;
					case 6: execlp(ptr, ptr, arg_list[1], arg_list[2], arg_list[3], arg_list[4], arg_list[5], (char*)NULL); break;
					default:
						printf("error in execute: too many arguments\n");
						free(buf);
						free(buf1);
						return;
				}
			} else if (pid > 0){
				if(0 != strcmp(arg_list[arg_size - 1], "&"))
					waitpid(pid, status_ptr, 0);
				free(buf);
				free(buf1);
				return;
			} else {
				printf("error in execute: fork process error!\n");
				free(buf);
				free(buf1);
				return;
			}

		} else {
			free(buf);
			free(buf1);
			return;
		}
	}

	*prompt = parse_PS1(get_var(sh_var_list, "PS1="));
/*	while(*sh_var_list)
		printf("%s\n",*sh_var_list++);
*/
}

int min(int a, int b){
	return a>b ? b:a;
}

int max(int a, int b){
	return a<b ? b:a;
}

int is_regular_file(const char *path){
	struct stat path_stat;
	stat(path, &path_stat);
	return S_ISREG(path_stat.st_mode);
}

int does_file_exist(const char *filename) {
    struct stat st;
    int result = stat(filename, &st);
    return result == 0;
}

int is_file_executable(const char *filename){
	struct stat path_stat;
	return stat(filename, &path_stat) == 0 && path_stat.st_mode & S_IXUSR;
}

char* locate_executable_file(char **var_list, char *filename){
	char *path, *pre, *p;
	char *buf;
	int last_len, cur_len;
	path = get_var(var_list,"PATH=");

	// check if file exists in current working directory
	if(does_file_exist(filename)){
		if(is_file_executable){
			debug_printf("we found an executable here: %s\n",filename);
			return filename;
		}
		else{
			printf("error in locate_executable_file: file is not executable!\n");
			return NULL;
		}
	}

	buf = (char*) malloc(MAX_BUFFER_SIZE * sizeof(char));

	pre = path;
	last_len = strlen(pre);
	while(last_len > 0 && pre != NULL && (p = strstr(pre, ":")) != NULL){
		cur_len = strlen(p);
		strncpy(buf, pre, last_len - cur_len);
		buf[last_len - cur_len] = '/';

		if((strlen(filename) + last_len - cur_len + 1) < MAX_BUFFER_SIZE - 1){
			strncpy(buf+last_len - cur_len + 1, filename, strlen(filename));
		} else {
			printf("error in locate_executable_file: filename too long!\n");
			free(buf);
			return NULL;
		}

		buf[last_len - cur_len + 1 + strlen(filename)] = '\0';
		if(does_file_exist(buf)){
			if(is_file_executable(buf))
				return buf;
			else{
				printf("error in locate_executable_file: filename is not executable!\n");
				free(buf);
				return NULL;
			}
		}
		last_len = cur_len - 1;
		pre = p + 1;
	}

	if(last_len > 0 && pre != NULL && (p = strstr(pre, ":")) == NULL){
		cur_len = strlen(pre);
		strncpy(buf, pre, cur_len);
		buf[cur_len] = '/';

		if((strlen(filename) + cur_len + 1) < MAX_BUFFER_SIZE - 1){
			strncpy(buf + cur_len + 1, filename, strlen(filename));
		} else {
			printf("error in locate_executable_file: filename too long!\n");
			free(buf);
			return NULL;
		}

		buf[cur_len + 1 + strlen(filename)] = '\0';
		if(does_file_exist(buf)){
			if(is_file_executable(buf))
				return buf;
			else{
				printf("error in locate_executable_file: filename is not executable!\n");
				free(buf);
				return NULL;
			}
		}
	}

	printf("error in locate_executable_file: filename does not exist!\n");
	free(buf);
	return NULL;
}

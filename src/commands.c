#define UNIX_PATH_MAX 108
#define SOCK_PATH "tpf_unix_sock.server"
#define CLIENT_PATH "tpf_unix_sock.server"
#define DATA "Hello from server"
#define SERVER_PATH "tpf_unix_sock.server"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/socket.h>
//#include <sys/un.h>
#include <pthread.h>

#include "commands.h"
#include "built_in.h"

static struct built_in_command built_in_commands[] = { 		//명령어 구현
  { "cd", do_cd, validate_cd_argv },// 0,1이면 정상작동
  { "pwd", do_pwd, validate_pwd_argv },
  { "fg", do_fg, validate_fg_argv }
};

static int is_built_in_command(const char* command_name)	//
{
  static const int n_built_in_commands = sizeof(built_in_commands) / sizeof(built_in_commands[0]);

  for (int i = 0; i < n_built_in_commands; ++i) {
    if (strcmp(command_name, built_in_commands[i].command_name) == 0) {
      return i;
    }
  }

  return -1; // Not found
}

struct sockaddr_un {
       unsigned short int sun_family; /* AF_UNIX */
       char sun_path[UNIX_PATH_MAX]; /* pathname */
};

//server socket 구현
void *server(void *com2){

	struct single_command *com = (struct single_command *)com2;
	int server_sock, client_sock, len, rc;
	int bytes_rec =0;
	struct sockaddr_un server_sockaddr;	
	struct sockaddr_un client_sockaddr;	
	char buf2[256];
	int backlog =10;

	memset(&server_sockaddr, 0, sizeof(struct sockaddr_un));	//초기화
	memset(&client_sockaddr, 0, sizeof(struct sockaddr_un));	//초기화
	memset(buf2, 0, 256);		//초기화

	server_sock = socket(AF_UNIX, SOCK_STREAM, 0);		//소켓 생성&소켓 번호를 server_sock에 대입
	printf("Server start!\n");
	if(server_sock == -1){
		printf("SOCKET ERROR\n");
		exit(1);
	}

	server_sockaddr.sun_family = AF_UNIX;
	strcpy(server_sockaddr.sun_path, SOCK_PATH);
	len = sizeof(server_sockaddr);

	unlink(SOCK_PATH);		//경로명 제거 안할시 bind()실패
	rc = bind(server_sock, (struct sockaddr *) &server_sockaddr, len);
	if(rc==-1){
		printf("BIND ERROR\n");
		close(server_sock);
		exit(1);
	}

	printf("Waiting for listening to server\n");
	rc = listen(server_sock, backlog);	//성공시 0반환
	if(rc == -1){
		printf("Listen error\n");
		close(server_sock);
		exit(1);
	}
	printf("socket listening...\n");

	client_sock = accept(server_sock, (struct sockaddr *) &client_sockaddr, &len);
	if(client_sock == -1){
		printf("Accept error\n");
		close(server_sock);
		close(client_sock);
		exit(1);
	}
	printf("Get name of connected Socket\n");
	len = sizeof(client_sockaddr);
	rc = getpeername(client_sock, (struct sockaddr *) &client_sockaddr, &len);
	if(rc == -1){
		printf("getpeername error\n");
		close(server_sock);
		close(client_sock);
		exit(1);
	}

	else{
		printf("Client socket filepath : %s\n", client_sockaddr.sun_path);
	}
	
	dup2(client_sock,0);	//stdin
	close(client_sock);
	printf("Waiting to read...\n");
	
	process(com);
	if(execv(com->argv[0],com->argv)==-1){
		printf("Error!\n");		
	}
	memset(buf2, 0, 256);

	close(server_sock);
//	close(client_sock);
	
//	return 0;
	exit(1);
}
//client socket구현부분
int client(struct single_command *com){
	
	printf("com : %s\n",com->argv[0]);

	int client_sock, rc, len;
	struct sockaddr_un server_sockaddr;
	struct sockaddr_un client_sockaddr;
	char buf[256];

	memset(&server_sockaddr, 0, sizeof(struct sockaddr_un));
	memset(&client_sockaddr, 0, sizeof(struct sockaddr_un));

	printf("Creating client socket\n");
	client_sock = socket(AF_UNIX,SOCK_STREAM,0);

	if(client_sock == -1){
		printf("socket error\n");
		exit(1);
	}
	
	printf("Socket Id : %d\n",client_sock);

	client_sockaddr.sun_family = AF_UNIX;
	strcpy(client_sockaddr.sun_path,CLIENT_PATH);
	len = sizeof(client_sockaddr);

	//unlink(CLIENT_PATH);		//경로명 제거 안할 시 싶패
	/*rc = bind(client_sock, (struct sockaddr *) &client_sockaddr, len);
	if(rc == -1){
		printf("Bind error\n");
		close(client_sock);
		exit(1);
	}*/

	printf("Waiting for connect\n");

	server_sockaddr.sun_family = AF_UNIX;
	strcpy(server_sockaddr.sun_path, SERVER_PATH);
	while(1){
	rc = connect(client_sock, (struct sockaddr *) &server_sockaddr, len);
	if(rc ==-1){
		printf("Connect error\n");
		close(client_sock);
		exit(1);
	}else break;
	}
	dup2(client_sock,1);
	close(client_sock);
	execv(com->argv[0],com->argv);
	printf("Closing socket client\n");
	close(client_sock);
	return 0;
}
int thread(struct single_command *com, struct single_command (*commands)[512]){
	int thread_id;
	pthread_t p_thread[1];
	int threadstatus;

	int pid,st,st2;
	if((pid=fork())==-1)		//error
		printf("Error\n");
	else if (pid!=0)		//parent
		wait(&st);
	else{			//child(pid==0)
		if((pid=fork())==-1)		//이 부분에서 fork를 한번 더한다. child가 server, parent가 client
			printf("Error\n");
		else if(pid!=0){
			waitpid(-1,&st2,WNOHANG);		//자식이랑 부모랑 같이 돌도록 설정하는 명령어
			com = (*commands)+1;
			thread_id = pthread_create(&p_thread[0],NULL,server, (void *)com);
			if(thread_id < 0){
				printf("thread p_id error\n");
				exit(1);
			}
			pthread_join(p_thread[0], (void**)&threadstatus);
			printf("---Server finish---\n");
			wait(&st2);
		}
		else{
			client(com);
			printf("---Client finish---\n");
		}
	return 0;
	}
}
/*
 * Description: Currently this function only handles single built_in commands. You should modify this structure to launch process and offer pipeline functionality.
 */

int process(struct single_command *com){
    assert(com->argc != 0);

    int built_in_pos = is_built_in_command(com->argv[0]);
    if (built_in_pos != -1) {
      if (built_in_commands[built_in_pos].command_validate(com->argc, com->argv)) {
        if (built_in_commands[built_in_pos].command_do(com->argc, com->argv) != 0) {
          fprintf(stderr, "%s: Error occurs\n", com->argv[0]);
        }
      } else {
        fprintf(stderr, "%s: Invalid arguments\n", com->argv[0]);
        return -1;

      }
    } else if (strcmp(com->argv[0], "") == 0) {
      return 0;
    } else if (strcmp(com->argv[0], "exit") == 0) {
      return 1;
    } else {

   	int pid;
	pid = fork();
	if(pid==0)
	{	
		if(execv(com->argv[0],com->argv)==-1){	//path resolution해결방법
	if(!access(com->argv[0], X_OK)) return 1;
	char *path = malloc(strlen("/usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin"));
	strcpy(path, "/usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin");

	char *token;
	token = strtok(path, ":");
	
	while(token !=NULL){
		char *resolute_path = malloc(512);

		strcat(resolute_path, token);
		strcat(resolute_path,"/");
		strcat(resolute_path,com->argv[0]);
		if(!access(resolute_path,X_OK)){
			strcpy(com->argv[0],resolute_path);
		}
			token = strtok(NULL,":");
	}
			return execv(com->argv[0],com->argv);
		}
		else 
			return execv(com->argv[0],com->argv);
	}	
	else{
		waitpid(pid,NULL,0);
		return 0;
	}
	}
  return 0;
}

int evaluate_command(int n_commands, struct single_command (*commands)[512])
{
  if (n_commands== 1) {
    struct single_command* com = (*commands);
    return process(com);
  } else if(n_commands>=2){
    struct single_command* com = (*commands);
    struct single_command* com2 = (*commands+1);	
    thread(com,commands);
  }
  else
	return 0;
}

void free_commands(int n_commands, struct single_command (*commands)[512])
{
  for (int i = 0; i < n_commands; ++i) {
    struct single_command *com = (*commands) + i;
    int argc = com->argc;
    char** argv = com->argv;

    for (int j = 0; j < argc; ++j) {
      free(argv[j]);
    }

    free(argv);
  }

  memset((*commands), 0, sizeof(struct single_command) * n_commands);
}

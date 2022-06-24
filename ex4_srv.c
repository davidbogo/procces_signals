#include <stdio.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <strings.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

int run = 1;

char* itoa(int val, int base){
	
	static char buf[32] = {0};
	
	int i = 30;
	
	for(; val && i ; --i, val /= base)
	
		buf[i] = "0123456789abcdef"[val % base];
	
	return &buf[i+1];
	
}

void readfileline(int fd, char *buf, size_t buf_len)
{
    int offset = 0;
    if (!buf || buf_len == 0)
        return;
    while (offset + 1 < buf_len) {
        size_t bytes_read = read(fd, buf + offset, 1);
        if (bytes_read == 0)
            break;
        if ((buf[offset] == '\n') || (buf[offset] == EOF))
            break;	// the character will later be replaced by 0
        offset++;
    }
    buf[offset] = 0;
}

void alarm_handler(int alarm)
{
    printf("The server was closed because no service request was received for the last 60 seconds\n");
    run = 0;
}

void srv_handler(int siguser1)
{
    int child_pid;
    alarm(60);
    child_pid = fork();
        switch (child_pid) {
        case 0:
        {
            char to_client[512] = "to_client_";
            char num_of_client[512];
            char first_param[512];
            char operator[512];
            char second_param[512];
            char result[512] = "";
            int open_to_srv, open_client, first_num, second_num, num_result, client_id;
            // We're the child
            open_to_srv = open("to_srv.txt", O_RDONLY);
            if (open_to_srv < 0) {
                printf("ERROR_FROM_EX4\n");
                raise(SIGTERM);
                raise(SIGKILL);
            }
            readfileline(open_to_srv, num_of_client, sizeof(num_of_client));
            readfileline(open_to_srv, first_param, sizeof(first_param));
            readfileline(open_to_srv, operator, sizeof(operator));
            readfileline(open_to_srv, second_param, sizeof(second_param));
            close(open_to_srv);
            remove("to_srv.txt");
            //open specific client
            strcat(num_of_client, ".txt");
            strcat(to_client, num_of_client);
            //open(to_client_xxx.txt)
            open_client = open(to_client,  O_WRONLY | O_CREAT | O_TRUNC, 0666);
            //calculate
            switch(operator[0]) {
            case '1':
                first_num = atoi(first_param);
                second_num = atoi(second_param);
                num_result = (first_num + second_num);
                strcat(result, itoa(num_result, 10));
                break;
            case '2':
                first_num = atoi(first_param);
                second_num = atoi(second_param);
                num_result = (first_num - second_num);
                strcat(result, itoa(num_result, 10));
                break;
            case '3':
                first_num = atoi(first_param);
                second_num = atoi(second_param);
                num_result = (first_num * second_num);
                strcat(result, itoa(num_result, 10));
                break;
            case'4':
                if (atoi(second_param) == 0) {
                    strcat(result, "you can't devide by zero!");
                } else {
                    first_num = atoi(first_param);
                    second_num = atoi(second_param);
                    num_result = (first_num / second_num);
                    strcat(result, itoa(num_result, 10));
                }
                break;
            default:
                printf("ERROR_FROM_EX4\n");
                raise(SIGTERM);
                raise(SIGKILL);
                
            }
            write(open_client, result, strlen(result));
            close(open_client);
            //signal(SIGUSER2)
            client_id = atoi(num_of_client);
            kill((pid_t)client_id, SIGUSR2);
            exit(0);
            break;
        }
        case -1:
            printf("ERROR_FROM_EX4\n");
            return -1;
        default:
            signal(SIGCHLD, SIG_IGN);
            signal(SIGUSR1, srv_handler);
            break;
        }
}

int main()
{
    signal(SIGUSR1, srv_handler);
    signal(SIGALRM, alarm_handler);
    
    remove("to_srv.txt"); //don't care if it fails. It should!
    while(run) {
        alarm(6);
        pause();
    }
    return -1;
}

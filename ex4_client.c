#include <sys/random.h>
#include <linux/random.h>
#include <errno.h>
#include <stdio.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <strings.h>
#include <stdlib.h>
#include <string.h>

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
    printf("Client closed because no response was received from the server for 30 seconds\n");
    raise(SIGTERM);
    raise(SIGKILL);
}

void client_handler(int siguser2)
{
    //open(to_client_xxx.txt) and delete in the end
    char to_client[512] = "to_client_";
    char pid_char[512];
    char result[512];
    int my_pid, open_client;
    alarm(0);
    my_pid = getpid();
    strcpy(pid_char, itoa(my_pid, 10));
    strcat(pid_char, ".txt");
    strcat(to_client, pid_char);
    open_client = open(to_client, O_RDONLY);
    readfileline(open_client, result, sizeof(result));
    printf("the result is: %s\n", result);
    close(open_client);
    if (remove(to_client) < 0) {
        printf("ERROR_FROM_EX4 client\n");
        raise(SIGTERM);
        raise(SIGKILL);
    }
}

int main(int argc, char** argv)
{
    signal(SIGUSR2, client_handler);
    signal(SIGALRM, alarm_handler);
    
    char srv_id[512];
    char to_client[512] = "to_client_";
    char write_char[512];
    char client_id[512];
    char first_param[512];
    char operator[512];
    char second_param[512];
    int count_open_tries = 0;
    int get_random;
    int random_num, random_sleep;
    int client;
    if (argc != 5) {
        printf("ERROR_FROM_EX4 no args\n");
        return -1;
    }
    strcpy(srv_id, argv[1]);
    strcpy(first_param, argv[2]);
    strcpy(operator, argv[3]);
    strcpy(second_param, argv[4]);
    while(count_open_tries < 11) {
        int to_srv_open;
        to_srv_open = open("to_srv.txt",  O_WRONLY | O_CREAT | O_EXCL , 0666);
        if (to_srv_open < 0) {
            if (errno == EEXIST) {
                count_open_tries++;
                if (count_open_tries < 11) {
                    get_random = getrandom(&random_num, sizeof(random_num), GRND_RANDOM);
                    if (get_random < 0) {
                        printf("ERROR_FROM_EX4 cant\n");
                        return -1;
                    }
                    if (random_sleep < 0)
                        random_sleep = -random_sleep;
                    random_sleep = ((random_num % 5) + 1);
                    sleep(random_sleep);
                }
                continue;
            } else {
                printf("ERROR_FROM_EX4 wont\n");
                return -1;
            }
        }
        client = getpid();
        strcpy(client_id, itoa(client, 10));
        strcpy(write_char, client_id);
        strcat(write_char, "\n");
        strcat(write_char, first_param);
        strcat(write_char, "\n");
        strcat(write_char, operator);
        strcat(write_char, "\n");
        strcat(write_char, second_param);
        strcat(write_char, "\n");
        //write four lines of client, params and operator
        write(to_srv_open, write_char, strlen(write_char));
        close(to_srv_open);
        kill((pid_t)(atoi(srv_id)), SIGUSR1);
        alarm(30);
        pause();
        break;
    }
    if (count_open_tries > 10) {
        printf("no available 'to_srv' file for me. I'm done!\n");
        return -1;
    }
    return 0;
}
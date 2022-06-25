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

//David Bogoslavsky 316393974

#define MAX_SLEEP_TIMES 11
#define COUNT_LIMIT 10
#define DECIMAL 10
int run = 1;

#define MAX_DIGITS_TO_CONVERT   30

static char itoa_res[32];

const char* itoa(int val)
{   // The function is non-re-entrant since it uses a static buffer, but
    // this is good enough for this task
    sprintf(itoa_res, "%d", val);
    return itoa_res;
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
    run = 0;
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
    strcpy(pid_char, itoa(my_pid));
    strcat(pid_char, ".txt");
    strcat(to_client, pid_char);
    open_client = open(to_client, O_RDONLY);
    readfileline(open_client, result, sizeof(result));
    printf("%s\n", result);
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
    
    char *srv_id = argv[1];
    char *first_param = argv[2];
    char *operator = argv[3];
    char *second_param = argv[4];
    char to_client[512] = "to_client_";
    char write_char[512];
    char client_id[512];
    int count_open_tries = 0;
    int random_sleep;
    size_t random_num;
    int client;
    if (argc != 5) {
        printf("ERROR_FROM_EX4\n");
        return -1;
    }
    if ((atoi(argv[3]) > 4) || (atoi(argv[3]) < 1)) {
        printf("ERROR_FROM_EX4\n");
        return -1;
    }
    while(count_open_tries < MAX_SLEEP_TIMES) {
        int to_srv_open;
        to_srv_open = open("to_srv.txt",  O_WRONLY | O_CREAT | O_EXCL , 0666);
        if (to_srv_open < 0) {
            if (errno == EEXIST) {
                count_open_tries++;
                if (count_open_tries < 11) {
                    getrandom(&random_num, sizeof(random_num), GRND_RANDOM);
                    random_sleep = ((random_num % 5) + 1);
                    sleep(random_sleep);
                }
                continue;
            } else {
                printf("ERROR_FROM_EX4\n");
                return -1;
            }
        }
        client = getpid();
        strcpy(client_id, itoa(client));
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
    if (count_open_tries > COUNT_LIMIT) {
        printf("no available 'to_srv' file for me. I'm done!\n");
        return -1;
    }
    if (!run) {
        return -1;
    }
    return 0;
}
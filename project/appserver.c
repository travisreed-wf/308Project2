#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "appserver.h"
#include "Bank.h"
#include <unistd.h>
#include <errno.h>

//TODO seg fault if youend immediately
node *root;
node *current;
pthread_mutex_t mutex;
pthread_t *threads;
account *accounts;
FILE * output_file;
char output_file_name[20] = "";
int num_of_requests = 2;
int take_input = 1;
int num_of_worker_threads = 0;
int num_of_accounts = 0;

int main(int argc, char * argv[]) {
//entry point for the program
//arguments are <# of worker threads> <# of accounts> <output file>;

    if (argc != 4) {
        printf("Not enough arguments provided");
        return 0;
    }
    num_of_worker_threads = atoi(argv[1]);
    num_of_accounts = atoi(argv[2]);
    strcpy(output_file_name, argv[3]);
    output_file = get_file(output_file_name);
    fclose(output_file);
    initialize_accounts(num_of_accounts);
    pthread_mutex_init(&mutex, NULL);
    create_worker_threads(num_of_worker_threads);
    accounts = (int *) malloc(sizeof(account) * num_of_accounts);
    for( int i = 0; i < num_of_accounts; i++)
    {
        account a;
        pthread_mutex_init(&a.lock, NULL);
        accounts[i] = a;
    }
    handle_input();


  return 0;
}

FILE * get_file(char * filename){
    FILE *file;
    if (fopen(filename, "r") == NULL) {
        if (errno == ENOENT) {
            return fopen(filename, "a");
        }
    }
    return file;
}

void handle_input(){
    char str[100];
    fgets(str, 99, stdin);
    request req;
    gettimeofday(&req.start, NULL);
    printf("ID 1\n");
    int len = strlen(str);
    if( str[len-1] == '\n' )
    str[len-1] = 0;
    req.request_id = 1;
    strcpy(req.command, str);
    if (!strcmp("END", str)){
        take_input = 0;
    }
    root = (node *) malloc( sizeof(node) );
    root->next = 0;
    root->r = req;
    while (take_input == 1){
        char str[100];
        fgets(str, 99, stdin);
        request req;
        gettimeofday(&req.start, NULL);
        printf("ID %d\n", num_of_requests);
        int len = strlen(str);
        if( str[len-1] == '\n' )
        str[len-1] = 0;
        req.request_id = num_of_requests;
        strcpy(req.command, str);
        if (!strcmp(str, "END")){
            take_input = 0;
        }
        //Iterate to the end of the linked list
        pthread_mutex_lock(&mutex);
        if ( root != NULL ) {
            current = root;
            while ( current->next != NULL)
            {
                current = current->next;
            }
            /* Create a node at the end of the list */
            current->next = (node *) malloc( sizeof(node) );
            current = current->next;
            current->next = NULL;
            current->r = req;
        }
        else {
            root = (node *) malloc( sizeof(node) );
            root->next = NULL;
            root->r = req;

        }
        pthread_mutex_unlock(&mutex);
        // output_file = fopen(output_file_name, "a");
        // fprintf(output_file, "Request %d received: %s\n", num_of_requests, str);
        // fclose(output_file);
        num_of_requests++;
    }
    output_file = fopen(output_file_name, "a");
    fprintf(output_file, "END command received by main\n");
    fclose(output_file);
    for (int i = 0;i < num_of_worker_threads; i++){
        pthread_join(threads[i], NULL);
    }
    return;


}

void run_command(request req){

    char list [21][20] = {"","","","","","","","","","","","","","","","","","","","",""};
    char* token;
    int i = 0;
    token = strtok(req.command, " ");
    while (token != NULL){
        strcpy(list[i],token);
        token = (strtok(NULL, " "));
        i++;
    }
    if (!strcmp(list[0], "END")){
        handle_end(req.request_id);
    }
    else if (!strcmp(list[0], "CHECK")){
        handle_balance_check(req.request_id, atoi(list[1]), req.start);
    }
    else if (!strcmp(list[0], "TRANS")){
        handle_transaction(req.request_id, list, req.start);
    }
    else{
        printf("Invalid Input\n");
    }
}

void handle_end(int request_id){
    take_input = 0;
    output_file = fopen(output_file_name, "a");
    fprintf(output_file, "END\n");
    fclose(output_file);


}
void handle_balance_check(int request_id, int account_id, struct timeval start){
    if (account_id > num_of_accounts || account_id <= 0){
        output_file = fopen(output_file_name, "a");
        fprintf(output_file,"%d Invalid Input\n", request_id);
        fclose(output_file);
        printf("Invalid account number\n");
        return;
    }
    pthread_mutex_lock(&accounts[account_id-1].lock);
    int balance = read_account(account_id);
    pthread_mutex_unlock(&accounts[account_id-1].lock);
    output_file = fopen(output_file_name, "a");
    struct timeval end;
    gettimeofday(&end, NULL);
    fprintf(output_file, "%d BAL %d TIME %d.%06d %d.%06d\n", request_id, balance, start.tv_sec, start.tv_usec, end.tv_sec, end.tv_usec);
    fclose(output_file);

}
void handle_transaction(int request_id, char list[21][20], struct timeval start) {
    int args = 0;
    output_file = fopen(output_file_name, "a");
    int safe = 1;
    int successful = 0;
    while (successful == 0){
        while(strcmp(list[args], "")){
            args++;
        }
        for (int m =1; m<args;m=m+2){
            if (atoi(list[m]) > num_of_accounts || atoi(list[m]) <= 0){
                fprintf(output_file,"%d Invalid Input\n", request_id);
                fclose(output_file);
                printf("Invalid account number");
                return;
            }
        }
        fprintf(output_file, "Number of args %d\n", args);
        for (int i = 1;i<args;i=i+2){
            fprintf(output_file, "Trying on %d\n", atoi(list[i]));
            if (pthread_mutex_trylock(&accounts[atoi(list[i])-1].lock) != 0){
                safe = 0;
                if (i > 1){
                   for (int j=i-2;j>=1;j=j-2){
                        fprintf(output_file, "Unsafe - Unlocking %d\n", atoi(list[j]));
                        pthread_mutex_unlock(&accounts[atoi(list[j])-1].lock);
                        usleep(50000);
                    }
                }
            }
            else {
               fprintf(output_file, "Aquired lock on %d\n", atoi(list[i]));
            }
        }
        if (safe == 1){
            int value = 0;
            for (int k=2; k<=args;k+=2){
                value = read_account(atoi(list[k-1]));
                if (value+atoi(list[k]) < 0){
                    safe = 2;
                    struct timeval end;
                    gettimeofday(&end, NULL);
                    fprintf(output_file, "%d ISF %d TIME %d.%06d %d.%06d\n", request_id, atoi(list[k-1]), start.tv_sec, start.tv_usec, end.tv_sec, end.tv_usec);
                    break;
                }
            }
        }
        if (safe == 1){
            int value = 0;
            for (int k=2; k<=args;k+=2){
                value = read_account(atoi(list[k-1]));
                write_account(atoi(list[k-1]), value+atoi(list[k]));
                fprintf(output_file, "Changing %d to be %d from %d\n", atoi(list[k-1]),  (value+atoi(list[k])), value);
            }
            struct timeval end;
            gettimeofday(&end, NULL);
            fprintf(output_file, "%d OK TIME %d.%06d %d.%06d\n", request_id, start.tv_sec, start.tv_usec, end.tv_sec, end.tv_usec);
        }
        if (safe != 0){
            for (int i=1;i<args;i+=2){
               fprintf(output_file, "Unlocking %d\n", atoi(list[i]));
               pthread_mutex_unlock(&accounts[atoi(list[i])-1].lock);
            }
            successful = 1;
        }
        fclose(output_file);
    }


}
void create_worker_threads(int num_of_worker_threads){
    threads = (int *) malloc(sizeof(pthread_t) * num_of_worker_threads);
    for (int i=0;i<num_of_worker_threads;i++){
        /* thread id or type*/
        pthread_t thread;
        threads[i] = thread;
        pthread_create(&thread, NULL, (void*)&worker_thread, NULL);
    }
}

void worker_thread()
{
    while (take_input == 1 || root != NULL) {
        pthread_mutex_lock(&mutex);
        request req;
        if (root != NULL){
            req = root->r;
            root = root->next;
            pthread_mutex_unlock(&mutex);
            output_file = fopen(output_file_name, "a");
            // fprintf(output_file, "Picking up request %d command - %s\n", req.request_id, req.command);
            fclose(output_file);
            run_command(req);

        }
        else {
            pthread_mutex_unlock(&mutex);
        }
    }
    return;
    //     // current = root;
    //     // while ( current != NULL ) {
    //     //     printf( "%s\n", current->r.command );
    //     //     current = current->next;
    //     // }

}

void add_to_output(char str[]){
    output_file = fopen(output_file_name, "a");
    fprintf(output_file, str);
    fclose(output_file);
}



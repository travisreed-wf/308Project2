typedef struct {
    pthread_mutex_t lock;
    int value;
} account;

typedef struct {
    int request_id;
    char command[200];
    struct timeval start;
} request;

typedef struct {
  request r;
  struct node *next;
} node;


int main(int argc, char * argv[]);
FILE * get_file(char * filename);
void handle_input();
void run_command(request req);
void create_worker_threads(int num_of_worker_threads);
void handle_end(int request_id);
void handle_balance_check(int request_id, int account_id, struct timeval start);
void handle_transaction(int request_id, char list[5][20], struct timeval start, int args);
void worker_thread();
void add_to_output(char * str);
void insertionSort(int *a, int array_size);

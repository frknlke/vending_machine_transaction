#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

#define NUM_OF_COMPANIES 5
#define NUM_OF_VENDING_MACHINES 10

/*  GLOBAL VARIABLES    */
int balances[NUM_OF_COMPANIES] = {0, 0, 0, 0, 0};
int TOTAL_NUMBER_OF_CONSUMERS = 0;
int CONSUMER_COUNTER = 0;
bool is_Finished = false;
FILE *fw = NULL;

/*  PTHREAD CONDITIONS AND MUTEX LOCKS FOR SYNCHRONIZATION OF CRITICAL SECTION  */
pthread_cond_t conditions[10] = { PTHREAD_COND_INITIALIZER };
pthread_mutex_t condition_locks[10] = { PTHREAD_MUTEX_INITIALIZER };
pthread_mutex_t vending_locks[10] = { PTHREAD_MUTEX_INITIALIZER };
pthread_mutex_t condition_for_vending[10] = { PTHREAD_MUTEX_INITIALIZER };
pthread_mutex_t locks_for_vending[10] = { PTHREAD_MUTEX_INITIALIZER };
pthread_mutex_t locks_for_balance_operations[NUM_OF_COMPANIES] = { PTHREAD_MUTEX_INITIALIZER };
pthread_mutex_t counter_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t controller_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t index_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t index_cond = PTHREAD_COND_INITIALIZER;

/*  THREAD METHODS  */
void *vending_machine(void *param1);
void *consumer_thread(void *param1);
/*  Struct to hold variables when costumer thread created   */
typedef struct Consumer_Wish{
    int sleep_time;
    int vending_machine_instance;
    char company_name[10];
    int amount;
    int customer_order;
};
/*  Struct to pass information to vending_machine thread    */
typedef struct Info_for_Machine{
    int order_of_customer;
    char company_name[10];
    int company_enum;
    int amount;
    int vending_instance;
};
struct Info_for_Machine operation_variables[10];

//input format :    <sleep time(miliseconds),vending machine instance,company name,amount>  ## No whitespace  
int main(int argc, int *argv[]){
    /*  opening file to take input  */
    char* input_name = argv[1];
    char* line = NULL;
    size_t size = 0;
    __ssize_t read;
    FILE *f = fopen(input_name, "r");
    if(f == NULL){
        printf("Error opening file\n");
        exit(1);
    }
    /*  Creating output file    */
    const char delim[2] = ".";
    char *output_name = strtok(input_name, delim);
    strcat(output_name, "_log.txt");
    fw = fopen(output_name, "w");
    if(fw == NULL){
        printf("Error opening output file\n");
        exit(1);
    }
    /*  Read the first line of the input file that is the number of costumers   */
    read = getline(&line, &size, f);
    int num_of_costumer = atoi(line);
    TOTAL_NUMBER_OF_CONSUMERS = num_of_costumer;

    int sleep_times[num_of_costumer];
    int desired_machine_instance[num_of_costumer];
    char desired_company[num_of_costumer][10];
    int amounts_to_paid[num_of_costumer];
    struct Consumer_Wish arr[num_of_costumer];

    /*  Variables to split the line taken from input file into its delimeter    */
    const char delimeter[2] = ",";
    char *token;

    /*  Read inputs from the file line by line and split the line into its delimeter, then store the values in the corresponding arrays     */
    int count=0;
    while ((read = getline(&line, &size, f)) != -1 && count < num_of_costumer){
       arr[count].sleep_time = (atoi(strtok(line, delimeter)));
       arr[count].vending_machine_instance = atoi(strtok(NULL, delimeter)) - 1;
       strcpy(arr[count].company_name, strtok(NULL,delimeter));
       arr[count].amount = atoi(strtok(NULL, delimeter)); 
        arr[count].customer_order = count+1;
       count++;
    }
    /*  Pthread_t arrays for costumer threads and consumer threads  */
    pthread_t vending_machines[NUM_OF_VENDING_MACHINES];
    pthread_t consumers[TOTAL_NUMBER_OF_CONSUMERS];
    
    /*  Thread creation for vending machines    */
    int idx_of = 0;
    for(int i = 0; i < NUM_OF_VENDING_MACHINES; i++){
        idx_of = i;
        pthread_create(&(vending_machines[i]), NULL, vending_machine, &idx_of);
        pthread_cond_wait(&index_cond,&index_lock);
        //usleep(10);
    }
    //sleep(3);
    /*  Thread creation for costumers   */
    for(int i = 0; i < TOTAL_NUMBER_OF_CONSUMERS ; i++){
        pthread_create(&(consumers[i]), NULL, consumer_thread, &(arr[i]));
    }
    /*  Join segment for consumers  */
    for(int i = 0; i < TOTAL_NUMBER_OF_CONSUMERS; i++){
        pthread_join(consumers[i], NULL);
    }
    /*  Join segment for vending machines   */
    for(int i = 0; i < NUM_OF_VENDING_MACHINES; i++){
        int rc = pthread_cancel(vending_machines[i]);
    }
    /*  Writing final balances to the output file   */
    fprintf(fw,"All prepayments are completed.\n");
    fprintf(fw,"Kevin: %dTL\n",balances[0]);
    fprintf(fw,"Bob: %dTL\n",balances[1]);
    fprintf(fw,"Stuart: %dTL\n",balances[2]);
    fprintf(fw,"Otto: %dTL\n",balances[3]);
    fprintf(fw,"Dave: %dTL\n",balances[4]);
    /*
        for(int i=0; i<5;i++){

        printf("%d\n", balances[i]);
    }*/
/*  Closing file that we created    */
fclose(f);
fclose(fw);
return 0;
}

/*  param1 indicates which machine instance the thread currently running is     */
void *vending_machine(void *param1){
    /*  Instance of the current vending_machine thread  */
    const int instance = *((int*)param1);
    // signal main thread to continue creating vending machine threads
    pthread_cond_signal(&index_cond);

    /*  While loop that iterates until all threads handle */
    while(!is_Finished){
        /*  Cond for waiting until costumer thread send signal that indicates all necessary information passed  */
        pthread_cond_wait(&(condition_for_vending[instance]), &(locks_for_vending[instance]));
        /*  Beginning of critical section for balance operations */
        pthread_mutex_lock(&(locks_for_balance_operations[operation_variables[instance].company_enum]));           /*  Mutex lock before going into critical section   */
        // Balance operation
        balances[operation_variables[instance].company_enum] += operation_variables[instance].amount;              /*  Add the amount taken from costumer to the desired company's balance */
        // Writing operation to the output file
        fprintf(fw,"Customer%d,%dTL,%s\n", operation_variables[instance].order_of_customer, operation_variables[instance].amount, operation_variables[instance].company_name);
        pthread_mutex_unlock(&(locks_for_balance_operations[operation_variables[instance].company_enum]));            /*  Unlock mutex    */
        // end of balance operations
        // sending signal to the costumer thread indicates vending machine is ready for the next consumer
        pthread_cond_signal(&(conditions[instance]));

        // increasing counter that represents number of costumer operation handled in a critical section
        pthread_mutex_lock(&counter_lock);
        CONSUMER_COUNTER++;
        pthread_mutex_unlock(&counter_lock);

        // assigning isFinished flag in a critical section
        pthread_mutex_lock(&controller_lock);
        is_Finished = (CONSUMER_COUNTER >= TOTAL_NUMBER_OF_CONSUMERS);
        pthread_mutex_unlock(&controller_lock);

    }

    pthread_exit(0);
}
/*  Costumer thread funciton    */
/*  *param1 is a void pointer represent a Consumer_Wish struct that consist of operation information    */
void *consumer_thread(void *param1){
    struct Consumer_Wish cons = *((struct Consumer_Wish*)param1);
    usleep(cons.sleep_time*1000);
    
    /*  Try to go into critical section of vending machine by taking its lock   */
    /*  It is allowed only when vending machine is not busy */
    pthread_mutex_lock(&(vending_locks[cons.vending_machine_instance]));                                /*  lock mutex that specific to one vending machine */
    
    operation_variables[cons.vending_machine_instance].order_of_customer = cons.customer_order;
    operation_variables[cons.vending_machine_instance].amount = cons.amount;                            /*  Update the fields that are necessary for vending machine    */
    strcpy(operation_variables[cons.vending_machine_instance].company_name, cons.company_name);
    operation_variables[cons.vending_machine_instance].vending_instance = cons.vending_machine_instance;
    if (strcmp(cons.company_name, "Bob") == 0)
    {
        operation_variables[cons.vending_machine_instance].company_enum = 1;
    }
    else if (strcmp(cons.company_name, "Stuart") == 0)
    {
        operation_variables[cons.vending_machine_instance].company_enum = 2;
    }
    else if (strcmp(cons.company_name, "Otto") == 0)
    {
        operation_variables[cons.vending_machine_instance].company_enum = 3;
    }
    else if (strcmp(cons.company_name, "Dave") == 0)
    {
        operation_variables[cons.vending_machine_instance].company_enum = 4;
    }
    else
    {
        operation_variables[cons.vending_machine_instance].company_enum = 0;
    }

    pthread_cond_signal(&(condition_for_vending[cons.vending_machine_instance]));                       /*  Signal vending machine thread to start operation    */

    pthread_cond_wait(&(conditions[cons.vending_machine_instance]), &(condition_locks[cons.vending_machine_instance]));
                                                                                                        /*  Wait signal from vending machine to unlock mutex    */
    pthread_mutex_unlock(&(vending_locks[cons.vending_machine_instance]));                              /*  Unlock mutex    */

    pthread_exit(0);
}


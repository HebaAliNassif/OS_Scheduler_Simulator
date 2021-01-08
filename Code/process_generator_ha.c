#include "headers.h"
#define MAXCHAR 300

struct node                 //struct to gold the information of each node and a pointer to the next and previous nodes in the linkedlist
{
    struct Process data;
    struct node * next;
    struct node * previous;
};

struct linkedlist           //likedlist struct
{
	struct node * head;
	struct node * tail;
    int size;
};

struct msgbuff
{
    long mtype;
    struct Process P;
};

void addNodeToLikedlistEnd(struct linkedlist* list, struct Process processToAdd)
{
    struct node * nodeToAdd = (struct node *) malloc(sizeof(struct node));  //create new node and assign the process to it.
    nodeToAdd->data = processToAdd;
    nodeToAdd->next = NULL;

    if(list->head == NULL)
    {
        list->head = nodeToAdd;
        list->tail = nodeToAdd;
    }
    else
    {
        list->tail->next = nodeToAdd;
        nodeToAdd->previous = list->tail;
        list->tail = list->tail->next;
    }

    list->size ++;
}


void removeHeadNodeFromLikedlist(struct linkedlist* list)
{
    if(list->head == NULL)
    {
        return;
    }
    else if(list->head->next == NULL)
    {
        free(list->head);
        list->head = NULL;
        list->tail = NULL;
    }
    else
    {
        struct node * tempNode = (struct node *) malloc(sizeof(struct node));  //create new node and assign the process to it.
        tempNode = list->head;
        list->head = list->head->next;
        free(tempNode);
    }
    list->size --;
}


void clearResources(int);
void readFromFileAndFillList(struct linkedlist* list);
void writeInFile();
void chooseAlgorithm(short* algorithmNumber, int* quantum);
bool sendProcessToScheduler(struct Process* p, int* msgq_id); 

int main(int argc, char * argv[])
{
    pid_t pid;
    signal(SIGINT, clearResources);
    struct linkedlist processes = {NULL, NULL, 0};
    
    key_t key_id;
    int msgq_id;

    key_id = ftok("keyfile", 65);
    msgq_id = msgget(key_id, 0666 | IPC_CREAT);

    if (msgq_id == -1)
    {
        perror("Error in create message queue");
        exit(-1);
    }
    printf("Process Generator: Message Queue ID = %d\n", msgq_id);

    // 1. Read the input files.
    readFromFileAndFillList(&processes);
   

    // 2. Ask the user for the chosen scheduling algorithm and its parameters, if there are any.
    short algorithmNumber;
    int quantum = 0;
    chooseAlgorithm(&algorithmNumber, &quantum);
    char quantumBuffer[10];
    sprintf(quantumBuffer, "%d", quantum);

    // 3. Initiate and create the scheduler and clock processes.
    pid=fork();
    if(pid == 0){
        printf("\nClock Initialization Succes with pid = %d\n", getpid()); 
        return execl("./clk.out", "./clk.out", (char*) NULL);
    }
    else if(pid == -1)
    {
        printf("\nClock Initialization Error\n");
        exit(-1); 
    }
    else
    {
        pid=fork();
        if(pid == 0){
            
            printf("\nScheduler Initialization Succes with pid = %d\n", getpid());
            if(algorithmNumber == 1) return execl("./scheduler.out", "./scheduler.out", "1",(char*) NULL);    
            else if(algorithmNumber == 2) return execl("./scheduler.out", "./scheduler.out", "2",(char*) NULL);   
            else if(algorithmNumber == 3) return execl("./scheduler.out", "./scheduler.out", "3", quantumBuffer,(char*) NULL);
            exit(-1);      
        }
        else if(pid == -1)
        {
            printf("\nScheduler Initialization Error\n");
            exit(-1); 
        }
    }
    // 4. Use this function after creating the clock process to initialize clock
    initClk();

    // To get time use this
    int clk = getClk();
    clk = getClk();
    printf("current time is %d\n", clk);
    
    // TODO Generation Main Loop
    // 5. Create a data structure for processes and provide it with its parameters.
    // 6. Send the information to the scheduler at the appropriate time.

    while (processes.head != NULL)
    {
        if(processes.head->data.arrivalTime > clk)
        {
            sleep(processes.head->data.arrivalTime - clk);  //sleep until the arrival time of the next process comes
            clk = getClk();
            printf("current time is %d\n", clk);
        }
        else
        {
            kill(pid, SIGUSR2);

            while(processes.head != NULL && processes.head->data.arrivalTime <= clk)
            { 
                if(sendProcessToScheduler(&processes.head->data, &msgq_id))
                {
                    removeHeadNodeFromLikedlist(&processes);
                }
            }
            struct Process endProcess;
            endProcess.id = -2;
            if(sendProcessToScheduler(&endProcess, &msgq_id))
            {
                printf("\nSuccess: End slot message was sent to the scheduler\n");
            }
            else
            {
                printf("\nFailure: End slot message was sent to the scheduler\n");
                exit(-1);
            }
        }
    }

    sleep(10);
    struct Process endProcess;
    endProcess.id = -1;
    kill(pid, SIGUSR2);
    if(sendProcessToScheduler(&endProcess, &msgq_id))
    {
        printf("\nSuccess: End message was sent to the scheduler\n");
    }
    else
    {
        printf("\nFailure: End message was sent to the scheduler\n");
        exit(-1);
    }

    int sid, stat_loc;
    while (1)                                       //wait until the scheduler terminates
    {
        sid = wait(&stat_loc);
  	    if(!(stat_loc & 0x00FF))
  	        printf("\nA child with pid %d terminated with exit code %d\n", sid, stat_loc>>8);
        if(sid == pid) break;
    }
    msgctl(msgq_id, IPC_RMID, (struct msqid_ds *)0);

    // 7. Clear clock resources
    destroyClk(true);
}

bool sendProcessToScheduler(struct Process* p, int* msgq_id)
{
    int send_val;
    struct msgbuff message;
    message.P = *p;

    send_val = msgsnd(*msgq_id, &message, sizeof(message.P), !IPC_NOWAIT);
    if (send_val == -1)
    {
        //perror("Errror in send");
        return false;
    }
    return true;
}

void clearResources(int signum)
{
    //TODO Clears all resources in case of interruption
}

bool startsWith(const char *a, const char *b)
{
   if(strncmp(a, b, strlen(b)) == 0) return 1;
   return 0;
}

void chooseAlgorithm(short* algorithmNumber, int* quantum)
{
    printf("Please enter the number of the required scheduling algorithm: \n");
    printf("1- Non-preemptive Highest Priority First (HPF) \n");
    printf("2- Shortest Remaining time Next (SRTN) \n");
    printf("3- Round Robin (RR) \n");

    short algoNum = -1;
    printf("Algorithm Number: ");
    scanf("%hd", &algoNum);
    while(algoNum!=1 && algoNum!=2 && algoNum!=3)
    {
        printf("Please enter a valid algorithm number(1 or 2 or 3): ");
        scanf("%hd", &algoNum);
    }

    *algorithmNumber = algoNum;

    switch (algoNum)
    {
    case 1:
        printf("\nChosen Algorithm: Non-preemptive Highest Priority First (HPF) \n");
        break;
    case 2:
        printf("\nChosen Algorithm: Shortest Remaining time Next (SRTN) \n");
        break;
    case 3:
        printf("\nChosen Algorithm: Round Robin (RR) \n");
        printf("Enter the quantum: ");
        int quan = 0;
        scanf("%d", &quan);
        while(quantum <= 0)
        {
            printf("Please enter a valid quantum: ");
            scanf("%d", &quan);
        }
        *quantum = quan;
        break;
    default:
        break;
    }
    
}

void loadProcess(char str[], struct linkedlist* list)
{
    struct Process newProcess;
    int init_size = strlen(str);
	char delim[] = "\t";
	char *ptr = strtok(str, delim);
    
    int i = 0;
	while(ptr != NULL)
	{

        char *my_string = malloc(MAXCHAR);
        strcpy(my_string, ptr);
        if(i == 0)
        {
            newProcess.id = atoi(my_string);
            i ++;
        }
        else if (i == 1)
        {
            newProcess.arrivalTime = atoi(my_string);
            i ++;
        }
        else if (i == 2)
        {
            newProcess.runTime = atoi(my_string);
            newProcess.remainingTime = atoi(my_string);//Important
            i ++;
        }
        else if (i == 3)
        {
            newProcess.priority = atoi(my_string);
            i ++;
        }

		ptr = strtok(NULL, delim);
        free(my_string);
	}
    newProcess.starttime = -1;
    newProcess.isStarted = 0;
    addNodeToLikedlistEnd(list, newProcess);
}

void readFromFileAndFillList(struct linkedlist* list)
{
    FILE * pFile;
    char str[MAXCHAR];
    pFile = fopen("processes.txt", "r");

    if (pFile == NULL){
        printf("Could not open file");
        return;
    }

    long long index = 0;
    while (fgets(str, MAXCHAR, pFile) != NULL)
    {
        if(!startsWith(str,"#"))
        {
            loadProcess(str,list);
        }
        index ++;
    }
    fclose(pFile);
}

void writeInFile()
{
    
}

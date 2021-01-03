#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "headers.h"
#include "circular_queue.h"
#define null 0
#define MAXCHAR 300


//Initial version of Round Robin scheduling algorithm with no process communication
void readFromFileAndFillList(struct LinkedList* list);
void loadProcess(char str[], struct LinkedList* list);
void addNodeToLikedlistEnd(struct LinkedList* list, struct processData processToAdd);
bool startsWith(const char *a, const char *b);



int main(int argc, char * argv[])
{

    struct LinkedList processes = {NULL, NULL, 0};
    struct LinkedList queue = {NULL, NULL, 0};
    readFromFileAndFillList(&processes);

    
    // 3. Initiate and create the scheduler and clock processes.
    pid_t pid=fork();
    if(pid == 0){
        printf("\nClock Initialization Succes\n"); 
        return execl("./clk.out", "./clk.out", (char*) NULL);
    }
    else if(pid == -1)
    {
        printf("\nClock Initialization Error\n");
        exit(-1); 
    }
    

    // 4. Use this function after creating the clock process to initialize clock
    initClk();
    // To get time use this
    int x = getClk();
    sleep(1);
    printf("current time is %d\n", x);
    // TODO Generation Main Loop
    // 5. Create a data structure for processes and provide it with its parameters.
    // 6. Send the information to the scheduler at the appropriate time.
    struct Node* current_node = processes.head;
    while (1)
    {
        x = getClk();
        
        if(x >= current_node ->processInfo.arrivaltime && current_node != NULL)
        {
            printf("current time is %d\n", x);
            printf("Id: %d\n",current_node->processInfo.id);
            //sleep(1);
            insertToQueue(&queue, current_node->processInfo);
            current_node = current_node->next;

        }

        /* Debug
        if (current_node == NULL)
        {
            current_node = queue.head;
            while (current_node != NULL)
            {
                printf("Id: %d\n",current_node->processInfo.id);
                current_node = current_node->next;
                if(current_node == queue.head)
                    break;
                
            }
            break;
        }
        */
        
    }

    
    


    
}

bool startsWith(const char *a, const char *b)
{
   if(strncmp(a, b, strlen(b)) == 0) return 1;
   return 0;
}

void addNodeToLikedlistEnd(struct LinkedList* list, struct processData processToAdd)
{
    struct Node * nodeToAdd = (struct Node *) malloc(sizeof(struct Node));  //create new node and assign the process to it.
    nodeToAdd->processInfo = processToAdd;
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

void loadProcess(char str[], struct LinkedList* list)
{
    struct processData newProcess;
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
            newProcess.arrivaltime = atoi(my_string);
            i ++;
        }
        else if (i == 2)
        {
            newProcess.runningtime = atoi(my_string);
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

    addNodeToLikedlistEnd(list, newProcess);
}


void readFromFileAndFillList(struct LinkedList* list)
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


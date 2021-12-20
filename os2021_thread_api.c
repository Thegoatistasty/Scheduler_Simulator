#include "os2021_thread_api.h"
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include<time.h>
#define namesize 1000
struct itimerval Signaltimer;
ucontext_t dispatch_context;
ucontext_t timer_context;
ucontext_t finish;
typedef struct queues
{
    int tid;
    char name[namesize];
    char entry_func[100];
    char B_priority[2];
    char C_priority[2];
    int cancel_mode;
    char status[15];
    struct queues* next;
    struct queues* previous;
    int W_event;
    //need timer
    clock_t Q_time;
    clock_t W_time;//in waiting Q
    clock_t counter;
    clock_t waitime;
    ucontext_t Tcontext;
    clock_t expireT;
} Queue;
Queue* Q_H=NULL;//1
Queue* Q_M=NULL;//2
Queue* Q_L=NULL;//3
Queue* waitingQ = NULL;//Event waiting 4
Queue* cancelQ =NULL;//5
Queue* current;//
Queue* time_WQ;// 6
void traverse(Queue* ptr)
{
    while(ptr!=NULL)
    {
        printf("*\t\t%-4d\t%-16s%-8s%-12s%-13s%-15lf%-9lf*\n", ptr->tid,ptr->name,ptr->status,ptr->B_priority, ptr->C_priority, (double)ptr->Q_time/CLOCKS_PER_SEC*100, (double)ptr->W_time/CLOCKS_PER_SEC*100);
        ptr=ptr->next;
    }
}
void extract(Queue* member)
{
    if (member->previous == NULL)
    {
        switch(member->C_priority[0])
        {
        case 'H':
            Q_H = member->next;
            break;
        case 'M':
            Q_M = member->next;
            break;
        case 'L':
            Q_L = member->next;
            break;
        }
        if(member->next!=NULL)
            member->next->previous=NULL;
    }
    else
    {
        member->previous->next = member->next;
        member->next->previous = member->previous;
    }
}
void append(Queue* member, Queue* destination, int k)
{
    if (destination==NULL)
    {
        destination = member;
        member -> previous = member->next=NULL;
        switch (k)
        {
        case 1:
            Q_H = destination;
            break;
        case 2:
            Q_M = destination;
            break;
        case 3:
            Q_L = destination;
            break;
        case 4:
            waitingQ= destination;
            break;
        case 5:
            cancelQ= destination;
            break;
        case 6:
            time_WQ= destination;
            break;
        }
        return;
    }
    else
    {
        Queue* temp = destination;
        while(temp->next!=NULL)
            temp =temp->next;
        temp ->next = member;
        member->previous = temp;
        member-> next =NULL;
    }
}
void finish_F()
{
    switch (current->C_priority[0])
    {
    case 'H':
        current->C_priority[0]='H';
        extract(current);
        append(current, Q_H,1);
        break;
    case 'M':
        current->C_priority[0]='H';
        printf("move %s from M to H\n", current->name);
        extract(current);
        append(current, Q_H,1);
        break;
    case 'L':
        current->C_priority[0]='M';
        extract(current);
        append(current, Q_M,2);
        break;
    }
}
static int id=1;
int OS2021_ThreadCreate(char *job_name, char *p_function, char *priority, int cancel_mode)
{
    Queue* temp = (Queue*) malloc(sizeof(Queue));
    strcpy(temp->name, job_name);
    strcpy(temp->status,"READY");
    strcpy(temp->entry_func,p_function);
    strcpy(temp->B_priority, priority);
    strcpy(temp->C_priority, priority);
    temp->cancel_mode = cancel_mode;
    temp->next =NULL;
    temp->Q_time = temp->W_time =0;
    temp->counter = clock();//in ready Q
    temp->tid = id++;
    temp->previous=NULL;
    switch (priority[0])
    {
    case 'H':
        append(temp, Q_H,1);
        break;
    case 'M':
        append(temp, Q_M,2);
        break;
    case 'L':
        append(temp, Q_L,3);
        break;
    }
    switch (temp->entry_func[8])
    {
    case '1':
        CreateContext(&temp->Tcontext, &finish, &Function1);
        break;
    case '2':
        CreateContext(&temp->Tcontext, &finish, &Function2);
        break;
    case '3':
        CreateContext(&temp->Tcontext, &finish, &Function3);
        break;
    case '4':
        CreateContext(&temp->Tcontext, &finish, &Function4);
        break;
    case '5':
        CreateContext(&temp->Tcontext, &finish, &Function5);
        break;
    default:
        CreateContext(&temp->Tcontext, &finish, &OS2021_DeallocateThreadResource);
    }
    return temp->tid;
}
void OS2021_ThreadCancel(char *job_name)
{
    Queue* temp = Q_H;
    while(temp!=NULL)
    {
        if(!strcmp(temp->name, job_name))
        {
            if(temp->cancel_mode==0)
            {
                //temp=to be cancel
                //temp2 in cancelQ
                extract(temp);
                append(temp,cancelQ,5);
            }
            strcpy(temp->status,"TERMINATED");
        }
        else
            temp=temp->next;
    }
    temp = Q_M;
    while(temp!=NULL)
    {
        if(!strcmp(temp->name, job_name))
        {
            if(temp->cancel_mode==0)
            {
                //temp=to be cancel
                //temp2 in cancelQ
                extract(temp);
                append(temp, cancelQ,5);
            }
            strcpy(temp->status,"TERMINATED");
        }
        else
            temp=temp->next;
    }
    temp = Q_L;
    while(temp!=NULL)
    {
        if(!strcmp(temp->name, job_name))
        {
            if(temp->cancel_mode==0)
            {
                //temp=to be cancel
                //temp2 in cancelQ
                extract(temp);
                append(temp, cancelQ,5);
            }
            strcpy(temp->status,"TERMINATED");
        }
        else
            temp=temp->next;
    }
    temp = waitingQ;
    while(temp!=NULL)
    {
        if(!strcmp(temp->name, job_name))
        {
            if(temp->cancel_mode==0)
            {
                //temp=to be cancel
                //temp2 in cancelQ
                extract(temp);
                append(temp, cancelQ,5);
            }
            strcpy(temp->status,"TERMINATED");
        }
        else
            temp=temp->next;
    }


}

void OS2021_ThreadWaitEvent(int event_id)
{
    current->W_event = event_id;
    current->Q_time = current->Q_time + clock()- current->counter;
    current->counter = clock();
    extract(current);
    append(current, waitingQ,4);
    strcpy(current->status, "WAITING");
    printf("%s wants to wait for event %d\n", current->name, event_id);
    setcontext(&dispatch_context);
}

void OS2021_ThreadSetEvent(int event_id)
{
    Queue* temp = waitingQ;
    if(temp==NULL)
        return;
    else
    {
        while(temp != NULL)
        {
            if(temp->W_event==event_id)
            {
                switch (temp->C_priority[0])
                {
                case 'H':
                    extract(temp);
                    append(temp,Q_H,1);
                    break;
                case 'M':
                    extract(temp);
                    append(temp,Q_M,2);
                    break;
                case 'L':
                    extract(temp);
                    append(temp,Q_L,3);
                    break;
                }
                strcpy(temp->status,"READY");
                temp->W_time = temp->W_time + clock()- temp->counter;
                temp->counter = clock();
                printf("%s changes the status of %s to READY\n", current->name, temp->name);
                return;
            }
            else
                temp=temp->next;
        }
    }

}

void OS2021_ThreadWaitTime(int msec)
{
    current->waitime = clock() + msec* CLOCKS_PER_SEC/100;
    current->counter= clock();
    extract(current);
    append(current, time_WQ,6);
    strcpy(current->status, "WAITING");
    current->Q_time = current->Q_time + clock()- current->counter;
    current->counter = clock();

}

void OS2021_DeallocateThreadResource()
{
    if(cancelQ == NULL)
    {
        return;
    }
    Queue* temp = cancelQ->next;
    while(cancelQ!=NULL)
    {
        free(cancelQ);
        cancelQ=temp;
    }
}

void OS2021_TestCancel()
{
    if(!strcmp(current->status,"TERMINATED"))
    {
        extract(current);
        append(current, cancelQ,5);
    }
}

void CreateContext(ucontext_t *context, ucontext_t *next_context, void *func)
{
    getcontext(context);
    context->uc_stack.ss_sp = malloc(STACK_SIZE);
    context->uc_stack.ss_size = STACK_SIZE;
    context->uc_stack.ss_flags = 0;
    context->uc_link = next_context;
    makecontext(context,(void (*)(void))func,0);
}

void ResetTimer()
{
    Signaltimer.it_value.tv_sec = 0;
    Signaltimer.it_value.tv_usec = 10000;
    if(setitimer(ITIMER_REAL, &Signaltimer, NULL) < 0)
    {
        printf("ERROR SETTING TIME SIGALRM!\n");
    }
}
void counttime()
{
    Queue* temp;
    temp = Q_H;
    while(temp!=NULL)
    {
        temp->Q_time = temp->Q_time+clock() -temp->counter;
        temp->counter = clock();
        temp = temp->next;
    }
    temp = Q_M;
    while(temp!=NULL)
    {
        temp->Q_time = temp->Q_time+clock() -temp->counter;
        temp->counter = clock();
        temp = temp->next;
    }
    temp = Q_L;
    while(temp!=NULL)
    {
        temp->Q_time = temp->Q_time+clock() -temp->counter;
        temp->counter = clock();
        temp = temp->next;
    }
    temp = waitingQ;
    while(temp!=NULL)
    {
        temp->W_time = temp->W_time+clock() -temp->counter;
        temp->counter = clock();
        temp = temp->next;
    }
    temp = time_WQ;
    while(temp!=NULL)
    {
        temp->W_time = temp->W_time+clock() -temp->counter;
        temp->counter = clock();
        temp = temp->next;
    }
}
void alrm(int unused)
{
    //getcontext(&timer_context);
    counttime();
    if(time_WQ!=NULL)
    {
        Queue* temp = time_WQ;
        while(temp!=NULL)
        {
            clock_t c = clock();
            if(temp->waitime <= c)
            {
                temp->W_time = temp->W_time + c - temp->counter;
                temp->counter =clock();
                strcpy(temp->status,"READY");
                //move to ready
                switch (temp->C_priority[0])
                {
                case 'H':
                    extract(current);
                    append(temp, Q_H,1);
                    break;
                case 'M':
                    extract(current);
                    append(temp, Q_M,2);
                case 'L':
                    extract(current);
                    append(temp, Q_L,3);
                    break;
                }
            }
            else
                temp=temp->next;
        }
    }
    if(clock() > current->expireT)
    {
        switch (current->C_priority[0])
        {
        case 'H':
            current->C_priority[0] = 'M';
            extract(current);
            append(current,Q_M,2);
            break;
        case 'M':
            current->C_priority[0] = 'L';
            extract(current);
            append(current,Q_L,3);
        case 'L':
            current->C_priority[0] = 'L';
            extract(current);
            append(current,Q_L,3);
            break;
        }
        current->W_time = current->W_time + clock()- current->counter;
        current->counter = clock();
        swapcontext(&current->Tcontext, &dispatch_context);
    }
    setcontext(&dispatch_context);
}
void printQs();
void Dispatcher()
{
    if(Q_H!=NULL)
    {
        current = Q_H;
        current->expireT = clock()+CLOCKS_PER_SEC*0.1;
        setcontext(&current->Tcontext);
    }
    else if(Q_M!=NULL)
    {
        current = Q_M;
        current->expireT = clock()+CLOCKS_PER_SEC*0.2;
        setcontext(&current->Tcontext);
    }
    else
    {
        current = Q_L;
        current->expireT = clock()+CLOCKS_PER_SEC*0.3;
        setcontext(&current->Tcontext);
        //swapcontext(&dispatch_context,&current->Tcontext);
    }

}
void parse(char *path)
{
    FILE *file;
    file = fopen(path,"r");
    char name[namesize];
    char entry_func[20];
    char priority[20];
    char cancel_mode[20];
    fscanf(file, "%[^[]%*c%*c%*c%*c", name);
    char comma = ',';
    while(1)
    {
        if(comma !=',')
            break;
        fscanf(file, "%*[^:]%*c%*c%*c%[^\"]%*c", name);
        fscanf(file, "%*[^:]%*c%*c%*c%[^\"]%*c", entry_func);
        fscanf(file, "%*[^:]%*c%*c%*c%[^\"]%*c", priority);
        fscanf(file, "%*[^:]%*c%*c%*c%[^\"]%*[^}]%*c", cancel_mode);
        OS2021_ThreadCreate(name,entry_func,priority,cancel_mode[0]-'0');
        fscanf(file,"%c",&comma);
    }
    fclose(file);
}
void printQs()
{
    printf("******************************************************\n");
    printf("*\t\tTID\tName\t\tState   B_Priority  C_Priority\t Q_Time\t\tW_Time\t *\n");
    traverse(Q_H);
    traverse(Q_M);
    traverse(Q_L);
    traverse(waitingQ);
    traverse(cancelQ);
}
void StartSchedulingSimulation()
{
    OS2021_ThreadCreate("Reclaimer","OS2021_DeallocateThreadResource","L",1);
    char *path = "./init_threads.json";
    parse(path);
    /*Set Timer*/
    Signaltimer.it_interval.tv_usec = 10000;
    Signaltimer.it_interval.tv_sec = 0;
    ResetTimer();
    //setcontext(&Q_M->Tcontext);
    signal(SIGTSTP, printQs);
    signal(SIGALRM, alrm);
    /*Create Context*/
    CreateContext(&dispatch_context, &timer_context, &Dispatcher);
    CreateContext(&finish, &timer_context, &finish_F);
    CreateContext(&timer_context, NULL, &alrm);
    //while(1);
    setcontext(&dispatch_context);
    //setcontext(&Q_M->Tcontext);
    while(1);
}
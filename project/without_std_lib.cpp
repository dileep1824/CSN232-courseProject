
//compile with -lpthread [-lrt] flags
#include <iostream>
#include <pthread.h>
#include <sys/types.h>
//#include <semaphore.h>
#include <syscall.h>
#include <unistd.h>
#include <string>

using namespace std;

int lock,r_sem,w_sem,ex_lock;
//sem_t lock,r_sem,w_sem,ex_lock;   //semaphore locks
int nw_waiting=0,nr_waiting=0;    //counters for waiting processes
int nw_active=0, nr_active=0;     //counters for active processes 

void* reader(void* args);         //reader function
void* writer(void* args);         //writer function

void sem_wait(int* a)
{
    while(*a == 0);
    *a = *a - 1;
}

void sem_post(int* a)
{
    *a = *a + 1;
}

int main()
{
    //Please use Posix based OS to compile this !!
    #ifdef WIN32
      cout<<"Please use Posix type os like Ubuntu,Linux,etc"<<endl;
      exit(1);
    #endif  

    //Initiating the semaphores
    /*
    sem_init(&lock,0,1);    //lock = 1
    sem_init(&ex_lock,0,1); //ex_lock = 1 -- is used to stop smudging the stdout when multiple readers are active
    sem_init(&r_sem,0,0);   //r_sem = 0
    sem_init(&w_sem,0,0);   //w_sem = 0
    */
    lock=1;
    ex_lock=1;
    r_sem=0;
    w_sem=0;

    //These set of threads are just for testing. Create some sequence as you wish to test 
    string str;
    cout<<"********************************************************************************************************"<<endl;
    cout<<"*Note:1.Please donot mind if the output is smudged for inputs for which multiple readers are active    *"<<endl;
    cout<<"*       The STDOUT becomes messy because multiple threads are accessing same screen.It would not cause *"<<endl;
    cout<<"*       any data inconsistency as long as only Reading is involved                                     *"<<endl;
    cout<<"*     2.And the animation in Critical section is placed instead of actual reading and writing          *"<<endl;
    cout<<"********************************************************************************************************"<<endl;
    cout<<endl<<"Enter a string containing only \"R\" or \"W\" to denote sequence of threads"<<endl;
    cout<<"R stands for Readers"<<endl<<"W stands for Writers"<<endl;
    cout<<"Example: RWRW,WWWW,RRR,etc"<<endl<<endl;
    cin>>str;

    int n=str.length();
    pthread_t t[n];      //array of thread IDs
    
    //creating threads
    for(int i=0;i<n;i++)
    {
        if(str[i] == 'R' || str[i] == 'r')
        {
            pthread_create(&t[i],NULL,reader,NULL);
        } 
        else if(str[i] == 'W' || str[i] == 'w')
        {
            pthread_create(&t[i],NULL,writer,NULL);
        }
        else
        {
            cout<<"Make sure you typed the correct characters"<<endl;
            exit(1);
        }

    }

    //Synchronous threading -- parent is waiting for all children to join
    for(int i=0;i<n;i++)
    {
        pthread_join(t[i],NULL);
    }

    return 0;
}

//Implemented according to the algorithm in report document attached
void* reader(void* args)         
{
    do
    {
        //entry section
        sem_wait(&lock);
        if((nw_waiting + nw_active) == 0)
        {
            nr_active++;
            sem_post(&r_sem);
        }
        else
        {
            nr_waiting++;
        }
        sem_post(&lock);
        sem_wait(&r_sem);

        //critical Section
        sem_wait(&ex_lock);
        pid_t id = syscall(SYS_gettid);
        cout<<"Reader Thread with id "<<id<<" entered critical section"<<endl<<endl;
        sem_post(&ex_lock);
        char str[]="Reading data structure";
        sleep(1);
        cout<<str<<" -"<<flush;
        for(int i=0;i<2;i++)
        {
            sleep(1);
            cout<<"\b\\"<<flush;
            sleep(1);
            cout<<"\b|"<<flush;
            sleep(1);
            cout<<"\b/"<<flush;
            sleep(1);
            cout<<"\b-"<<flush;
        }
        cout<<'\r'<<endl<<endl;
        //sleep(1);
        sem_wait(&ex_lock);
        cout<<"Reader Thread with id "<<id<<" completed reading"<<endl;
        sem_post(&ex_lock);

        //exit section
        sem_wait(&lock);
        nr_active--;
        if((nr_active == 0) && (nw_waiting > 0))
        {
            sem_post(&w_sem);
            nw_active++;
            nw_waiting--;
        }
        sem_post(&lock);

        //remainder section
        //this code doesnot have any remainder section

    } while (false);            //false can be replaced with true for multiple iterations
    
    pthread_exit(0);
}

//Implemented according to the algorithm in report document attached
void* writer(void* args)
{
    do
    {
        //entry section
        sem_wait(&lock);
        if((nr_waiting + nr_active) == 0 && nw_active == 0)
        {
            nw_active++;
            sem_post(&w_sem);
        }
        else
        {
            nw_waiting++;
        }
        sem_post(&lock);
        sem_wait(&w_sem);

        //critical section
        cout<<"*-----------------------------------------------------------*"<<endl<<endl;
        pid_t id = syscall(SYS_gettid);
        cout<<"Writer Thread with id "<<id<<" entered critical section"<<endl<<endl;
        char str[]="Writing to data structure";
        cout<<str<<" -"<<flush;
        for(int i=0;i<2;i++)
        {
            sleep(1);
            cout<<"\b\\"<<flush;
            sleep(1);
            cout<<"\b|"<<flush;
            sleep(1);
            cout<<"\b/"<<flush;
            sleep(1);
            cout<<"\b-"<<flush;
        }
        cout<<endl<<endl;
        cout<<"Writer Thread with id "<<id<<" completed Writing"<<endl;
        cout<<"*-----------------------------------------------------------*"<<endl;

        //exit section
        sem_wait(&lock);
        nw_active--;
        if((nw_active == 0) && (nr_waiting > 0))
        {
            while(nr_waiting > 0)
            {
                sem_post(&r_sem);
                nr_waiting--;
                nr_active++;
            }
        }
        else if(nw_waiting > 0)
        {
            sem_post(&w_sem);
            nw_waiting--;
            nw_active++;
        }
        sem_post(&lock);
    } while (false);              //false can be replaced with true for multiple iterations
    
    pthread_exit(0);
}


//Note that this implementation is starve free for any sequence of Readers and Writers
//Because Reader or Writer becomes active only if there are no opposing threads waiting
//That means a thread which arrived after its opposing thread is kept waiting till its opposing thread ended.
//This ensures that each thread gets an inherent property similar to priority which increases as time passes(aging effect)
//Hence all threads are executed.
#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include<string.h>
typedef struct cust
{
    char *name;
    int arrivalTime;
    int mechanicTime;
    int oilTime;
    pthread_t pid;
    pthread_cond_t cid;
    pthread_cond_t oid;
    struct cust *next;
    struct cust *link;
    struct cust *oilLink;
}cust;
/*********************** FUNCTION DECLARATIONS ***************************************/
static void *allocateMsg(size_t,char *);
static void *reallocateMsg(void *,size_t,char *);
char *readLine(FILE*);
void getParkingLotSize(char *line);
void *customers(void *arg);
void *mechanicService(void *args);
void *oilService(void *args);
cust *mqueue(cust *temp);
cust *oqueue(cust *temp);
cust *mdequeue();
cust *odequeue();
/*********************** GLAOBAL VARIBALES ******************************************/
cust *head=NULL;
cust *tail=NULL;
cust *front=NULL;
cust *rear=NULL;
cust *ofront=NULL;
cust *orear=NULL;
cust *p=NULL;
char *mCust;
char *oCust;
int mechanicLot;
int mechanicRoom;
int oilLot;
int firstToMech=0;
int firstToOil=0;
int mechanicStatus=1;
int oilStatus=1;
int wait=0;
/*********************** Conditional variables and locks ****************************/
pthread_mutex_t mechanicLock;
pthread_mutex_t oilLock;
pthread_t mThread;
pthread_t oThread;
pthread_cond_t firstm;
pthread_cond_t firsto;;
pthread_cond_t me;
pthread_cond_t oilme;
int main()
{
    int totalTime=0;
    int line=0;
    FILE *fp;
    char *s;
    char *token;
    pthread_cond_init(&firstm,NULL);
    pthread_cond_init(&me, NULL);
    pthread_cond_init(&oilme,NULL);
    pthread_cond_init(&firsto,NULL);
    pthread_mutex_init(&mechanicLock, NULL);
    pthread_mutex_init(&oilLock, NULL);
    pthread_create(&mThread, NULL, mechanicService, NULL);
    pthread_create(&oThread, NULL, oilService, NULL);
    fp=fopen("jobcard","r");
    while(!feof(fp))
    {
        line++;
        s=readLine(fp);
        if(s==NULL)
        break;
        if(line==1)
        getParkingLotSize(s);
        if(line!=1)
        {
           cust *temp=malloc(sizeof(cust));                     
           char *token;
           token=strtok(s,",");
           temp->name=token;
           token=strtok(NULL,",");
           temp->arrivalTime=atoi(token);
           token=strtok(NULL,",");
           temp->mechanicTime=atoi(token);
           token=strtok(NULL,",");
           temp->oilTime=atoi(token);
           temp->next=NULL;
           if(temp->arrivalTime>totalTime)
           {
              sleep(temp->arrivalTime-totalTime);               //making thread sleep till totalTime equals to arrivalTime
              totalTime=temp->arrivalTime;
           }
           if(totalTime==temp->arrivalTime)
           {
              if(head==NULL)
              {
                  head=temp;
                  tail=temp;
                  pthread_create(&head->pid,NULL, customers, (void*)head);
              }
              else
              {
                   tail->next=temp;
                   tail=tail->next;
                   pthread_create(&tail->pid, NULL, customers, (void *)tail);
              }
           }
        }
    }
    p=head;
    cust *t;                                                                             
    while(p!=NULL)
    {
        t=p;
        pthread_join(p->pid,NULL);       //waiting for threads to join
        p=p->next;
        free(t);                        //freeing up memory here
    }
    printf("\n");
    pthread_cancel(mThread);
    pthread_cancel(oThread);
    printf(".......All done.....\n");
    return 0;
}



/**************************************************************************************************************************************
                               DEFINITIONS FOR CUSTOMERS
***************************************************************************************************************************************
WORKING: 
First customer will wait for mechanic.
Customer will wait in the mechanic lot if mechanic is busy with other customer, if there is space in mechanic lot.
Else customer will leave car maintenance shop.
After being serviced by mechanic customer will notify next customer in the queue if any waiting for mechanic.
If so then signal next customer for mechanic.
If customer need oil repair then it will notify the oilRepair and wait for oilRepair or their chance.
Else they will leave the car maintenance shop.
***************************************************************************************************************************************/


void *customers(void *args)
{
    int service=0;
    cust *this=malloc(sizeof(cust));
    cust *mQtoken;
    cust *mDtoken;
    cust *oQtoken;
    cust *oDtoken;
    this=(cust *)args;
    printf("Customer %s arrival.\n", this->name);
    pthread_mutex_lock(&mechanicLock);
    firstToMech++;
    if(mechanicRoom)
    {
        mechanicRoom--;
        mQtoken=mqueue(this);
        if(firstToMech==1 && mechanicStatus==1)
        {
            pthread_cond_signal(&firstm);
            pthread_cond_wait(&me, &mechanicLock);
            mDtoken=mdequeue();
            mechanicRoom++;
            mechanicStatus=0;
            pthread_mutex_unlock(&mechanicLock);
            sleep(this->mechanicTime);
            printf("Customer %s is being serviced by mechanic for %d seconds.\n",this->name,this->mechanicTime);
            service=1;
        }
        else
        {
            printf("Customer %s waiting for mechanic.\n", this->name);
            pthread_cond_wait(&mQtoken->cid,&mechanicLock);
            mechanicStatus=0;
            pthread_mutex_unlock(&mechanicLock);
            printf("Customer %s is no longer waiting for mechanic; signalled by customer %s.\n", this->name, mCust);
            sleep(this->mechanicTime);
            printf("Customer %s is being serviced by mechanic for %d sec.\n", this->name, this->mechanicTime);
            service=1;
        }
        if(service==1) 
        {
                pthread_mutex_lock(&mechanicLock);
                mechanicStatus=1;
                if(mechanicRoom!=mechanicLot)
                {
                   mCust=this->name;
                   mDtoken=mdequeue();
                   mechanicRoom++; 
                   printf("Customer %s notifying customer %s that they are next for mechanic.\n", this->name, mDtoken->name);
                   pthread_cond_signal(&mDtoken->cid);  
                }
                if(this->oilTime)
                {
                    printf("Customer %s done with mechanic.\n", this->name);
                    service=2;
                }
                else
                {
                    printf("Customer %s is leaving shop.\n",this->name);
                }   
        }
        if(mechanicRoom==mechanicLot)
            firstToMech=0;
    }  
    else
    {
        printf("Customer %s leaves busy car maintenance shop.\n", this->name);
    }
    pthread_mutex_unlock(&mechanicLock);
    if(service==2)
    {
        pthread_mutex_lock(&oilLock);
        firstToOil++;
            oQtoken=oqueue(this);
            wait++;
            if(firstToOil==1 && oilStatus==1)
            {
                pthread_cond_signal(&firsto);
                pthread_cond_wait(&oilme, &oilLock);
                oDtoken=odequeue();
                wait--;
                oilStatus=0;
                pthread_mutex_unlock(&oilLock);
                sleep(this->oilTime);
                service=1;
            }
            else
            {
                printf("Customer %s waiting for oil tech.\n", this->name);
                pthread_cond_wait(&oQtoken->oid,&oilLock);
                oilStatus=0;
                pthread_mutex_unlock(&oilLock);
                sleep(this->oilTime);
                service=1;
            }
            if(service==1) 
            {
                    pthread_mutex_lock(&oilLock);
                    oilStatus=1;
                    if(wait)
                    {
                       oCust=this->name;
                       oDtoken=odequeue();
                       wait--;
                       pthread_cond_signal(&oDtoken->oid);
                    }  
                    if(wait==0)
                    {
                        firstToOil=0; 
                    }  
                    printf("Customer %s is done with oil tech.\n", this->name);
                    printf("Customer %s is leaving shop.\n", this->name);
            }   
        pthread_mutex_unlock(&oilLock);
    }
    pthread_exit(NULL);
}


/**************************************************************************************************************************************
                               DEFINITION FOR MECHANIC.
***************************************************************************************************************************************
WORKING: 
It will signal the first customer who request for mechanic service. 
It keeps track on the same till all the entries in the file are read.
***************************************************************************************************************************************/
void *mechanicService (void *args)
{
    while(1)
    {
      pthread_cond_wait(&firstm,&mechanicLock);
      if(mechanicRoom !=mechanicLot)
      {
         pthread_cond_signal(&me);
      }
    }
} 

/**************************************************************************************************************************************
                               DEFINITION FOR OIL REPAIR.
***************************************************************************************************************************************
WORKING: 
It will signal the first customer who request for oil repair.
It keeps track on the same till all the entries in the file are read.
***************************************************************************************************************************************/

void *oilService (void *args)
{
    while(1)
    {
      pthread_cond_wait(&firsto,&oilLock);
      if(firstToOil==1)
      {
         pthread_cond_signal(&oilme);
      }
    }
} 

/**************************** THIS WILL KEEP TRACK ON CUSTOMERS WAITING FOR MECHANIC ************************************************/

cust *mqueue(cust *temp)
{
    temp->link=NULL;
    cust *p;
    pthread_cond_init(&temp->cid,NULL);
    if(front==NULL)
    {
        front=temp;
    }
    else
    {
        rear->link=temp;
    }
    rear=temp;
    return rear;
}
/**************************** THIS WILL DEQUEUE CUSTOMERS WAITING IN MECHANIC LOT ******************************************************/
cust *mdequeue()
{
    cust *p;
    cust *temp;
    temp=front;
    front=front->link;
    return temp;
}
/**************************** THIS WILL KEEP TRACK ON CUSTOMERS WAITING FOR OIL TECH ************************************************/
cust *oqueue(cust *temp)
{
    temp->oilLink=NULL;
    pthread_cond_init(&temp->oid,NULL);
    if(ofront==NULL)
    {
        ofront=temp;
    }
    else
    {
        orear->oilLink=temp;
    }
    orear=temp;
    return orear;
}
/**************************** THIS WILL DEQUEUE CUSTOMERS WAITING IN THE OIL REPAIR QUEUE ***********************************************/
cust *odequeue()
{
    cust *p;
    cust *temp;
    temp=ofront;
    ofront=ofront->oilLink;
    return temp;
}
/********** THIS WILL SET THE MECHANIC LOT SIZE AND OIL LOT SIZE (not of use as unlimited customers can wait for oil repair) ************/
void getParkingLotSize(char *line)
{
    char *token;
   token=strtok(line,",");
    mechanicLot=atoi(token);
   token = strtok(NULL,",");
    oilLot=atoi(token);
    mechanicRoom=mechanicLot; 
}
/**************************FUNCTIONS TO READ TEXT FILE********************************************************************************************
                                      
CREDITS : DR. JOHN LUSTH 
****************************************************************************************************************************************************/
void * allocateMsg(size_t size,char *where)
{
   char *s;
   s = malloc(size);
   if (s == 0)
  {
    fprintf(stderr,"%s: could not allocate string, out of memory\n",
    where);
   exit(3);
 }

 return s;
}

static void * reallocateMsg(void *s,size_t size,char *where)
{
   char *t;
    t = realloc(s,size);
    if (t == 0)
    {
       fprintf(stderr,"%s: could not reallocate string, out of memory\n",
       where);
     exit(3);
    }

 return t;
}

char *readLine(FILE *fp)
{
 int ch,index;
 char *buffer;
 int size = 512;
 
 ch = fgetc(fp);
 if (ch == EOF) return 0;

 buffer = allocateMsg(size,"readLine");

 index = 0;
 while (ch != '\n')
     {
     if (ch == EOF) break;
     if (index > size - 2)
         {
         ++size;
         buffer = reallocateMsg(buffer,size,"readLine");
         }
     buffer[index] = ch;
     ++index;
     ch = fgetc(fp);
     }


 if (index > 0)              //there is something in the buffer
     clearerr(fp);           //so force the read to be good

 buffer[index] = '\0';

 return buffer;
}

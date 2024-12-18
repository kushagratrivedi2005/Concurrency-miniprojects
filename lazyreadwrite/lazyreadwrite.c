#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <errno.h>


#define MAX_FILES 100
#define MAX_USERS 100
#define MAX_QUEUE_SIZE 1000
#define MAX_REQUESTS 1000


// Colors for output
#define YELLOW "\033[1;33m"
#define PINK "\033[1;35m"
#define WHITE "\033[1;37m"
#define GREEN "\033[1;32m"
#define RED "\033[0;31m"
#define RESET "\033[0m"

time_t start;
// User request structure
typedef struct
{
    int userid;
    int fileid;
    char operation_type[10];
    int requesttime;
} userrequest;

userrequest requests[MAX_REQUESTS];

typedef struct
{
    bool isexisting;
    int numberofreaders;
    bool is_writing;
    int numberofusers;
    int requestswaiting;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} filestate;

int R, W, D;
int numberoffile, maxusers, waittime;
userrequest requestqueue[MAX_QUEUE_SIZE];
int queuefront = 0, queuerear = 0;
filestate filestates[MAX_FILES];
pthread_mutex_t queuemutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queuecond = PTHREAD_COND_INITIALIZER;
bool isend = false;

void enqueue(userrequest req)
{
    pthread_mutex_lock(&queuemutex);
    requestqueue[queuerear] = req;
    queuerear = (queuerear + 1) % MAX_QUEUE_SIZE;
    pthread_cond_signal(&queuecond);
    pthread_mutex_unlock(&queuemutex);
}

userrequest dequeue()
{
    pthread_mutex_lock(&queuemutex);
    while ((queuefront == queuerear) && !isend)
    {
        pthread_cond_wait(&queuecond, &queuemutex);
    }
    userrequest req;
    if (!isend && !(queuefront == queuerear))
    {
        req = requestqueue[queuefront];
        queuefront = (queuefront + 1) % MAX_QUEUE_SIZE;
        pthread_mutex_unlock(&queuemutex);
        return req;
    }
    pthread_mutex_unlock(&queuemutex);
    return (userrequest){-1,-1,-1,-1};
}

bool readfunction(userrequest req)
{
    filestate *state = &filestates[req.fileid];
    while ((int)(time(NULL) - start) < req.requesttime + 1)
    {
        usleep(100000); 
    }
    pthread_mutex_lock(&state->mutex);
    state->requestswaiting++;
    struct timespec timeout;
    clock_gettime(CLOCK_REALTIME, &timeout);
    timeout.tv_sec += waittime;
    while (state->numberofusers >= maxusers)
    {
        if ((pthread_cond_timedwait(&state->cond, &state->mutex, &timeout) == ETIMEDOUT) && ((int)(time(NULL) - start) - req.requesttime > waittime)){
                printf(RED "User %d canceled the request due to no response at %ld seconds [RED]\n" RESET,
                       req.userid, start);
                state->requestswaiting--;
                pthread_mutex_unlock(&state->mutex);
                return false;
        }
    }
    if ((int)(time(NULL) - start) - req.requesttime > waittime)
    {
        printf(RED "User %d canceled the request due to no response at %ld seconds [RED]\n" RESET,
               req.userid, start);
        state->requestswaiting--;
        pthread_mutex_unlock(&state->mutex);
        return false;
    }
    state->requestswaiting--;
    if (req.fileid >= numberoffile || !filestates[req.fileid].isexisting)
    {
        printf(WHITE "LAZY has declined the request of User %d at %d seconds because an invalid/deleted file was requested. [WHITE]\n" RESET,
               req.userid, (int)(time(NULL) - start));
        pthread_mutex_unlock(&state->mutex);
        return false;
    }
    printf(PINK "LAZY has taken up the request of User %d at %d seconds [PINK]\n" RESET,req.userid, (int)(time(NULL) - start));
    state->numberofusers++;
    state->numberofreaders++;
    pthread_mutex_unlock(&state->mutex);
    sleep(R);
    pthread_mutex_lock(&state->mutex);
    printf(GREEN "The request for User %d was completed at %d seconds [GREEN]\n" RESET,req.userid, (int)(time(NULL) - start));
    state->numberofusers--;
    state->numberofreaders--;
     for (int i = 0; i < state->requestswaiting; i++) {
        pthread_cond_signal(&state->cond);
        usleep(1000);
    }
    pthread_mutex_unlock(&state->mutex);
    return true;
}

bool writefunction(userrequest req)
{
    filestate *state = &filestates[req.fileid];
    while ((int)(time(NULL) - start) < req.requesttime + 1)
    {
        usleep(100000); 
    }
    pthread_mutex_lock(&state->mutex);
    struct timespec timeout;
    clock_gettime(CLOCK_REALTIME, &timeout);
    timeout.tv_sec += waittime;
    state->requestswaiting++;
    while (state->numberofusers >= maxusers || state->is_writing)
    {
            if ((pthread_cond_timedwait(&state->cond, &state->mutex, &timeout) == ETIMEDOUT)&&((int)(time(NULL) - start) - req.requesttime > waittime))
            {
                printf(RED "User %d canceled the request due to no response at %d seconds [RED]\n" RESET,
                       req.userid, (int)(time(NULL) - start));
                pthread_mutex_unlock(&state->mutex);
                state->requestswaiting--;
                return false;
            }
    }
            if ((int)(time(NULL) - start) - req.requesttime > waittime)
            {
                printf(RED "User %d canceled the request due to no response at %d seconds [RED]\n" RESET,
                       req.userid,(int)(time(NULL) - start));
                pthread_mutex_unlock(&state->mutex);
                state->requestswaiting--;
                return false;
            }
    state->requestswaiting--;
    if (req.fileid >= numberoffile || !filestates[req.fileid].isexisting)
    {
        printf(WHITE "LAZY has declined the request of User %d at %d seconds because an invalid/deleted file was requested. [WHITE]\n" RESET,
               req.userid, (int)(time(NULL) - start));
        pthread_mutex_unlock(&state->mutex);
        return false;
    }
    printf(PINK "LAZY has taken up the request of User %d to WRITE at %d seconds [PINK]\n" RESET,
           req.userid, (int)(time(NULL) - start));

    state->numberofusers++;
    state->is_writing = true;
    pthread_mutex_unlock(&state->mutex);
    sleep(W);
    pthread_mutex_lock(&state->mutex);
    printf(GREEN "The request for User %d was completed at %d seconds [GREEN]\n" RESET,
           req.userid, (int)(time(NULL) - start));
    state->numberofusers--;
    state->is_writing = false;
     for (int i = 0; i < state->requestswaiting; i++) {
        pthread_cond_signal(&state->cond);
        usleep(1000);
    }
    pthread_mutex_unlock(&state->mutex);
    return true;
}

bool deletefunction(userrequest req)
{
    filestate *state = &filestates[req.fileid];
    while ((int)(time(NULL) - start) < req.requesttime + 1)
    {
        usleep(100000); 
    }
    pthread_mutex_lock(&state->mutex);
    struct timespec timeout;
    clock_gettime(CLOCK_REALTIME, &timeout);
    timeout.tv_sec += waittime;
    state->requestswaiting++;
    while (state->numberofusers >= maxusers || state->is_writing || state->numberofreaders > 0 )
    {
         if (!state->isexisting)
        {
            printf(WHITE "LAZY has declined the request of User %d at %d seconds because an invalid/deleted file was requested. [WHITE]\n" RESET,
                req.userid, (int)(time(NULL) - start));
            pthread_mutex_unlock(&state->mutex);
            state->requestswaiting--;
            return false;
        }
            if ((pthread_cond_timedwait(&state->cond, &state->mutex, &timeout) == ETIMEDOUT)&&(int)(time(NULL) - start) - req.requesttime > waittime)
            {
                printf(RED "User %d canceled the request due to no response at %d seconds [RED]\n" RESET,
                       req.userid, (int)(time(NULL) - start));
                pthread_mutex_unlock(&state->mutex);
                state->requestswaiting--;
                return false;
            }
    }
    if ((int)(time(NULL) - start) - req.requesttime > waittime)
    {
        printf(RED "User %d canceled the request due to no response at %d seconds [RED]\n" RESET,
               req.userid, (int)(time(NULL) - start));
        pthread_mutex_unlock(&state->mutex);
        state->requestswaiting--;
        return false;
    }
    state->requestswaiting--;
    if (!state->isexisting)
    {
        printf(WHITE "LAZY has declined the request of User %d at %d seconds because an invalid/deleted file was requested. [WHITE]\n" RESET,
               req.userid, (int)(time(NULL) - start));
        pthread_mutex_unlock(&state->mutex);
        return false;
    }
    printf(PINK "LAZY has taken up the request of User %d at %d seconds [PINK]\n" RESET,
           req.userid, (int)(time(NULL) - start));

    state->isexisting = false;
    pthread_mutex_unlock(&state->mutex);
    sleep(D);
    pthread_mutex_lock(&state->mutex);
    printf(GREEN "The request for User %d was completed at %d seconds [GREEN]\n" RESET,
           req.userid, (int)(time(NULL) - start));
    state->numberofusers--;
    pthread_cond_broadcast(&state->cond);
    pthread_mutex_unlock(&state->mutex);
    return true;
}




void *process_request(void *arg)
{
    userrequest req = dequeue();
    if (req.userid == -1)
        return NULL;
    while ((int)(time(NULL) - start) < req.requesttime)
        usleep(100000);
    if (strcmp(req.operation_type, "READ") == 0) {
        printf(YELLOW "User %d has made a request for performing READ on file %d at %d seconds [YELLOW]\n" RESET,
            req.userid, req.fileid + 1, req.requesttime);
        readfunction(req);
    } else if (strcmp(req.operation_type, "WRITE") == 0) {
        printf(YELLOW "User %d has made a request for performing WRITE on file %d at %d seconds [YELLOW]\n" RESET,
            req.userid, req.fileid + 1, req.requesttime);
        writefunction(req);
    } else if (strcmp(req.operation_type, "DELETE") == 0) {
        printf(YELLOW "User %d has made a request for performing DELETE on file %d at %d seconds [YELLOW]\n" RESET,
            req.userid, req.fileid + 1, req.requesttime);
        deletefunction(req);
    }
   return NULL;
}


int compare_requests(const void* a, const void* b) {
    userrequest* req1 = (userrequest*)a;
    userrequest* req2 = (userrequest*)b;
    int time_diff = req1->requesttime - req2->requesttime;
    if (time_diff != 0) {
        return time_diff;
    }

    // Now compare based on operation_type priority (READ < WRITE < DELETE)
    if (strcmp(req1->operation_type, "READ") == 0 && 
        (strcmp(req2->operation_type, "WRITE") == 0 || strcmp(req2->operation_type, "DELETE") == 0)) {
        return -1; // READ comes before WRITE and DELETE
    } 
    if (strcmp(req1->operation_type, "WRITE") == 0 && strcmp(req2->operation_type, "DELETE") == 0) {
        return -1; // WRITE comes before DELETE
    }
    if(strcmp(req1->operation_type,req2->operation_type)==0){return 0;}
    return 1; // Otherwise, req1 comes after req2
}


int main()
{
   int request_count = 0;
    scanf("%d %d %d", &R, &W, &D);
    scanf("%d %d %d", &numberoffile, &maxusers, &waittime);
    waittime--;
    char input[20];int userid, fileid, requesttime;char oper[100];
    while (true) {
        scanf("%s", input);
        if (strcmp(input, "STOP") == 0)
            break;
        userid = atoi(input);
        scanf("%d %s %d", &fileid, oper, &requesttime);
        requests[request_count++] = (userrequest){
            userid, 
            fileid - 1, 
            "",  // Empty string initialization
            requesttime
        };
strcpy(requests[request_count - 1].operation_type, oper);  // Copy oper to the operation_type field
    }
    qsort(requests, request_count, sizeof(userrequest), compare_requests);
    for (int i = 0; i < request_count; i++) {
        enqueue(requests[i]);
    }
    for (int i = 0; i < numberoffile; i++)
    {
        filestates[i].numberofusers = 0;
        filestates[i].isexisting = true;
        filestates[i].is_writing = false;
        filestates[i].numberofreaders = 0;
        filestates[i].requestswaiting = 0;
        pthread_mutex_init(&filestates[i].mutex, NULL);
    }
    time(&start);
    printf(WHITE "LAZY has woken up!\n" RESET);
     pthread_t request_threads[request_count];
    for (int i = 0; i < request_count; ++i) {
        int status = pthread_create(&request_threads[i], NULL, process_request, NULL);
        if (status != 0) {
            fprintf(stderr, "Error creating thread for request %d\n", i);
        }
        usleep(500);}
    for (int i = 0; i < request_count; ++i) {
        if (pthread_join(request_threads[i], NULL) != 0) {
            fprintf(stderr, "Error joining thread for request %d\n", i);
        }}
    isend = true;
    printf(WHITE "LAZY has no more pending requests and is going back to sleep!\n" RESET);
    return 0;
}

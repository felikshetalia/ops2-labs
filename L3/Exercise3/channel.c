
#include "channel.h"
#include "macros.h"

#include <errno.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <semaphore.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

pthread_mutexattr_t init_mutex(){
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutexattr_setrobust(&attr, PTHREAD_MUTEX_ROBUST);
    //pthread_mutex_init(&obj->data_mtx, &attr);
    return attr;
}

void mutex_lock_handler(pthread_mutex_t* mutex)
{
    int error;
    if ((error = pthread_mutex_lock(mutex)) != 0)
    {
        if (error == EOWNERDEAD)
        {
            pthread_mutex_consistent(mutex);
        }
        else
        {
            ERR("pthread_mutex_lock");
        }
    }
}

channel_t* channel_open(const char* path) {

    // Use sem_name to initialize semaphore accompanied by channel
    char sem_name[PATH_MAX] = "sem_";
    strncpy(sem_name + 4, path, PATH_MAX - 5);
    sem_name[PATH_MAX-1] = 0;

    // Implement stage 1 here
    //UNUSED(path);
    sem_t *sm;
    int init_flag = 1;
    if((sm = sem_open(sem_name, O_CREAT, 0666, 1)) == SEM_FAILED){
        ERR("sem_open");
    }
    sem_wait(sm);
    // protect the creation of the shared memory
    int fd = shm_open(path, O_CREAT | O_EXCL | O_RDWR, 0600);
    if(fd < 0){
        if(errno == EEXIST){
            init_flag = 0;
            if((fd = shm_open(path, O_RDWR, 0600)) < 0){
                sem_post(sm);
                ERR("shm_open");
            }
        }
        else
            ERR("shm_open");
    } else {
        if(ftruncate(fd, sizeof(channel_t)) < 0){
            shm_unlink(path);
            sem_post(sm);
            ERR("ftruncate");
        }
    }

    channel_t * data;
    if(MAP_FAILED == (data = (channel_t*)mmap(NULL, sizeof(channel_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0))){
        if(init_flag){
            shm_unlink(path);
            sem_post(sm);
            ERR("mmap");
        }
        else{
            sem_post(sm);
            ERR("mmap");
        }
    }

    close(fd);

    if(data->status == CHANNEL_UNINITIALIZED){
        pthread_mutexattr_t attr = init_mutex(data);
        pthread_mutex_init(&data->data_mtx, &attr);
        if(errno){
            shm_unlink(path);
            sem_post(sm);
            ERR("init_mutex");
        }
        data->status = CHANNEL_EMPTY;
    }

    int ret;
    if((ret = pthread_cond_init(&data->producer_cv, NULL)) != 0){
        ERR("cond_init");

    }
    if((ret = pthread_cond_init(&data->consumer_cv, NULL)) != 0){
        ERR("cond_init");

    }
    sem_post(sm);
    sem_close(sm);
    return data;
}

void channel_close(channel_t* channel) {
    // Implement stage 1 here
    mutex_lock_handler(&channel->data_mtx);
    pthread_mutex_destroy(&channel->data_mtx);
    pthread_cond_destroy(&channel->producer_cv);
    pthread_cond_destroy(&channel->consumer_cv);
    munmap(channel, sizeof(channel_t));
    
    UNUSED(channel);
}

int channel_produce(channel_t* channel, const char* produced_data, uint16_t length) {
    // Implement stage 3 here
    UNUSED(channel);
    UNUSED(produced_data);
    UNUSED(length);

    mutex_lock_handler(&channel->data_mtx);
    while(channel->status == CHANNEL_EMPTY)
    {
        if(pthread_cond_wait(&channel->producer_cv, &channel->data_mtx)){
            pthread_mutex_unlock(&channel->data_mtx);
            ERR("cond wait");
        }
    }
    strncpy(channel->data, produced_data, length);
    length = strlen(produced_data) + 1;
    channel->status = CHANNEL->CHANNEL_OCCUPIED;
    return 0;
}

int channel_consume(channel_t* channel, char* consumed_data, uint16_t* length) {
    // Implement stage 2 here
    UNUSED(channel);
    UNUSED(consumed_data);
    UNUSED(length);

    mutex_lock_handler(&channel->data_mtx);
    if(channel->status == CHANNEL_DEPLETED){
        pthread_mutex_unlock(&channel->data_mtx);
        return 1;
    }
    while(channel->status == CHANNEL_OCCUPIED)
    {
        if(pthread_cond_wait(&channel->consumer_cv, &channel->data_mtx)){
            pthread_mutex_unlock(&channel->data_mtx);
            ERR("cond wait");
        }
    }
    strncpy(consumed_data, channel->data, *length);
    *length = channel->length;
    channel->status = CHANNEL->EMPTY;

    if(pthread_cond_signal(&channel->producer_cv)){
        pthread_mutex_unlock(&channel->data_mtx);
    }
    pthread_mutex_unlock(&channel->data_mtx);

    return 0;
}

#pragma once

#include <pthread.h>
#include <stdint.h>

#define MAX_STATION_COUNT 64
#define INVALID_STATION MAX_STATION_COUNT
#define MAX_CONNECTION_COUNT (((MAX_STATION_COUNT - 1) * MAX_STATION_COUNT) / 2)
#define MAX_TRAIN_NAME_LENGTH 32

/**
 * Type describing station in shared memory
 */
typedef uint8_t station_idx;

/**
 * Structure of railway network in share memory.
 * Network is represented as a directed graph by neighbour list for every station.
 * Each edge (one railroad with direction) has: length, name of occupying train and information if rail has derailed on
 * it. To synchronize operations on railroads every rail has separate mutex.
 *
 * Station has indices from 0,1,...,MAX_STATION_COUNT - 1.
 * Edges are represented by indices calculated from formula (v,w) -> v * MAX_STATION_COUNT + w.
 */
typedef struct railway_network
{
    /**
     * Number of railroads (edges) in graph. It is setup once during railroad initialization.
     */
    uint16_t connection_count;
    /**
     * Number of stations (vertices) in graph. It is setup once during railroad initialization.
     */
    uint8_t station_count;

    /**
     * Description of graph as a neighbour list. First index points to array of neighbours of given station.
     * To get know how many neighbours has given station use neighbour_count array.
     * Neighbours are not sorted but are unique.
     */
    station_idx neighbour_list[MAX_STATION_COUNT][MAX_STATION_COUNT - 1];
    /**
     * This array holds information about neighbour count from given station.
     * Use it with neighbour_list to traverse all neighbours.
     */
    uint8_t neighbour_count[MAX_STATION_COUNT];

    /*
     * This mutexes locks all connection related variables below.
     * W can assume that al variables above cannot be changed by trains and are lock free.
     * Use conversion functions to get index of connection from indices of two stations.
     */
    pthread_mutex_t connection_locks[MAX_CONNECTION_COUNT];
    /**
     * This array holds lengths of railroads used in network.
     * To indicate missing connection is has max possible value.
     * Use conversion functions to get index of connection from indices of two stations.
     */
    uint16_t connection_lengths[MAX_CONNECTION_COUNT];
    /**
     * This array contains information about derailed trains on given connection.
     * If some index has 0 railroad is in good condition.
     * Otherwise railroad is broken and cannot be used by any other train.
     * Use conversion functions to get index of connection from indices of two stations.
     */
    uint8_t connection_destroyed[MAX_CONNECTION_COUNT];
    /**
     * This array contains name of occupying train.
     * If connection is free there should be zeroed out.
     * Use conversion functions to get index of connection from indices of two stations.
     */
    char connection_train_occupying[MAX_CONNECTION_COUNT][MAX_TRAIN_NAME_LENGTH];
} railway_network_t;

/**
 * This functions assumes there is no existing shared memory object.
 * Its responsibility is to create shared memory object and initialize railway network.
 * On failure it should stop the program with error message (ERR macro).
 * After initialization network should have all shared objects initialized.
 * Network should have 0 connections and stations.
 * All neighbours has to be set to INVALID station and all connections should have maximum possible length.
 *
 * @return As a result it should return mmaped shared memory object  which is properly initialized.
 */
railway_network_t* railway_network_init();

/**
 * This functions assumes there is existing shared memory object.
 * Its responsibility is to mmap the object to memory space of process.
 * On failure it should stop the program with error message (ERR macro).
 *
 * @return As a result it should return mmaped shared memory object.
 */
railway_network_t* railway_network_open();

/**
 * This function should release shared memory object.
 * Descriptor and pointer has to be connected to same shared memory object.
 * This functions will not destroy shared memory object.
 *
 * @param shmem Pointer to shared memory object received from railway_network_init/open function.
 */
void railway_network_close(railway_network_t* shmem);

/**
 *  This function should do the same as railway_network_close
 *  and additionally it should destroy shared memory object in kernel.
 * @param shmem Pointer to shared memory object received from railway_network_init/open function.
 */
void railway_network_destroy(railway_network_t* shmem);

/**
 * Functions used to conversion from station indices to connection index.
 * a --> b
 *
 * @param a Station where connection starts
 * @param b Station where connection ends
 * @return Index of connection between a -> b
 */
int convert_connection_to_idx(station_idx a, station_idx b);

/**
 * Function used to get station indices from connection index.
 * a --> b
 *
 * @param idx Index of connection
 * @param a Station index where connections starts. This function should set this variable.
 * @param b Station index where connections ends. This function should set this variable.
 */
void convert_idx_to_connection(int idx, station_idx* a, station_idx* b);

/**
 * This function handles procedure of correct locking of connection mutex.
 * It has take into consideration what to do when mutex's owner died when it was locked.
 * After returning from function mutex is locked and in valid state.
 *
 * @param net Pointer to railway network in shared memory.
 * @param conn_idx Index of connection to lock.
 * @return
 * It returns 0 when mutex lock was uneventful.
 * If 1 is returned it means mutex was made consistent after locking (earlier owner died).
 */
int railway_lock_connection_mutex(railway_network_t* net, int conn_idx);

/**
 * This functions states if given connection exists in network.
 *
 * @param net Pointer to railway network in shared memory.
 * @param a Station where connections should start.
 * @param b Station where connections should end.
 * @param idx Index of found connection. Otherwise it set to -1.
 * @return Returns 1 if connection was found. Otherwise it returns 0.
 */
int railway_network_connection_exists(railway_network_t* net, station_idx a, station_idx b, int* idx);

/**
 * This functions adds connection between given stations.
 * It does nothing when connection already exists.
 *
 * @param net Pointer to railway network in shared memory.
 * @param a Station where connections starts.
 * @param b Station where connections ends.
 * @param length Length of the connection.
 * @return Returns 0 if adding succeeded. If connection exists or station index is out of bound it returns 1.
 */
int railway_network_add_connection(railway_network_t* net, station_idx a, station_idx b, uint16_t length);

/**
 * Functions found shortest way in network between two stations. It skips destroyed connections during search.
 * If there is shortest path function should return number of edges in this path and set them in path array.
 * Path array first element has starting station as given from index and last element has ending station as given to
 * index.
 *
 *
 * @param net Pointer to railway network in shared memory.
 * @param from Station where path starts.
 * @param to Station where path ends.
 * @param path Pointer to array where path connecting from and to is set. Elements of path are connection indices.
 * @return It returns number of elements in path array. If path cannot be found it returns -1.
 */
int railway_network_find_shortest_way(railway_network_t* net, station_idx from, station_idx to,
                                      uint16_t path[MAX_STATION_COUNT]);

/**
 * This function prints information about railway network to STDOUT.
 *
 * @param net Pointer to railway network in shared memory.
 */
void railway_network_print(const railway_network_t* net);

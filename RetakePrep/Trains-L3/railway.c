#include "railway.h"

#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "macros.h"

#define SHMEM_NAME "/sop-shmem"

railway_network_t *railway_network_init()
{
    // STAGE1
    // Use SHMEM_NAME for creating shared memory object.
    // It would keep $make clean-resources command working
    int fd;
    int init_flag = 1;
    if((fd = shm_open(SHMEM_NAME, O_CREAT | O_EXCL | O_RDWR, 0666)) < 0)
    {
        if(errno == EEXIST){
            init_flag = 0;
            if((fd = shm_open(SHMEM_NAME, O_RDWR, 0600)) < 0){
                ERR("shm_open");
            }
        }
        else
            ERR("shm_open");
    }
    else{
        if(ftruncate(fd, sizeof(railway_network_t)) <0){
            shm_unlink(SHMEM_NAME);
            ERR("ftruncate");
        }
    }
    railway_network_t * network;
    if(MAP_FAILED == (network = (railway_network_t*)mmap(NULL, sizeof(railway_network_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0))){
        if(init_flag){
            shm_unlink(SHMEM_NAME);
            ERR("mmap");
        }
        else{
            ERR("mmap");
        }
    }

    if(close(fd) < 0)
        ERR("close");

    network->connection_count = 0;
    network->station_count = 0;
    for(int i = 0; i < MAX_STATION_COUNT; i++){
        for(int j = 0; j < MAX_STATION_COUNT - 1; j++){
            network->neighbour_list[i][j] = INVALID_STATION;
            
        }
    }

    return network;
}

railway_network_t *railway_network_open()
{
    // STAGE2
    return NULL;
}

void railway_network_close(railway_network_t *shmem)
{
    // STAGE2
    UNUSED(shmem);
}

void railway_network_destroy(railway_network_t *shmem)
{
    // STAGE1
    if(munmap(shmem, sizeof(railway_network_t)) == 0){
        printf("munmap successful\n");
    }
    else
        ERR("munmap");
}

int convert_connection_to_idx(station_idx a, station_idx b) { return a * MAX_STATION_COUNT + b; }

void convert_idx_to_connection(int idx, station_idx *a, station_idx *b)
{
    *a = idx / MAX_STATION_COUNT;
    *b = idx % MAX_STATION_COUNT;
}

int railway_network_connection_exists(railway_network_t *net, station_idx a, station_idx b, int *idx)
{
    if (idx)
        *idx = -1;
    for (uint8_t i = 0; i < net->neighbour_count[a]; ++i)
        if (net->neighbour_list[a][i] == b)
        {
            if (idx)
                *idx = i;
            return 1;
        }
    return 0;
}

int railway_network_add_connection(railway_network_t *net, station_idx a, station_idx b, uint16_t length)
{
    if (a > net->station_count || b > net->station_count)
        return 1;

    if (railway_network_connection_exists(net, a, b, NULL) || railway_network_connection_exists(net, b, a, NULL))
        return 1;

    net->neighbour_list[a][net->neighbour_count[a]] = b;
    net->neighbour_list[b][net->neighbour_count[b]] = a;
    net->neighbour_count[a]++;
    net->neighbour_count[b]++;
    net->connection_lengths[convert_connection_to_idx(a, b)] = length;
    net->connection_lengths[convert_connection_to_idx(b, a)] = length;

    return 0;
}

int railway_lock_connection_mutex(railway_network_t *net, int conn_idx)
{
    // STAGE3 and STAGE4
    UNUSED(net);
    UNUSED(conn_idx);
    return 0;
}

int railway_network_find_shortest_way(railway_network_t *net, station_idx from, station_idx to,
                                      uint16_t path[MAX_STATION_COUNT])
{
    if (from > net->station_count || to > net->station_count)
        return -1;

    // Prepare data structures
    int front_idx = 0;
    int last_idx = 0;
    station_idx visiting_order[MAX_STATION_COUNT];
    station_idx previous_station[MAX_STATION_COUNT];
    int shortest_path_length[MAX_STATION_COUNT];
    uint8_t visited[MAX_STATION_COUNT];
    int distance[MAX_STATION_COUNT];
    memset(visiting_order, INVALID_STATION, sizeof(visiting_order));
    memset(previous_station, INVALID_STATION, sizeof(previous_station));
    memset(shortest_path_length, 0, sizeof(shortest_path_length));
    memset(visited, 0, sizeof(visited));
    for (int i = 0; i < MAX_STATION_COUNT; ++i)
        distance[i] = -1;

    // Add starting station as first vertex
    visiting_order[0] = from;
    distance[from] = 0;
    distance[to] = -1;
    last_idx = 1;

    // Main loop of dijkstra algorithm
    while (front_idx < last_idx)
    {
        station_idx current = visiting_order[front_idx++];
        int current_distance = distance[current];

        for (int i = 0; i < net->neighbour_count[current]; ++i)
        {
            station_idx neighbour = net->neighbour_list[current][i];
            int connection_idx = convert_connection_to_idx(current, neighbour);

            // Checking if connection is broken without lock! Otherwise, searching for path is blocked by running trains.
            if (net->connection_destroyed[connection_idx])
            {
                // Skipping broken connection
                continue;
            }
            uint16_t connection_length = net->connection_lengths[connection_idx];

            // Updating distance to neighbour
            if (distance[neighbour] == -1 || distance[neighbour] > current_distance + connection_length)
            {
                distance[neighbour] = current_distance + connection_length;
                previous_station[neighbour] = current;
                shortest_path_length[neighbour] = shortest_path_length[current] + 1;
            }

            // Queueing neighbour for visit if it wasn't visited already
            if (!visited[neighbour])
            {
                visited[neighbour] = 1;
                visiting_order[last_idx++] = neighbour;
            }
        }
    }

    if (distance[to] == -1)
        return -1;

    // Gathering shortest path
    station_idx it = to;
    for (int i = shortest_path_length[to] - 1; i >= 0; --i)
    {
        station_idx previous = previous_station[it];
        path[i] = convert_connection_to_idx(previous, it);
        it = previous;
    }

    return shortest_path_length[to];
}

void railway_network_print(const railway_network_t *net)
{
    printf("Station count: %d\n", net->station_count);
    int conn_counter = 1;
    for (int i = 0; i < net->station_count; ++i)
        for (int j = 0; j < net->neighbour_count[i]; ++j)
        {
            station_idx a = i;
            station_idx b = net->neighbour_list[i][j];

            int conn_idx = convert_connection_to_idx(a, b);

            printf("%d. %d-%d (%d)", conn_counter++, a, b, net->connection_lengths[conn_idx]);

            if (strlen(net->connection_train_occupying[conn_idx]))
                printf(" occupied by %s", net->connection_train_occupying[conn_idx]);

            if (net->connection_destroyed[conn_idx])
                printf(" DESTROYED");

            printf("\n");
        }
}

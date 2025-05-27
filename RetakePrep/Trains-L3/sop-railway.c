#include <getopt.h>  // <--- use it!
#include <stdio.h>
#include <stdlib.h>
#include "inttypes.h"

#include "macros.h"
#include "railway.h"

void usage(const char* exec_name)
{
    printf("%s [-dp] [-c filename] - manage railway network\n", exec_name);
    printf("-c filename - create railway network described in file\n");
    printf("-d - destroy existing railway network\n");
    printf("-p - print existing railway network (with running trains)\n");
}

railway_network_t* create_railway(const char* filepath)
{
    railway_network_t* net = railway_network_init();

    FILE* railway_file = fopen(filepath, "r");
    if (!railway_file)
        ERR("fopen");

    int connection_count = 0;
    net->station_count = 0;
    if (1 != fscanf(railway_file, "%" SCNu8, &net->station_count) || net->station_count == 0)
    {
        fclose(railway_file);
        railway_network_close(net);
        fprintf(stderr, "Cannot read station count from %s\n", filepath);
        return NULL;
    }
    if (1 != fscanf(railway_file, "%ui", &connection_count) || connection_count == 0)
    {
        fclose(railway_file);
        railway_network_close(net);
        fprintf(stderr, "Cannot read connection count from %s\n", filepath);
        return NULL;
    }

    if (net->station_count > MAX_STATION_COUNT)
    {
        fclose(railway_file);
        railway_network_close(net);
        fprintf(stderr, "Cannot create railway network with %ud > %ud stations from file %s\n", net->station_count,
                MAX_STATION_COUNT, filepath);
        return NULL;
    }

    station_idx a = INVALID_STATION;
    station_idx b = INVALID_STATION;
    uint16_t length = 0;
    for (int i = 0; i < connection_count; ++i)
    {
        a = INVALID_STATION;
        b = INVALID_STATION;
        length = 0;

        if (1 != fscanf(railway_file, "%" SCNu8, &a) || a == INVALID_STATION)
        {
            fclose(railway_file);
            railway_network_close(net);
            fprintf(stderr, "Cannot read station A at line %d from %s\n", i, filepath);
            return NULL;
        }
        if (1 != fscanf(railway_file, "%" SCNu8, &b) || b == INVALID_STATION)
        {
            fclose(railway_file);
            railway_network_close(net);
            fprintf(stderr, "Cannot read station B at line %d from %s\n", i, filepath);
            return NULL;
        }
        if (1 != fscanf(railway_file, "%" SCNu16, &length) || length == 0)
        {
            fclose(railway_file);
            railway_network_close(net);
            fprintf(stderr, "Cannot read connection length at line %d from %s\n", i, filepath);
            return NULL;
        }

        if (railway_network_add_connection(net, a, b, length))
        {
            fclose(railway_file);
            railway_network_close(net);
            fprintf(stderr, "Cannot add connection from line %d to network from file %s\n", i, filepath);
            return NULL;
        }
    }

    fclose(railway_file);
    return net;
}

int main(int argc, char* argv[])
{
    // STAGE1 and STAGE2
    UNUSED(argc);
    UNUSED(argv);

    railway_network_t* net = railway_network_init();
    railway_network_destroy(net);

    return EXIT_SUCCESS;
}
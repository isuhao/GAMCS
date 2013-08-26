#ifndef SIMGROUP_H
#define SIMGROUP_H
#include <pthread.h>
#include <string>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <fstream>
#include "Group.h"
#include "MyAgent.h"

using namespace std;

#define MAX_MEMBER 1000

struct Channel
{
    pthread_mutex_t mutex;
    struct Frame *frame;
};

struct Frame
{
    char data[2048];
    struct Frame *next;
};

struct Neigh
{
    int id;
    struct Neigh *next;
};

class SimGroup : public Group
{
    public:
        SimGroup(int);
        virtual ~SimGroup();

        void LoadTopo(string);
        int NumOfMembers();

        int Send(int, void *, size_t);
        int Recv(int, void *, size_t);
    protected:
    private:
        string topofile;

        void BuildNeighs();
        vector<int> GetNeighs(int);
        struct Channel *GetChannel(int);
        void Notify(int);

        struct Channel channels[MAX_MEMBER];
        struct Neigh *neighlist[MAX_MEMBER];
};

#endif // SIMGROUP_H
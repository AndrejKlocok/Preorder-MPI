/**
 * Autor:   Andrej Klocok, xkloco00
 * Projekt: Preorder a eulerova kruznica
 * Subor:   mss.cpp
 */

#include <mpi.h>
#include <vector>
#include <math.h>
#include <map>

#define EDGE 0
#define VEC 1
#define NEXT 2
#define PROC 3
#define VALUE 4
#define ACK 5

using namespace std;



struct edge {
   char fromNode;  // u
   char toNode;    // v
    int myId;
    int rev;
    int next;
};

void createEdge(int from, int to, int *p, std::vector<char> *array, std::map<char, vector<edge> > *adjMap){
    struct edge edge1, edge2;
    
    edge1.fromNode = array->at(from);
    edge1.toNode = array->at(to);
    edge1.myId = *p;
    edge1.rev = *p +1;
    edge1.next = -1;
    
    edge2.fromNode = array->at(to);
    edge2.toNode = array->at(from);
    edge2.myId = *p +1;
    edge2.rev = *p;
    edge2.next = -1;
    
    (*adjMap)[edge1.fromNode].push_back(edge1);
    (*p)++;
    
    (*adjMap)[edge2.fromNode].push_back(edge2);
    (*p)++;
}

void createAdjList(std::vector<char> array, int processorCount){
    std::map<char, vector<edge> > adjMap;
    vector<int> adjList;
    vector<int> indexVertex;
    int index = 0;
    int p=0;
    
    for(int i=1; i < array.size(); i++){
        std::vector<edge> edgeListTMP;
        adjMap.insert(std::pair<char, vector<edge> > (array.at(i), edgeListTMP));
    }
    // Vytvorim vektor parov hran a rozoslem hrany procesorom
    for(int i=1; i < array.size(); i++){
        
        // poslem cestu k lavemu synovi
        if(2*i < array.size()){
            createEdge(i, 2*i, &p, &array, &adjMap);
        }        
        // poslem cestu k pravemu synovi
        if((2*i +1 )< array.size()){
            createEdge(i, 2*i+1, &p, &array, &adjMap);
        }
    }
    adjList.push_back(-1);
    
    for(map<char, vector<edge> >::iterator it = adjMap.begin(); it!=adjMap.end(); it++){   
        vector<edge> vec = it->second;
        
        indexVertex.push_back(index);   // ulozime si index vrcholu
        bool first = true;

        for(int j=0; j < vec.size(); j++){
            struct edge e = vec.at(j);
            
            if (j+1 < vec.size()){
                e.next = vec.at(j+1).myId;
            }
            
            if (first)
                adjList.push_back(e.myId);
            first = false;
            
            MPI_Send(&e, sizeof(e), MPI_BYTE, e.myId, EDGE, MPI_COMM_WORLD); 
        }
        
    }
    
    for (int p=0; p< processorCount; p++){
        MPI_Send(&adjList[0], adjList.size(), MPI_INT, p, VEC, MPI_COMM_WORLD);
    }
}

void recvEdge(struct edge* myEdge, vector<int>* adjList, int arrSize){
    MPI_Status status; 
    
    MPI_Recv(myEdge, sizeof(*myEdge), MPI_BYTE, 0, EDGE, MPI_COMM_WORLD, &status);
    MPI_Recv(&adjList[0], arrSize, MPI_INT, 0, VEC, MPI_COMM_WORLD, &status);
}

bool serve(int value, int nextEdge, int rank){
    MPI_Status status;     
    int rankToSend;
    
    // recv rank procesora
    MPI_Recv(&rankToSend, 1, MPI_INT, MPI_ANY_SOURCE, PROC, MPI_COMM_WORLD, &status);
    // printf("%d working\n", rank);
    if (rankToSend != -1){
        // posli value
        MPI_Send(&value, 1, MPI_INT, rankToSend, VALUE, MPI_COMM_WORLD);
        // posli odkaz na suseda
        MPI_Send(&nextEdge, 1, MPI_INT, rankToSend, NEXT, MPI_COMM_WORLD);
        
        return true;
    }
    else{
        MPI_Send(&rank, 1, MPI_INT, status.MPI_SOURCE, ACK, MPI_COMM_WORLD);
        printf("%d:I am free thanks to %d\n", rank, status.MPI_SOURCE);
        return false;
    }
}

void lastProcDuty(int rank, int numbTimes, int value, int nextEdge, int proccesorCount){
    MPI_Status status;
    int release = -1;
    int prisoner;
    printf("%d here i will serve for %d times \n", rank, numbTimes);
    for(int i=0; i < numbTimes; i++){
        printf("%d here serving %d time\n", rank, i);
        serve(value, nextEdge, rank);
    }
    
    for(int i=0; i < proccesorCount; i++){
        if(i != rank){
            printf("Lets free %d \n", i);
            MPI_Send(&release, 1, MPI_INT, i, PROC, MPI_COMM_WORLD);
            MPI_Recv(&prisoner, 1, MPI_INT, i, ACK, MPI_COMM_WORLD, &status);
        }
    }
}

int main(int argc, char* argv[]) {

    int processorsCount;        // pocet vsetkych procesorov
    int rank;                   // rank procesora
    MPI_Status status;          // status
    bool timeCheck = false;     // meranie casovej narocnosti
    std::string input;
    std::vector<char> array;
    std::vector<int> adjList;
    struct edge myEdge;
    
    if(argc == 3 ) {
        if (strcmp(argv[1], "-arr") == 0){
            input = argv[2];
        }
    } 
    /* Nespravny format argumentov*/
    else {
        cout << "Wrong arguments format" << endl;
        return 1;
    }
    array.push_back(0);         //0-ty prvok ignorujeme
    
    for(int i=0; i < input.size(); i++){
        array.push_back(input.at(i));  
    }
    
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD,&processorsCount);
    MPI_Comm_rank(MPI_COMM_WORLD,&rank);
    
    
    
    if(rank == 0){
        createAdjList(array, processorsCount);
    }
    
    MPI_Recv(&myEdge, sizeof(myEdge), MPI_BYTE, 0, EDGE, MPI_COMM_WORLD, &status);
    adjList.resize(array.size());
    MPI_Recv(&adjList[0], array.size(), MPI_INT, 0, VEC, MPI_COMM_WORLD, &status);
    
    
    int nextEdge;
  
    MPI_Send(&myEdge.next, 1, MPI_INT, myEdge.rev, NEXT, MPI_COMM_WORLD); 
    MPI_Recv(&nextEdge, 1, MPI_INT, myEdge.rev, NEXT, MPI_COMM_WORLD, &status);
    
   
    /*  Eulerova cesta  */
    if(nextEdge == -1){
        int vertex;
        for(int i =1; i<array.size(); i++){
            if(array.at(i) == myEdge.toNode){
                vertex = i;
                break;
            }
        }
        nextEdge = adjList.at(vertex);
    }
    /*--------------------*/

    /*  Ak moj nasledik je prva hrana tak zareznem  */
    if(nextEdge == 0)
        nextEdge = rank;
    /*--------------------*/
    
    /*  Suffix sum  */
    int neighValue, newNextEdge, value, numbTimes;
    int end = ceil(log2(processorsCount));
    
    value = 1;
    
    if(nextEdge == rank)
        value = 0;
        
    
    // fix ked bude sa pytat prvy
    // kolko sprav bude?
    for(int k = 0; k<end; k++){
        // posledny procesor
        
        if(rank == nextEdge){
            numbTimes = pow(2,k);
            lastProcDuty(rank,numbTimes, value, nextEdge, processorsCount);
        }
        //ostatne procesory
        else{
           // poslem svoj rank
            MPI_Send(&rank, 1, MPI_INT, nextEdge, PROC, MPI_COMM_WORLD);
            printf("%d: serving\n", rank);
            while(1){
                if(!serve(value, nextEdge, rank)){
                    break;
                }
            };
            printf("%d: receiving \n", rank);
            MPI_Recv(&neighValue, 1, MPI_INT, nextEdge, VALUE, MPI_COMM_WORLD, &status);
            MPI_Recv(&newNextEdge, 1, MPI_INT, nextEdge, NEXT, MPI_COMM_WORLD, &status);
            
            value += neighValue;
            nextEdge = newNextEdge; 
        }
    }
    //correction
    //value = value - processorsCount+1;
    /*--------------*/
   
    // printf("I am %d, my value is %d\n", rank, value);
    
    MPI_Finalize(); 
    return 0;
}

--------------------------------------------------------------------------
/**
 * Autor:   Andrej Klocok, xkloco00
 * Projekt: Preorder a eulerova kruznica
 * Subor:   mss.cpp
 */

#include <mpi.h>
#include <vector>
#include <math.h>
#include <map>
#include <stdlib.h> 

#define EDGE 0
#define VEC 1
#define NEXT 2
#define PROC 3
#define VALUE 4
#define ACK 5

using namespace std;



struct edge {
   char fromNode;  // u
   char toNode;    // v
    int myId;
    int rev;
    int next;
};

struct sendProc {
    int myRank;
    bool dontRecv;
};

struct recvProc {
    int nextEdge;
    int value;
    bool finalVal;
};

void createEdge(int from, int to, int *p, std::vector<char> *array, std::map<char, vector<edge> > *adjMap){
    struct edge edge1, edge2;
    
    edge1.fromNode = array->at(from);
    edge1.toNode = array->at(to);
    edge1.myId = *p;
    edge1.rev = *p +1;
    edge1.next = -1;
    
    edge2.fromNode = array->at(to);
    edge2.toNode = array->at(from);
    edge2.myId = *p +1;
    edge2.rev = *p;
    edge2.next = -1;
    
    (*adjMap)[edge1.fromNode].push_back(edge1);
    (*p)++;
    
    (*adjMap)[edge2.fromNode].push_back(edge2);
    (*p)++;
}

void createAdjList(std::vector<char> array, int processorCount){
    std::map<char, vector<edge> > adjMap;
    vector<int> adjList;
    vector<int> indexVertex;
    int index = 0;
    int p=0;
    
    for(int i=1; i < array.size(); i++){
        std::vector<edge> edgeListTMP;
        adjMap.insert(std::pair<char, vector<edge> > (array.at(i), edgeListTMP));
    }
    // Vytvorim vektor parov hran a rozoslem hrany procesorom
    for(int i=1; i < array.size(); i++){
        
        // poslem cestu k lavemu synovi
        if(2*i < array.size()){
            createEdge(i, 2*i, &p, &array, &adjMap);
        }        
        // poslem cestu k pravemu synovi
        if((2*i +1 )< array.size()){
            createEdge(i, 2*i+1, &p, &array, &adjMap);
        }
    }
    adjList.push_back(-1);
    
    for(map<char, vector<edge> >::iterator it = adjMap.begin(); it!=adjMap.end(); it++){   
        vector<edge> vec = it->second;
        
        indexVertex.push_back(index);   // ulozime si index vrcholu
        bool first = true;

        for(int j=0; j < vec.size(); j++){
            struct edge e = vec.at(j);
            
            if (j+1 < vec.size()){
                e.next = vec.at(j+1).myId;
            }
            
            if (first)
                adjList.push_back(e.myId);
            first = false;
            
            MPI_Send(&e, sizeof(e), MPI_BYTE, e.myId, EDGE, MPI_COMM_WORLD); 
        }
        
    }
    
    for (int p=0; p< processorCount; p++){
        MPI_Send(&adjList[0], adjList.size(), MPI_INT, p, VEC, MPI_COMM_WORLD);
    }
}

void recvEdge(struct edge* myEdge, vector<int>* adjList, int arrSize){
    MPI_Status status; 
    
    MPI_Recv(myEdge, sizeof(*myEdge), MPI_BYTE, 0, EDGE, MPI_COMM_WORLD, &status);
    MPI_Recv(&adjList[0], arrSize, MPI_INT, 0, VEC, MPI_COMM_WORLD, &status);
}

bool serve(int value, int nextEdge, int rank){
    MPI_Status status;     
    int rankToSend;
    
    // recv rank procesora
    MPI_Recv(&rankToSend, 1, MPI_INT, MPI_ANY_SOURCE, PROC, MPI_COMM_WORLD, &status);
    // printf("%d working\n", rank);
    if (rankToSend != -1){
        // posli value
        MPI_Send(&value, 1, MPI_INT, rankToSend, VALUE, MPI_COMM_WORLD);
        // posli odkaz na suseda
        MPI_Send(&nextEdge, 1, MPI_INT, rankToSend, NEXT, MPI_COMM_WORLD);
        
        return true;
    }
    else{
        MPI_Send(&rank, 1, MPI_INT, status.MPI_SOURCE, ACK, MPI_COMM_WORLD);
        printf("%d:I am free thanks to %d\n", rank, status.MPI_SOURCE);
        return false;
    }
}

void lastProcDuty(int rank, int numbTimes, int value, int nextEdge, int proccesorCount){
    MPI_Status status;
    int release = -1;
    int prisoner;
    printf("%d here i will serve for %d times \n", rank, numbTimes);
    for(int i=0; i < numbTimes; i++){
        printf("%d here serving %d time\n", rank, i);
        serve(value, nextEdge, rank);
    }
    
    for(int i=0; i < proccesorCount; i++){
        if(i != rank){
            printf("Lets free %d \n", i);
            MPI_Send(&release, 1, MPI_INT, i, PROC, MPI_COMM_WORLD);
            MPI_Recv(&prisoner, 1, MPI_INT, i, ACK, MPI_COMM_WORLD, &status);
        }
    }
}

int main(int argc, char* argv[]) {

    int processorsCount;        // pocet vsetkych procesorov
    int rank;                   // rank procesora
    MPI_Status status;          // status
    bool timeCheck = false;     // meranie casovej narocnosti
    std::string input;
    std::vector<char> array;
    std::vector<int> adjList;
    struct edge myEdge;
    
    
    if(argc == 3 ) {
        if (strcmp(argv[1], "-arr") == 0){
            input = argv[2];
        }
    } 
    /* Nespravny format argumentov*/
    else {
        cout << "Wrong arguments format" << endl;
        return 1;
    }
    array.push_back(0);         //0-ty prvok ignorujeme
    
    for(int i=0; i < input.size(); i++){
        array.push_back(input.at(i));  
    }
    
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD,&processorsCount);
    MPI_Comm_rank(MPI_COMM_WORLD,&rank);
    
    
    
    if(rank == 0){
        createAdjList(array, processorsCount);
    }
    
    MPI_Recv(&myEdge, sizeof(myEdge), MPI_BYTE, 0, EDGE, MPI_COMM_WORLD, &status);
    adjList.resize(array.size());
    MPI_Recv(&adjList[0], array.size(), MPI_INT, 0, VEC, MPI_COMM_WORLD, &status);
    
    
    int nextEdge;
  
    MPI_Send(&myEdge.next, 1, MPI_INT, myEdge.rev, NEXT, MPI_COMM_WORLD); 
    MPI_Recv(&nextEdge, 1, MPI_INT, myEdge.rev, NEXT, MPI_COMM_WORLD, &status);
    
   
    /*  Eulerova cesta  */
    if(nextEdge == -1){
        int vertex;
        for(int i =1; i<array.size(); i++){
            if(array.at(i) == myEdge.toNode){
                vertex = i;
                break;
            }
        }
        nextEdge = adjList.at(vertex);
    }
    /*--------------------*/

    /*  Ak moj nasledik je prva hrana tak zareznem  */
    if(nextEdge == 0)
        nextEdge = rank;
    /*--------------------*/
    
    /*  Suffix sum  */
    int value;
    int end = ceil(log2(processorsCount));
    
    struct sendProc mySend, sendNeigh; 
    struct recvProc myRecv, recvNeigh;
    
    bool dontRecv = false;
    bool finalVal = false;
    
    value = 1;
    
    if(nextEdge == rank){
        value = 0;
        finalVal = true;
    }
    if(rank == 0){
        dontRecv = true;
    }
    
    // fix ked bude sa pytat prvy
    // kolko sprav bude?
    for(int k = 0; k<end; k++){
        
        mySend.myRank = rank;
        mySend.dontRecv = dontRecv;
        
        myRecv.finalVal = finalVal;
        myRecv.nextEdge = nextEdge;
        myRecv.value = value;
        
        // poslem svoj rank
        if(!finalVal){
            MPI_Send(&mySend, sizeof(mySend), MPI_BYTE, nextEdge, PROC, MPI_COMM_WORLD);
        }
        
        if(!dontRecv){
           
            MPI_Recv(&sendNeigh, sizeof(sendNeigh), MPI_BYTE, MPI_ANY_SOURCE, PROC, MPI_COMM_WORLD, &status);
            MPI_Send(&myRecv, sizeof(myRecv), MPI_BYTE, sendNeigh.myRank, VALUE, MPI_COMM_WORLD);
            dontRecv = sendNeigh.dontRecv;
        }
        
        if(!finalVal){
            MPI_Recv(&recvNeigh, sizeof(recvNeigh), MPI_BYTE, nextEdge, VALUE, MPI_COMM_WORLD, &status);
            
            value += recvNeigh.value;
            nextEdge = recvNeigh.nextEdge;
            finalVal = recvNeigh.finalVal;
        }
        
        
        
        
    }
    //correction
    //value = abs(value - processorsCount+1);
    /*--------------*/
   
    printf("I am %d, my value is %d\n", rank, value);
    
    MPI_Finalize(); 
    return 0;
}


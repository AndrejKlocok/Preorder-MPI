/**
 * Autor:   Andrej Klocok, xkloco00
 * Projekt: Priradenie poradia preorder vrcholom
 * Subor:   pro.cpp
 */

#include <mpi.h>
#include <vector>
#include <math.h>
#include <map>
#include <stdlib.h> 
/**
 * Tag reprezentuje odosielanu hodnotu ako struct edge..
 */
#define EDGE 0
/**
 *  Tag reprezentuje odosielanu hodnotu ako pole integerov.
 */
#define VEC 1
/**
 * Tag reprezentuje odosielanu hodnotu ako integer
 */
#define INTEGER 2
/**
 *  Tag reorezentuje odosielanu hodnotu ako struct sendProc. Tento tag sa vyuziva
 *  pri implementacii sumy suffixov.
 */
#define PROC 3
/**
 * Tag reprezentuje odosielanu hodnotu ako struct recvProc. Tento tag vyuzivame
 * pri implementacii sumy suffixov.
 */
#define VALUE 4
/**
 * Tento tag nesu spravy potrebne na synchronizaciu procesorov po kazdej iteracii
 * sumy suffixov.
 */
#define ACK 5

using namespace std;
/**
 *  Struktura, ktora zahrna potrebne informaciie pre inicializaciu hrany.
 */
struct edge {
   int fromNode;  // u
   int toNode;    // v
   int myId;
   int rev;
   int next;
   bool isForward;
};
/**
 *  Struktura sa vyzuva pri implementacii sumy suffixov. Obsahuje rank odosielajuceho
 *  procesora a bool hodotu, ktora znaci, ci ma procesor ocakavat spravu v dalsej iteracii
 */
struct sendProc {
    int rank;
    bool doRecv;
};
/**
 *  Strukra zahrna jednotlive informacie, ktore si procesory pri sume suffixov vymienaju.
 */
struct recvProc {
    int nextEdge;
    int value;
};
/**
 * Funkcia vytvori obe hrany z daneho vrchola (t.j. A->B a B->A) a ulozi ich do adjMap na prislusny
 * index. 
 * @param from      index do pola vrcholov pociatocneho vrchola hrany   U
 * @param to        index do pola vrcholov konecneho vrchola hrany      V
 * @param p         index procesora, ktoremu hrana patri
 * @param array     pole vrcholov
 * @param adjMap    adjecency list reprezetovany vektorom vektorov hran
 */
void createEdges(int from, int to, int *p, std::vector<char> *array, std::vector< vector<edge> > *adjList){
    struct edge edge1, edge2;
    std::vector<edge> *vec;

    edge1.fromNode = from;
    edge1.toNode = to;
    edge1.myId = *p;
    edge1.rev = *p +1;
    edge1.next = -1;
    edge1.isForward = true;
    
    edge2.fromNode = to;
    edge2.toNode = from;
    edge2.myId = *p +1;
    edge2.rev = *p;
    edge2.next = -1;
    edge2.isForward = false;
    
    vec = &(adjList->at(from));
    vec->push_back(edge1);
    (*p)++;
    
    vec = &(adjList->at(to));
    vec->push_back(edge2);
    (*p)++;
}
/**
 * Funkcia vytvori adjecency list z reprezentacie binarneho stromu v poli array.
 * @param array             pole vrcholov
 * @param processorCount    celkovy pocet procesorov
 */
void createAdjList(std::vector<char> array, int processorCount){
    std::vector< vector<edge> > adjMap;
    std::vector<int> adjList;
    int p=0;
    
    // inicializacia prvotneho adjecency listu
    for(int i=0; i < array.size(); i++){
        std::vector<edge> edgeListTMP;
        adjMap.push_back(edgeListTMP);
    }
    // Vytvorim vektor parov hran a rozoslem hrany procesorom
    for(int i=1; i < array.size(); i++){
        
        // poslem cestu k lavemu synovi
        if(2*i < array.size()){
            createEdges(i, 2*i, &p, &array, &adjMap);
        }        
        // poslem cestu k pravemu synovi
        if((2*i +1 )< array.size()){
            createEdges(i, 2*i+1, &p, &array, &adjMap);
        }
    }
    adjList.push_back(-1);
    // Vytvorenie finalneho adjecency listu a rozposlanie hran
    for(int i=1; i < array.size(); i++){
        vector<edge> vec = adjMap.at(i);

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
    // Kazdemu procesoru poslem skrateny adjecency list
    for (int p=0; p< processorCount; p++){
        MPI_Send(&adjList[0], adjList.size(), MPI_INT, p, VEC, MPI_COMM_WORLD);
    }
}
/**
 * Funkcia sluzi na synchronizaciu procesorov po kazdej iteracii sumy suffixov
 * @param rank              rank procesora, ktory synchronizuje ostatne procesy
 * @param proccesorCount    celkovy pocet procesorov
 */
void sync(int rank, int proccesorCount){
    MPI_Status status;
    int dummy;
    // Pockam kym neskoncia vsetky
    for(int i=0; i < proccesorCount; i++){
        if(i != rank){
            MPI_Recv(&dummy, 1, MPI_INT, i, ACK, MPI_COMM_WORLD, &status);
        }
    }
    // Spustime ich znovu
    for(int i=0; i < proccesorCount; i++){
        if(i != rank){
            MPI_Send(&dummy, 1, MPI_INT, i, ACK, MPI_COMM_WORLD);
        }
    }
}
/**
 * Funkcia reprezentuje cinnost posledneho procesora v sume suffixov
 * @param rank              rank posledneho procesora v sume suffixov
 * @param numbTimes         pocet procesorov, ktore poslu spravu
 * @param myRecv            struktura obsahujuca potrebne informacie, pre ostatne procesory
 * @param proccesorCount    celkovy pocet procesorov
 */
void lastProcDuty(int rank, int numbTimes, struct recvProc myRecv, int proccesorCount){
    MPI_Status status;
    struct sendProc sendNeigh;
        
    // maximum dotazov pre posledny proces
    if(numbTimes > proccesorCount)
        numbTimes = proccesorCount;
    
    // Obsluha poziadaviek
    for(int i=0; i < numbTimes ; i++){
        //prijmeme ziadost procesora
        MPI_Recv(&sendNeigh, sizeof(struct sendProc), MPI_BYTE, MPI_ANY_SOURCE, PROC, MPI_COMM_WORLD, &status);
        // posli value
        MPI_Send(&myRecv, sizeof(myRecv), MPI_BYTE, sendNeigh.rank, VALUE, MPI_COMM_WORLD);
    }
    
    // synchronizujem
    sync(rank, proccesorCount);
}
/**
 * Funkcia testuje doprednost hrany.
 * @param e testovana hrana
 * @return  bool hodnota porovnania
 */
int isForward(struct edge* e){
    if(e->isForward)
        return true;
    else
        return false;
}
/**
 * Funkcia sluzi na vypis preored prechodu cez strom.
 * @param array pole vrcholov, vstupna reprezentacia stromu.
 */
void finalPrint(std::vector<char> array){
    MPI_Status status; 
    struct edge e;          
    int order;
    std::map<int,int> preorder;
    
    
    // vrchol je prvy v poradi
    preorder[1] = 1;
    // od kazdej doprednej hrany prijmem poradie vrcholu toNode
    for(int p=2; p<array.size(); p++){
        MPI_Recv(&e, sizeof(e), MPI_BYTE, MPI_ANY_SOURCE, EDGE, MPI_COMM_WORLD, &status);
        MPI_Recv(&order, 1, MPI_INT, e.myId, VALUE, MPI_COMM_WORLD, &status);
        preorder[order] = e.toNode;
    }
    
    //vypis
    for(map<int, int>::iterator it = preorder.begin(); it!=preorder.end(); it++){
        printf("%c", array.at(it->second));
    }
    printf("\n");
}

int main(int argc, char* argv[]) {
    int processorsCount;        // pocet vsetkych procesorov
    int rank;                   // rank procesora
    MPI_Status status;          // status
    std::string input;          // vstupne pole
    std::vector<char> array;    // pole vrcholov
    std::vector<int> adjList;   // adjecency list
    struct edge myEdge;         // hrana kazdeho procesora
    bool timeCheck = false;     // meranie casovej narocnosti
    double timeStart, timeEnd;  // casove razitka
    
    if(argc == 3 ) {
        if (strcmp(argv[1], "-arr") == 0){
            input = argv[2];
        }
    }
    else if(argc == 4 ) {
        if (strcmp(argv[1], "-arr") == 0){
            input = argv[2];
        }
        if (strcmp(argv[3], "-time") == 0){
            timeCheck = true;
        }
    } 
    /* Nespravny format argumentov*/
    else {
        cout << "Wrong arguments format" << endl;
        return 1;
    }
    array.push_back(0);     //0-ty prvok ignorujeme
    
    // inicializacia pola vrcholov
    for(int i=0; i < input.size(); i++){
        array.push_back(input.at(i));
    }
    
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD,&processorsCount);
    MPI_Comm_rank(MPI_COMM_WORLD,&rank);

    
    if(rank == 0){
        if(timeCheck)
            timeStart = MPI_Wtime();
        createAdjList(array, processorsCount);
    }
    /*  Kazdy procesor prijme svoju hranu a adjecency list  */
    MPI_Recv(&myEdge, sizeof(myEdge), MPI_BYTE, 0, EDGE, MPI_COMM_WORLD, &status);
    adjList.resize(array.size());
    MPI_Recv(&adjList[0], array.size(), MPI_INT, 0, VEC, MPI_COMM_WORLD, &status);
    
    /*  Eulerova cesta  */
    int nextEdge;   // rank hrany naslednika
  
    MPI_Send(&myEdge.next, 1, MPI_INT, myEdge.rev, INTEGER, MPI_COMM_WORLD); 
    MPI_Recv(&nextEdge, 1, MPI_INT, myEdge.rev, INTEGER, MPI_COMM_WORLD, &status);
    
    
    if(nextEdge == -1){
        nextEdge = adjList.at(myEdge.toNode);
    }
    /*--------------------*/
    
    /*  Ak moj nasledik je prva hrana tak budem ukazovat na seba a oznamim to ostatnym procesorom */
    int lastEdge;
    if(nextEdge == 0){
        nextEdge = rank;
        for(int i=0; i<processorsCount; i++)
            if(i!=rank)
                MPI_Send(&rank, 1, MPI_INT, i, INTEGER, MPI_COMM_WORLD); 
    }
    else{
        MPI_Recv(&lastEdge, 1, MPI_INT, MPI_ANY_SOURCE, INTEGER, MPI_COMM_WORLD, &status);
    }
    /*--------------------*/
    
    /*  Suffix sum  */
    int value;  // hodnota, poradie preorder hrany
    int tmp;    // dummy
    int end = ceil(log2(processorsCount));  // pocet iteracii
    
    struct recvProc myRecv, recvNeigh;  // reprezentacia vymienanych hodnot v sume suffixov
    struct sendProc mySend, sendNeigh;  // identifikacia odosielaneho procesora
    
    // inicializacia prijimania sprav
    bool doRecv = true;
    
    // prveho procesora sa nik nepyta
    if(rank==0)
        doRecv=false;
 
    // inicializacia vymienanej hodnoty
    value = isForward(&myEdge);
    
    // suma sufixov
    for(int k = 0; k<end; k++){
        // inicializacia struktur
        myRecv.nextEdge = nextEdge;
        myRecv.value = value;
        
        mySend.rank = rank;
        mySend.doRecv = doRecv;
        
        // praca posledneho procesora 
        if(rank == nextEdge){
            lastProcDuty(rank,pow(2,k), myRecv, processorsCount);
        }
        // praca ostatnych
        else{
            // poslem svoju identifikaciu naslednikovy
            MPI_Send(&mySend, sizeof(mySend), MPI_BYTE, nextEdge, PROC, MPI_COMM_WORLD);
            
            // ak sa ma niekto ma pytat tak odpoviem
            if(doRecv){
                MPI_Recv(&sendNeigh, sizeof(struct sendProc), MPI_BYTE, MPI_ANY_SOURCE, PROC, MPI_COMM_WORLD, &status);
                // posli value
                MPI_Send(&myRecv, sizeof(myRecv), MPI_BYTE, sendNeigh.rank, VALUE, MPI_COMM_WORLD);
                doRecv = sendNeigh.doRecv;
            }
            // prijmem spravu od naslednika a upravim si hodnoty
            MPI_Recv(&recvNeigh, sizeof(recvNeigh), MPI_BYTE, nextEdge, VALUE, MPI_COMM_WORLD, &status);

            value += recvNeigh.value;
            nextEdge = recvNeigh.nextEdge;
            
            // synchronizacia
            MPI_Send(&rank, 1, MPI_INT, lastEdge, ACK, MPI_COMM_WORLD);
            MPI_Recv(&tmp, 1, MPI_INT, MPI_ANY_SOURCE, ACK, MPI_COMM_WORLD, &status);
        }
    }
    
    // uprava hodnot
    value = processorsCount - value +1;
    /*--------------*/
    
    // Posleme prvemu procesoru svoje hodnoty
    if(isForward(&myEdge)){
        MPI_Send(&myEdge, sizeof(myEdge), MPI_BYTE, 0, EDGE, MPI_COMM_WORLD);
        MPI_Send(&value, 1, MPI_INT, 0, VALUE, MPI_COMM_WORLD);
    }
    // Prvy vytiskne preorder
    if(rank == 0){
        if(timeCheck){
            timeEnd = MPI_Wtime();
            cout << "Time:" << (timeEnd-timeStart) << endl;   
        }
        else {
            finalPrint(array);
        }

    }
    
    MPI_Finalize(); 
    return 0;
}


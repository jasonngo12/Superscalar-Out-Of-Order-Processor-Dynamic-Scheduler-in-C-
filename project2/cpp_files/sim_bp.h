#ifndef SIM_BP_H
#define SIM_BP_H
#include <vector>
using namespace std;

typedef struct bp_params{
    unsigned long int K;
    unsigned long int M1;
    unsigned long int M2;
    unsigned long int N;
    char*             bp_name;
}bp_params;

// Put additional data structures here as per your requirement

class BimodalPredictor{
    public:
    unsigned long int m;
    //char* trace_file;
    int tableSize;
    int predictions;
    int mispredictions;    
    vector<int> bimodalTable;

    BimodalPredictor(unsigned long int m) : m(m), tableSize(1 << m) {
        bimodalTable.resize(tableSize, 2);
    }

    int predict(unsigned long int addr){
        predictions++;
        int index = (addr >> 2)&(tableSize -1);
        return bimodalTable[index];
    }

    void update(unsigned long int addr, char str, int prediction){
        //predictions++;
        int index = (addr >> 2)&(tableSize -1);
        if(str == 't'){
            if(prediction >= 2) bimodalTable[index] = 3;
            else{
                mispredictions++;
                bimodalTable[index]++;
            } 
        }
        else if(str == 'n'){
            if(prediction <= 1) bimodalTable[index] = 0;
            else {
                mispredictions++;
                bimodalTable[index]--;
            }
        }
    }

    void printBimodal(){
        //printf("COMMMAND\n");
        //printf(" ./sim bimodal %lu %s \n", m, trace_file);
        printf("OUTPUT\n");
        printf(" number of predictions:    %d\n", predictions);
        printf(" number of mispredictions: %d\n", mispredictions);
        printf(" misprediction rate:       %.2f%%\n", (float)(100*mispredictions)/(predictions));
        //printf("%%\n");
        printf("FINAL BIMODAL CONTENTS\n");
        for(long unsigned int i = 0; i < bimodalTable.size(); i++){
            printf("%lu  %d\n",i, bimodalTable[i]);
        }
    }

};

class gshare{
    public:
    unsigned long int m;
    unsigned long int n;
    int tableSize;
    unsigned int BHR;    
    int predictions;
    int mispredictions;    
    vector<int> gshareTable;

    gshare(unsigned long int m, unsigned long int n) : m(m), n(n), tableSize(1 << m) {
        gshareTable.resize(tableSize, 2);
        BHR = 0;
    }

    int predict(unsigned long int addr){
        predictions++;
        unsigned int pc = (addr >> 2) & ((1 << m) - 1); 
        unsigned int upperBits = pc >> (m - n);       
        unsigned int lowerBits = pc & ((1 << (m - n)) - 1); 
        unsigned int xorBits = upperBits ^ BHR;    
        unsigned int index = (xorBits << (m - n)) | lowerBits; 
        return gshareTable[index];
    }

    int update(unsigned long int addr, char str, int prediction){
        int outcome;
        unsigned int pc = (addr >> 2) & ((1 << m) - 1); 
        unsigned int upperBits = pc >> (m - n);       
        unsigned int lowerBits = pc & ((1 << (m - n)) - 1); 
        unsigned int xorBits = upperBits ^ BHR;    
        unsigned int index = (xorBits << (m - n)) | lowerBits; 

        if(str == 't'){
            outcome = 1;
            if(gshareTable[index] >= 2) gshareTable[index] = 3;
            else{
                mispredictions++;
                gshareTable[index]++;
            } 
        }
        else{
            outcome = 0;
            if(gshareTable[index] <= 1) gshareTable[index] = 0;
            else {
                mispredictions++;
                gshareTable[index]--;
            }
        }
        return outcome;
        //BHR = ((BHR >> 1) | (outcome << (n-1))) & ((1 << n) - 1);
        //BHRupdate(outcome);
    }

    void BHRupdate(int outcome){
        BHR = ((BHR >> 1) | (outcome << (n-1))) & ((1 << n) - 1);
    }

    void printgshare(){
        printf("OUTPUT\n");
        printf(" number of predictions:    %d\n", predictions);
        printf(" number of mispredictions: %d\n", mispredictions);
        printf(" misprediction rate:       %.2f%%\n", (float)(100*mispredictions)/(predictions));
        //printf("%%\n");
        printf("FINAL GSHARE CONTENTS\n");
        for(long unsigned int i = 0; i < gshareTable.size(); i++){
            printf("%lu  %d\n",i, gshareTable[i]);
        }
    }

};

class hybrid{
    public:
    unsigned long int k;
    unsigned long int m1;
    unsigned long int n;
    unsigned long int m2;
    BimodalPredictor BP; 
    gshare GP;
    
    
    int tableSize;       
    int predictions;
    int mispredictions;
    vector<int> hybridTable;

    hybrid(unsigned long int k, unsigned long int m1, unsigned long int n, unsigned long int m2) : k(k), m1(m1), n(n), m2(m2), BP(m2), GP(m1, n), tableSize(1 << k) {
        hybridTable.resize(tableSize, 1);       
        
    }

    void predict(unsigned long int addr, char str){
        int index = (addr >> 2)&(tableSize -1);
        predictions++;
        int outcome;
        int BP_predict;
        int GP_predict;
        BP_predict = BP.predict(addr);
        GP_predict = GP.predict(addr);
        
        if(hybridTable[index] >= 2) GP.update(addr, str, GP_predict);
        else BP.update(addr, str, BP_predict);

        //update BHR
        if(str == 't') outcome = 1;
        else outcome = 0;
        GP.BHRupdate(outcome);

        //update branch chooser counter
        if(str == 't'){
            if(GP_predict >=2 && !(BP_predict >= 2)) hybridTable[index] = std::min(3, hybridTable[index]+1);
            else if(!(GP_predict >=2) && BP_predict >= 2) hybridTable[index] = std::max(0, hybridTable[index]-1);
        }
        else if(str == 'n'){
            if(GP_predict >=2 && !(BP_predict >= 2)) hybridTable[index] = std::max(0, hybridTable[index]-1);
            else if(!(GP_predict >=2) && BP_predict >= 2) hybridTable[index] = std::min(3, hybridTable[index]+1);
        }

    }

    void printHybrid(){
        printf("OUTPUT\n");
        printf(" number of predictions:    %d\n", predictions);
        printf(" number of mispredictions: %d\n", GP.mispredictions + BP.mispredictions);
        printf(" misprediction rate:       %.2f%%\n", (float)(100*(GP.mispredictions + BP.mispredictions))/(predictions));
        printf("FINAL CHOOSER CONTENTS\n");
        for(long unsigned int i = 0; i < hybridTable.size(); i++){
            printf("%lu  %d\n",i, hybridTable[i]);
        }
        printf("FINAL GSHARE CONTENTS\n");
        for(long unsigned int i = 0; i < GP.gshareTable.size(); i++){
            printf("%lu  %d\n",i, GP.gshareTable[i]);
        }
        printf("FINAL BIMODAL CONTENTS\n");
        for(long unsigned int i = 0; i < BP.bimodalTable.size(); i++){
            printf("%lu  %d\n",i, BP.bimodalTable[i]);
        }
    }

};




#endif

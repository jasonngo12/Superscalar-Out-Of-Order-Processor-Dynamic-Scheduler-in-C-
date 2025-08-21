#ifndef SIM_PROC_H
#define SIM_PROC_H
#include <queue>
#include <deque>
#include <vector>
using namespace std;


typedef struct proc_params{
    unsigned long int rob_size;
    unsigned long int iq_size;
    unsigned long int width;
}proc_params;

// Put additional data structures here as per your requirement
class Instruction { 
    public:
    unsigned long pc;
    int op;
    int dest;
    int src1;
    int src2;
    int age;
    int FE, DE, RN, RR, DI, IS, EX, WB, RT, RTcount;
    Instruction(unsigned long PC, int OP, int DEST, int SRC1, int SRC2, int AGE) : pc(PC), op(OP), dest(DEST), src1(SRC1), src2(SRC2), age(AGE){
        FE = 0;
        DE = 0;
        RN = 0;
        RR = 0;
        DI = 0;
        IS = 0;
        EX = 0;
        WB = 0;
        RT = 0;
        RTcount = 0;
    }

};

class ROBentry{
    public:
    int valid;
    unsigned long pc;
    int dest;
    int ready;
    int age;
    ROBentry() : valid(0), pc(-1), dest(-1), ready(0), age(-1){}
    ROBentry(int VALID, unsigned long PC, int DEST, int READY, int AGE) : valid(VALID), pc(PC), dest(DEST), ready(READY), age(AGE){}
};

class IQentry{
    public:
    int valid;
    int dest;
    int ready1;
    int ready2;
    int src1;
    int src2;
    int age;
    int latency;
    IQentry() : valid(0), dest(-1), ready1(0), ready2(0), src1(0), src2(0), age(0), latency(0){}
    IQentry(int VALID, int DEST, int READY1, int READY2, int SRC1, int SRC2, int AGE, int LATENCY) : valid(VALID), dest(DEST), ready1(READY1), ready2(READY2), src1(SRC1), src2(SRC2), age(AGE), latency(LATENCY){}
};

class ROB{
    public:
    unsigned int head;
    unsigned int tail;
    int numElements;
    vector<ROBentry> robVector;
    
    ROB(int HEAD, int TAIL, int ROB_SIZE) : head(HEAD), tail(TAIL){
        numElements = 0;
        robVector.resize(ROB_SIZE);
    }
};



class RMTentry{
    public:
    int valid;
    unsigned int ROBtag;
    RMTentry() : valid(0), ROBtag(0){}
};

class Simulator {
    public:
    unsigned int ROB_SIZE;
    unsigned int IQ_SIZE;
    unsigned int WIDTH;
    int register_count;
    int cycles;
    int iqElements;
    size_t instIndex;
    vector<Instruction> instructionsVector;
    queue<Instruction> fetchQueue;
    queue<Instruction> decodeQueue;
    queue<Instruction> renameQueue;
    queue<IQentry> regReadQueue;
    queue<IQentry> dispatchQueue;
    vector<IQentry> issueVector;
    queue<IQentry> executeQueue;
    queue<IQentry> writebackQueue;
    //queue<IQentry> retireQueue;

    //vector<int> ARF;  
    vector<RMTentry> RMT;  
    //queue<ROBentry> ROB;
    ROB Rob;


    Simulator(unsigned int rob_size, unsigned int iq_size, unsigned int width)  : ROB_SIZE(rob_size), IQ_SIZE(iq_size), WIDTH(width), Rob(0, 0, rob_size){
        //ARF.resize(register_count, -1);  
        register_count = 67;
        cycles = 0;
        instIndex = 0;
        RMT.resize(register_count);  
        issueVector.resize(iq_size);
        //Rob = ROB(0, 0, rob_size);
        //ROB.resize(rob_size);
    }

    bool Advance_Cycle(){
        cycles++;
        //instIndex == instructionsVector.size() && Rob.numElements == 0
        if((instIndex == instructionsVector.size()) && (Rob.numElements == 0)) return false;
        else return true;

    }

    void Fetch(){
        if(instructionsVector.size() == instIndex || !decodeQueue.empty()) return;  //possible seg fault
        else if((instIndex < instructionsVector.size()) && decodeQueue.empty()){ 
            unsigned int fetchCount = 0;
            while(instIndex < instructionsVector.size() && (fetchCount < WIDTH)){
                decodeQueue.push(instructionsVector[instIndex]);                         
                //instructionsVector.erase(instructionsVector.begin());    
                instructionsVector[instIndex].FE = cycles;                
                instructionsVector[instIndex].DE = cycles + 1;
                instIndex++;
                fetchCount++;
            }
            return;
        }        
    }

    void Decode(){
        if(!decodeQueue.empty()){
            if(!renameQueue.empty()) return;
            else{
                while(!decodeQueue.empty()){
                    Instruction inst = decodeQueue.front();
                    renameQueue.push(inst);
                    decodeQueue.pop();
                    instructionsVector[inst.age].RN = cycles+1;
                }
            }
        }
    }

    void Rename(){
        if(!renameQueue.empty()){
            if(!regReadQueue.empty() || ROB_SIZE - Rob.numElements < renameQueue.size()) return;
            else if(regReadQueue.empty() && ROB_SIZE - Rob.numElements >= renameQueue.size()){
                while(!renameQueue.empty()){
                    int latency;
                    Instruction inst = renameQueue.front();                    
                    renameQueue.pop();
                    
                    //add code for if tail = head and head is occupied
                    ROBentry robEntry(1, inst.pc, inst.dest, 0, inst.age);
                    Rob.robVector[Rob.tail] = robEntry;
                    Rob.numElements++;
                
                    
                    if(inst.src1 != -1){
                        if(RMT[inst.src1].valid == 1) inst.src1 = RMT[inst.src1].ROBtag; //assigned ROB index
                        else inst.src1 = -2;  //assigned ARF value 
                    }
                    if(inst.src2 != -1){
                        if(RMT[inst.src2].valid == 1) inst.src2 = RMT[inst.src2].ROBtag;  //assigned ROB index
                        else inst.src2 = -2;      //assigned ARF value;
                    }

                    //dest is always assigned ROB entry    
                    if(inst.dest>=0){ // make sure dest is not -1 or -2 
                        RMT[inst.dest].ROBtag = Rob.tail;
                        RMT[inst.dest].valid = 1;
                    }
                    
                    inst.dest = Rob.tail; //might not need this?
                    
                    if(Rob.tail == ROB_SIZE - 1) Rob.tail = 0;
                    else Rob.tail++;

                    //push instruction to 
                    if(inst.op == 0) latency = 1;
                    else if(inst.op == 1) latency = 2;
                    else latency = 5;
                    IQentry iqEntry(1, inst.dest, 0, 0, inst.src1, inst.src2, inst.age, latency);
                    regReadQueue.push(iqEntry);
                    instructionsVector[inst.age].RR = cycles+1;
                }
            }
        }
    }

    void RegRead(){
        if(!regReadQueue.empty()){
            if(!dispatchQueue.empty()) return;
            else{
                while(!regReadQueue.empty()){ //how many inst do we send to dispatch
                    
                    IQentry inst = regReadQueue.front();
                    regReadQueue.pop();
                    //IQentry() : valid(0), dest(-1), ready1(0), ready2(0){}
                    //int ready1 = 0;
                    //int ready2 = 0;

                    //check ready bit of src1
                    if(inst.ready1 != 1){ // check if ready bit was already set from execute stage
                        if(inst.src1 == -1) //if no src reg
                            inst.ready1 = 1; 
                        else if(inst.src1 == -2) //value stored in ARF
                            inst.ready1 = 1;
                        else{
                            if(Rob.robVector[inst.src1].ready == 1) inst.ready1 = 1;
                            else inst.ready1 = 0;                        
                        }
                    }
                    

                    //check ready bit of src2
                    if(inst.ready2 != 1){ // check if ready bit was already set from execute stage
                        if(inst.src2 == -1) 
                            inst.ready2 = 1; 
                        else if(inst.src2 == -2) 
                            inst.ready2 = 1;
                        else{
                            if(Rob.robVector[inst.src2].ready == 1) inst.ready2 = 1;
                            else inst.ready2 = 0;
                        }
                    }
                                        
                                        
                    //RMT[inst.dest].ROBtag
                    //IQentry iqEntry(1, inst.dest, ready1, ready2, inst.src1, inst.src2, inst.age, latency); //when instr gets pushed to IQ table do we init as invalid
                    dispatchQueue.push(inst);
                    instructionsVector[inst.age].DI = cycles+1;
                    
                }                
            }
        }
    }

    void Dispatch(){
        if(!dispatchQueue.empty()){
            if(IQ_SIZE - iqElements < dispatchQueue.size()) return;
            else{
                while(!dispatchQueue.empty()){
                    IQentry iqEntry = dispatchQueue.front();
                    dispatchQueue.pop();
                    //find invalid entries in issue vector to insert new iqEntries
                    for(unsigned int i = 0; i<IQ_SIZE; i++){
                        if(issueVector[i].valid == 0){                            
                            issueVector[i] = iqEntry;
                            instructionsVector[iqEntry.age].IS = cycles+1;
                            iqElements++;
                            break;
                        }
                    }                    
                }                
            }
        }
    }

    void Issue(){
        IQentry iqTarget;
        unsigned int issued = 0;
        int index = 0;
        bool execute = false;
        while((issued < WIDTH)){
            //((5*WIDTH) - executeQueue.size() > 0)
            for(unsigned int i = 0; i < IQ_SIZE; i++){
                if(issueVector[i].valid == 1 && issueVector[i].ready1 == 1 && issueVector[i].ready2 == 1){
                    index = i;
                    iqTarget = issueVector[i];
                    execute = true;
                    //issueVector[i].valid = 0;
                    break;
                }
            }
            for(unsigned int x = 0; x < IQ_SIZE; x++){ //start x at i +1?
                if(issueVector[x].valid == 1 && issueVector[x].ready1 == 1 && issueVector[x].ready2 == 1){
                    if(issueVector[x].age < iqTarget.age){
                        index = x;
                        iqTarget = issueVector[x];  
                        //execute = 1;                      
                    }
                }
            }
            if(execute){
                issueVector[index].valid = 0;
                iqElements--;
                issued++;
                executeQueue.push(iqTarget);
                instructionsVector[iqTarget.age].EX = cycles+1;
                execute = false;
            }
            else break;
            
        }      
    }

    void Execute(){
        queue<IQentry> tempQueue;
        while(!executeQueue.empty()){
            IQentry temp = executeQueue.front();
            executeQueue.pop();
            if(temp.latency != 1){
                temp.latency--;                
                tempQueue.push(temp);
            }
            else {
                //add instruction to writeback queue
                writebackQueue.push(temp);
                instructionsVector[temp.age].WB = cycles+1;

                //set ready bits in issue vector
                for(unsigned int i = 0; i<IQ_SIZE; i++){
                    if(issueVector[i].valid == 1){
                        if(issueVector[i].src1 == temp.dest) issueVector[i].ready1 = 1;
                        if(issueVector[i].src2 == temp.dest) issueVector[i].ready2 = 1;                        
                    }
                }

                //set ready bits in dispatch queue (bundle?)
                deque<IQentry> tempQueue2;
                queue<IQentry> tempQueueCopy = dispatchQueue;
                while (!tempQueueCopy.empty()) {
                    tempQueue2.push_back(tempQueueCopy.front());
                    tempQueueCopy.pop();
                }
                for(auto it = tempQueue2.begin(); it != tempQueue2.end(); it++){
                    if(it->valid == 1){
                        if(it->src1 == temp.dest) it->ready1 = 1;
                        if(it->src2 == temp.dest) it->ready2 = 1;                        
                    }
                } 

                //restore dispatch queue  
                for(auto it = tempQueue2.begin(); it != tempQueue2.end(); it++){
                    tempQueueCopy.push(*it);
                }  
        
                dispatchQueue = tempQueueCopy;

                //set ready bits in regReadQueue
                deque<IQentry> tempQueue3;
                queue<IQentry> tempQueueCopy2 = regReadQueue;
                while (!tempQueueCopy2.empty()) {
                    tempQueue3.push_back(tempQueueCopy2.front());
                    tempQueueCopy2.pop();
                }
                for(auto it = tempQueue3.begin(); it != tempQueue3.end(); it++){
                    if(it->valid == 1){
                        if(it->src1 == temp.dest) it->ready1 = 1;
                        if(it->src2 == temp.dest) it->ready2 = 1;                        
                    }
                } 

                //restore regReadQueue  
                for(auto it = tempQueue3.begin(); it != tempQueue3.end(); it++){
                    tempQueueCopy2.push(*it);
                }  
        
                regReadQueue = tempQueueCopy2;

            }
        }
        executeQueue = tempQueue;
    }

    void Writeback(){
        while(!writebackQueue.empty()){
            IQentry temp = writebackQueue.front();
            writebackQueue.pop();
            //retireQueue.push(temp);
            instructionsVector[temp.age].RT = cycles+1;
            Rob.robVector[temp.dest].ready = 1;
        }
    }

    void Retire(){
        unsigned int retired = 0;
        int RMTindex;
        while(retired < WIDTH){
            //Rob.robVector[Rob.head].ready == 0 || 
            if(Rob.robVector[Rob.head].ready == 0 || Rob.robVector[Rob.head].valid == 0) return;
            else{
                RMTindex = Rob.robVector[Rob.head].dest;
                //check rmt table
                if(RMT[RMTindex].ROBtag == Rob.head && RMTindex >= 0) {
                    RMT[RMTindex].valid = 0;
                    RMT[RMTindex].ROBtag = -1;                    
                }  
                //cycles count in retire register
                instructionsVector[Rob.robVector[Rob.head].age].RTcount = cycles+1;
                Rob.robVector[Rob.head].pc = -1;
                Rob.robVector[Rob.head].dest = -1;
                //Rob.robVector[Rob.head].ready = 0;
                Rob.robVector[Rob.head].valid = 0;
                Rob.robVector[Rob.head].age = -1;
                
                if(Rob.head == ROB_SIZE - 1) Rob.head = 0;
                else Rob.head++;

                //Rob.head++;                    
                Rob.numElements--;
                retired++;  
                            
            }
        }
    }

    void Print(){
        for(unsigned int i = 0; i < instructionsVector.size(); i++){
            Instruction inst = instructionsVector[i];
            printf("%d fu{%d} src{%d,%d} dst{%d} ", inst.age, inst.op, inst.src1, inst.src2, inst.dest);
            printf("FE{%d,%d} ", inst.FE, inst.DE-inst.FE);
            printf("DE{%d,%d} ", inst.DE, inst.RN-inst.DE);
            printf("RN{%d,%d} ", inst.RN, inst.RR-inst.RN);
            printf("RR{%d,%d} ", inst.RR, inst.DI-inst.RR);
            printf("DI{%d,%d} ", inst.DI, inst.IS-inst.DI);
            printf("IS{%d,%d} ", inst.IS, inst.EX-inst.IS);
            printf("EX{%d,%d} ", inst.EX, inst.WB-inst.EX);
            printf("WB{%d,%d} ", inst.WB, inst.RT-inst.WB);
            printf("RT{%d,%d}\n", inst.RT, inst.RTcount-inst.RT);
        }
    }

};

#endif

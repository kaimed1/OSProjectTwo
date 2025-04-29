// CS 3113 Professor Radhakrishnan
// @authors: Xavier Williams - University of Oklahoma 2025

#include <iostream>
#include <queue>
#include <string>
#include <tuple>

using namespace std;

struct PCB {
    int processID;
    // 0 = new, 1 = ready, 2 = running, 3 = i/o waiting, 4 = terminated
    int state;
    int programCounter;
    int instructionBase;
    int dataBase;
    int memoryLimit;
    int cpuCyclesUsed;
    int registerValue;
    int maxMemoryNeeded;
    int mainMemoryBase;
    vector<int> logicalMemory;
};

// Global variables
int cpuAllocated;
int contextSwitchTime;
int globalClock = 0;
int* startTimes; // array to keep track of process start times

void loadJobsToMemory(queue<PCB> &newJobQueue, queue<pair<int, int>> &readyQueue, int* mainMemory, int maxMemory) {
    int currentAddress = 0;
    while(!newJobQueue.empty()) {
        PCB newJob = newJobQueue.front();
        newJobQueue.pop();
        if(newJob.memoryLimit <= maxMemory - currentAddress) {
            // Adjust the PCB fields to reflect the new memory location
            newJob.mainMemoryBase = currentAddress;
            newJob.state = 1;
            newJob.instructionBase = newJob.instructionBase + newJob.mainMemoryBase;
            newJob.dataBase = newJob.dataBase + newJob.mainMemoryBase;

            // Load the job into memory
            mainMemory[currentAddress] = newJob.processID;
            mainMemory[currentAddress + 1] = newJob.state;
            //mainMemory[currentAddress + 2] = newJob.instructionBase; // set program counter to instruction base during load stage
            mainMemory[currentAddress + 2] = newJob.programCounter;
            mainMemory[currentAddress + 3] = newJob.instructionBase;
            mainMemory[currentAddress + 4] = newJob.dataBase;
            mainMemory[currentAddress + 5] = newJob.memoryLimit;
            mainMemory[currentAddress + 6] = newJob.cpuCyclesUsed;
            mainMemory[currentAddress + 7] = newJob.registerValue;
            mainMemory[currentAddress + 8] = newJob.maxMemoryNeeded;
            mainMemory[currentAddress + 9] = newJob.mainMemoryBase;

            currentAddress += 10;
            for(int i = 0; i < newJob.logicalMemory.size(); i++) {
                mainMemory[currentAddress + i] = newJob.logicalMemory[i];
            }
            readyQueue.push(pair(newJob.mainMemoryBase, newJob.dataBase));
            currentAddress += newJob.memoryLimit;
        } else {
            cout << "Error: Not enough memory to load job " << newJob.processID << endl;
        }
    }
}

// Since all values are being updated in main memory during execution, we just need to update the state and push the job back to the ready queue
void timeOutInterrupt(int startAddress, int dataPointer, int* mainMemory, queue<pair<int, int>> &readyQueue) { 
    mainMemory[startAddress + 1] = 1; // change state to ready
    readyQueue.push(pair(startAddress, dataPointer));
    cout << "Process "<< mainMemory[startAddress] << " has a TimeOUT interrupt and is moved to the ReadyQueue." << endl;
}

// We need to send the I/O waiting queue the start address, data pointer, cycles, and time of entry
void ioInterrupt(int startAddress, int dataPointer, int* mainMemory, int exitTime, queue<tuple<int, int, int>> &IOWaitingQueue) {
    mainMemory[startAddress + 1] = 3; // change state to i/o waiting
    IOWaitingQueue.push(make_tuple(startAddress, dataPointer, exitTime));
    cout << "Process "<< mainMemory[startAddress] << " issued an IOInterrupt and moved to the IOWaitingQueue." << endl;
}

// Pop each job from the queue, check if globalClock > exitTime, then either move to ready queue or push back to IOWaitingQueue
void checkWaitingQueue(queue<tuple<int, int, int>> &IOWaitingQueue, queue<pair<int, int>> &readyQueue, int* mainMemory) {
    int originalSize = IOWaitingQueue.size();
    for(int i = 0; i < originalSize; i++){
        tuple<int, int, int> currentJob = IOWaitingQueue.front();
        IOWaitingQueue.pop();
        if(globalClock >= get<2>(currentJob)){
            cout << "print" << endl;
            cout << "Process "<< mainMemory[get<0>(currentJob)] << " completed I/O and is moved to the ReadyQueue." << endl;
            mainMemory[get<0>(currentJob) + 1] = 1; // change state to ready
            readyQueue.push(pair(get<0>(currentJob), get<1>(currentJob)));
        } else {
            IOWaitingQueue.push(currentJob);
        }
    }
}

void executeCPU(int startAddress, int dataPointer, int* mainMemory, queue<pair<int, int>> &readyQueue, queue<tuple<int, int, int>> &IOWaitingQueue) {
    int processID = mainMemory[startAddress];
    int state = mainMemory[startAddress + 1];
    int programCounter = mainMemory[startAddress + 2];
    int instructionBase = mainMemory[startAddress + 3];
    int dataBase = mainMemory[startAddress + 4];
    int memoryLimit = mainMemory[startAddress + 5];
    int cpuCyclesUsed = mainMemory[startAddress + 6];
    int registerValue = mainMemory[startAddress + 7];
    int maxMemoryNeeded = mainMemory[startAddress + 8];
    int mainMemoryBase = mainMemory[startAddress + 9];

    if(programCounter == 0) { // if program counter is at the instruction base, set it to the next instruction
        programCounter = instructionBase;
    }
    mainMemory[startAddress + 2] = programCounter;

    state = 2; // change state to running
    mainMemory[startAddress + 1] = state;

    int currentCycles = 0; //keep track of how many cycles are used in the current execution

    // If this is the first time the process is running, record the start time
    if(startTimes[processID - 1] == -1) {
        startTimes[processID - 1] = globalClock;
    }
    bool ioFlag = false;
    while(programCounter < dataBase && ioFlag == false) {
        int instruction = mainMemory[programCounter];
        switch (instruction){
            case 1: {
                cout << "compute" << endl;
                int iterations = mainMemory[dataPointer];
                dataPointer++;
                int cycles = mainMemory[dataPointer];
                dataPointer++;
                cpuCyclesUsed += cycles;
                currentCycles += cycles;
                globalClock += cycles;
                mainMemory[startAddress + 6] = cpuCyclesUsed;
                programCounter++;
                mainMemory[startAddress + 2] = programCounter;
                break;
            }
            case 2: {
                int cycles = mainMemory[dataPointer];
                dataPointer++;
                cpuCyclesUsed += cycles;
                mainMemory[startAddress + 6] = cpuCyclesUsed;
                programCounter++;
                mainMemory[startAddress + 2] = programCounter;
                int exitTime = globalClock + cycles;
                ioInterrupt(startAddress, dataPointer, mainMemory, exitTime, IOWaitingQueue);
                ioFlag = true; // breaks the loop, forcing a context switch
                break;
            }
            case 3: {
                int value = mainMemory[dataPointer];
                dataPointer++;
                registerValue = value;
                mainMemory[startAddress + 7] = registerValue;
                int address = mainMemory[dataPointer];
                dataPointer++;
                address += mainMemoryBase + 10;
                if(address >= dataBase && address < startAddress + 10 + memoryLimit) {
                    cout << "stored" << endl;
                    mainMemory[address] = value;
                } else {
                    cout << "store error!" << endl;
                }
                cpuCyclesUsed++;
                currentCycles++;
                globalClock++;
                mainMemory[startAddress + 6] = cpuCyclesUsed;
                programCounter++;
                mainMemory[startAddress + 2] = programCounter;
                break;
            }
            case 4: {
                int address = mainMemory[dataPointer];
                dataPointer++;
                address += mainMemoryBase + 10;
                if(address >= dataBase && address < startAddress + 10 + memoryLimit) {
                    cout << "loaded" << endl;
                    registerValue = mainMemory[address];
                    mainMemory[startAddress + 7] = registerValue;
                } else {
                    cout << "load error!" << endl;
                }
                cpuCyclesUsed++;
                currentCycles++;
                globalClock++;
                mainMemory[startAddress + 6] = cpuCyclesUsed;
                programCounter++;
                mainMemory[startAddress + 2] = programCounter;
                break;
            }
        }

        // After each instruction, check if process is over time and still has instructions remaining
        if(currentCycles >= cpuAllocated && programCounter < dataBase) { 
            timeOutInterrupt(startAddress, dataPointer, mainMemory, readyQueue);
            break;
        }
    }

    if(programCounter >= dataBase && ioFlag == false) { // check if process has terminated; ioFlag is used to prevent double termination when print is the last instruction
        programCounter = instructionBase-1;
        mainMemory[startAddress + 2] = programCounter;
        state = 4; // change state to terminated
        mainMemory[startAddress + 1] = state;

        // Output PCB details after execution
        cout << "Process ID: " << mainMemory[startAddress] << endl;
        cout << "State: " << "TERMINATED" << endl;
        cout << "Program Counter: " << mainMemory[startAddress + 2] << endl;
        cout << "Instruction Base: " << mainMemory[startAddress + 3] << endl;
        cout << "Data Base: " << mainMemory[startAddress + 4] << endl;
        cout << "Memory Limit: " << mainMemory[startAddress + 5] << endl;
        cout << "CPU Cycles Used: " << mainMemory[startAddress + 6] << endl;
        cout << "Register Value: " << mainMemory[startAddress + 7] << endl;
        cout << "Max Memory Needed: " << mainMemory[startAddress + 8] << endl;
        cout << "Main Memory Base: " << mainMemory[startAddress + 9] << endl;
        cout << "Total CPU Cycles Consumed: " << globalClock - startTimes[processID - 1] << endl;        
        cout << "Process " << processID << " terminated. Entered running state at: " << startTimes[processID-1] << 
                ". Terminated at: " << globalClock << ". Total execution time: " << globalClock-startTimes[processID-1] << "." << endl;
    }

}

int main() {
    int maxMemory;
    int numProcesses;
    queue<PCB> newJobQueue;
    queue<pair<int, int>> readyQueue;
    queue<tuple<int, int, int>> IOWaitingQueue; // tuple is (startAddress, dataPointer, time of exit)
    int* mainMemory;

    // Step 1: Read and parse input file
    cin >> maxMemory >> cpuAllocated >> contextSwitchTime;
    cin >> numProcesses;
    // mainMemory = new int[maxMemory]; 
    // for(int i = 0; i < maxMemory; i++) {
    //     mainMemory[i] = -1;
    // }
    // startTimes = new int[numProcesses]; 
    // for(int i = 0; i < numProcesses; i++) {
    //     startTimes[i] = -1;
    // }
    for(int i = 0; i < numProcesses; i++) {
        // Each process gets a new PCB struct, initialize all fields
        PCB newJob;
        cin >> newJob.processID;
        newJob.state = 0;
        newJob.programCounter = 0;
        newJob.cpuCyclesUsed = 0;
        newJob.registerValue = 0;
        newJob.instructionBase = 10;
        cin >> newJob.maxMemoryNeeded;
        newJob.memoryLimit = newJob.maxMemoryNeeded;
        int numInstructions;
        cin >> numInstructions;
        newJob.dataBase = newJob.instructionBase + numInstructions;

        // Make enough space for the instructions in the local memory
        for(int j = 0; j < numInstructions; j++) {
            newJob.logicalMemory.push_back(0);
        }
        // Store each instruction in the previously padded space, then store corresponding data sequentially after the instruction space
        for(int j = 0; j < numInstructions; j++) {
            int instruction;
            cin >> instruction;
            newJob.logicalMemory[j] = instruction;
            int data;
            // Even encoded instructions take 1 data value, odd encoded instructions take 2
            if(instruction % 2 == 0){
                cin >> data;
                newJob.logicalMemory.push_back(data);
            } else {
                cin >> data;
                newJob.logicalMemory.push_back(data);
                cin >> data;
                newJob.logicalMemory.push_back(data);
            }
        }
        newJobQueue.push(newJob);
    }

    // Step 2: Load jobs into main memory
    loadJobsToMemory(newJobQueue, readyQueue, mainMemory, maxMemory);

    // Step 3: After you load the jobs in the queue go over the main memory and print the content of mainMemory
    for(int i = 0; i < maxMemory; i++) {
        cout << i << " : " << mainMemory[i] << endl;
    }

    // Step 4: Process execution
    
    while (!readyQueue.empty() || !IOWaitingQueue.empty()) { // need to check both queues
        globalClock += contextSwitchTime; // initial context switch

        // Only run executeCPU if there are jobs in the ready queue
        if(!readyQueue.empty()) {
            int startAddress = readyQueue.front().first;
            int dataPointer = readyQueue.front().second;
            readyQueue.pop();

            // Execute the job
            cout << "Process " << mainMemory[startAddress] << " has moved to Running." << endl;
            executeCPU(startAddress, dataPointer, mainMemory, readyQueue, IOWaitingQueue); // executeCPU will always result in an interrupt or termination
        }

        // Check if any jobs in the IOWaitingQueue are ready to move back to the ready queue
        checkWaitingQueue(IOWaitingQueue, readyQueue, mainMemory);
    }
    globalClock += contextSwitchTime; // final context switch
    cout << "Total CPU time used: " << globalClock << endl;



    return 0;
}
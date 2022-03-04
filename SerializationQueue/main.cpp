//
//  main.cpp
//  SerializationQueue
//
//  Created by Samuel Donovan on 3/3/22.
//

#include <iostream>
#include <queue>
#include <condition_variable>
#include <thread>

using namespace std;

void printA() {
    cout << 'A' << endl;
}

void printB() {
    this_thread::sleep_for(chrono::seconds(1));
    cout << 'B' << endl;
}

struct SerialQueue {
    
    mutex workM;
    bool terminateUponEmpty;
    queue<void(*)()> work;
    
    thread workThread;
    
    mutex newJobOrTerminateM;
    condition_variable newJobOrTerminate;
    
    SerialQueue() : terminateUponEmpty(false) {
        workThread = thread([this]{
            handleJobs();
        });
    }
    
    void terminate() {
        workM.lock();
        terminateUponEmpty = true;
        newJobOrTerminate.notify_one();
        workM.unlock();
        workThread.join();
    }
    
    void handleJobs() {
        
        unique_lock<mutex> lck(newJobOrTerminateM);
        
    checkForNewJob:
        workM.lock();
        if (work.empty()) {
            if (terminateUponEmpty) {
                workM.unlock();
                return;
            }
            workM.unlock();
        } else {
            void(* const job)() = work.front();
            work.pop();
            workM.unlock();
            job();
            goto checkForNewJob;
        }
        newJobOrTerminate.wait(lck);
        goto checkForNewJob;
    }
    
    void addJob(void(*job)()) {
        workM.lock();
        work.push(job);
        newJobOrTerminate.notify_one();
        workM.unlock();
    }
};

int main(int argc, const char * argv[]) {
    SerialQueue sq;
    
    this_thread::sleep_for(chrono::seconds(1));
    sq.addJob(printA);
    this_thread::sleep_for(chrono::seconds(1));
    sq.addJob(printB);
    
    sq.terminate();
}

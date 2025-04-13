#include <iostream>
#include <thread>
#include <vector>
#include <random>
#include <chrono>
#include <queue>
#include "semaphore.h"

using namespace std;

// --- Simulation Settings ---
const int NUM_TELLERS = 3;
const int NUM_CUSTOMERS = 50;
const int MAX_SAFE_CAPACITY = 2; // How many tellers can be in the safe at once

// Simple way to identify transaction types
enum TransactionType {
    DEPOSIT,
    WITHDRAWAL
};

// --- Global Shared Resources & Synchronization Primitives ---

// Semaphores controlling access to shared resources
Semaphore bankOpenSem(0);       
Semaphore safeSem(MAX_SAFE_CAPACITY); 
Semaphore managerSem(1);       
Semaphore doorSem(2);          
Semaphore printSem(1);         

// Tools for managing the customer waiting line
Semaphore queueMutex(1);       
Semaphore tellerAvailableSem(0); 
queue<int> customerQueue;

// Tracking teller state
vector<bool> tellerAvailable(NUM_TELLERS, false); 
vector<int> tellerCustomer(NUM_TELLERS, -1);

// Semaphores for detailed step-by-step coordination between a specific teller and their assigned customer
vector<Semaphore*> tellerReadySem;    
vector<Semaphore*> customerReadySem;  
vector<Semaphore*> askTransactionSem; 
vector<Semaphore*> tellTransactionSem;
vector<Semaphore*> transactionDoneSem;
vector<Semaphore*> customerLeaveSem; 

// Stores the transaction type requested by the customer assigned to a specific teller
vector<TransactionType> customerTransactions(NUM_TELLERS);

// Keeping track of simulation progress
int customersServed = 0;       
Semaphore customerCountSem(1); 

// Random number generation for simulating variability
mt19937 rng(chrono::steady_clock::now().time_since_epoch().count());

// --- Helper Functions ---

// Prints a message to the console safely from any thread
void syncPrint(const string& msg) {
    printSem.wait();
    cout << msg << endl;
    printSem.signal();
}

// Pauses the current thread for a random duration to simulate work/travel time
void randomSleep(int min_ms, int max_ms) {
    if (min_ms >= max_ms) {
         if (min_ms > 0) this_thread::sleep_for(chrono::milliseconds(min_ms));
         return;
    }
    uniform_int_distribution<int> dist(min_ms, max_ms);
    this_thread::sleep_for(chrono::milliseconds(dist(rng)));
}

// --- Teller Logic ---
// This function defines the behavior of each teller thread
void teller(int id) {
    syncPrint("Teller " + to_string(id) + " []: ready to serve");    
    bankOpenSem.signal();
    while (true) {
        syncPrint("Teller " + to_string(id) + " []: waiting for a customer");
        queueMutex.wait();
        tellerAvailable[id] = true;

        if (!customerQueue.empty()) {
            int customerId = customerQueue.front();
            customerQueue.pop();
            tellerCustomer[id] = customerId; 
            tellerAvailable[id] = false;     
            queueMutex.signal();             
            tellerReadySem[id]->signal();
            customerReadySem[id]->wait();
        } else {
            queueMutex.signal();
            tellerAvailableSem.signal();
            tellerReadySem[id]->wait();
            customerReadySem[id]->wait();
            queueMutex.wait();
            if (tellerCustomer[id] == -1) { 
                queueMutex.signal();
                break;
            }
            queueMutex.signal();
        }

        int custId = tellerCustomer[id];

        // ---- Serve the customer ----
        syncPrint("Teller " + to_string(id) + " [Customer " + to_string(custId) + "]: serving a customer");

        // Coordinate asking for the transaction type
        syncPrint("Teller " + to_string(id) + " [Customer " + to_string(custId) + "]: asks for transaction");
        askTransactionSem[id]->signal();
        tellTransactionSem[id]->wait(); 

        // Retrieve the transaction type the customer provided
        TransactionType transType = customerTransactions[id];

        // ---- Perform the transaction ----
        if (transType == DEPOSIT) {
            syncPrint("Teller " + to_string(id) + " [Customer " + to_string(custId) + "]: handling deposit transaction");
            syncPrint("Teller " + to_string(id) + " [Customer " + to_string(custId) + "]: going to safe");
            safeSem.wait(); // Wait if safe capacity is reached
            syncPrint("Teller " + to_string(id) + " [Customer " + to_string(custId) + "]: enter safe");
            randomSleep(10, 50); // Simulate work inside the safe
            syncPrint("Teller " + to_string(id) + " [Customer " + to_string(custId) + "]: leaving safe");
            safeSem.signal(); // Release spot in the safe
            syncPrint("Teller " + to_string(id) + " [Customer " + to_string(custId) + "]: finishes deposit transaction.");
        } else { // WITHDRAWAL
            syncPrint("Teller " + to_string(id) + " [Customer " + to_string(custId) + "]: handling withdrawal transaction");
            
            // Access the manager (shared resource)
            syncPrint("Teller " + to_string(id) + " [Customer " + to_string(custId) + "]: going to the manager");
            managerSem.wait(); // Wait if manager is busy
            syncPrint("Teller " + to_string(id) + " [Customer " + to_string(custId) + "]: getting manager's permission");
            randomSleep(5, 30); // Simulate talking to the manager
            syncPrint("Teller " + to_string(id) + " [Customer " + to_string(custId) + "]: got manager's permission");
            managerSem.signal(); // Release the manager

            // Access the safe (shared resource)
            syncPrint("Teller " + to_string(id) + " [Customer " + to_string(custId) + "]: going to safe");
            safeSem.wait(); // Wait if safe capacity is reached
            syncPrint("Teller " + to_string(id) + " [Customer " + to_string(custId) + "]: enter safe");
            randomSleep(10, 50); // Simulate work inside the safe
            syncPrint("Teller " + to_string(id) + " [Customer " + to_string(custId) + "]: leaving safe");
            safeSem.signal(); // Release spot in the safe

            syncPrint("Teller " + to_string(id) + " [Customer " + to_string(custId) + "]: finishes withdrawal transaction.");
        }

        // ---- Finalize interaction ----
        syncPrint("Teller " + to_string(id) + " [Customer " + to_string(custId) + "]: wait for customer to leave.");
        transactionDoneSem[id]->signal(); 
        customerLeaveSem[id]->wait();   

        // ---- Update overall count ----
        customerCountSem.wait();
        customersServed++;
        customerCountSem.signal();
    }

    syncPrint("Teller " + to_string(id) + " []: leaving for the day");
}

// --- Customer Logic ---
void customer(int id) {
    // Customer decides what they want to do
    uniform_int_distribution<int> transaction_dist(0, 1);
    TransactionType transactionType = static_cast<TransactionType>(transaction_dist(rng));

    if (transactionType == DEPOSIT) {
        syncPrint("Customer " + to_string(id) + " []: wants to perform a deposit transaction");
    } else {
        syncPrint("Customer " + to_string(id) + " []: wants to perform a withdrawal transaction");
    }

    // Simulate time before the customer arrives at the bank
    randomSleep(0, 100);
    syncPrint("Customer " + to_string(id) + " []: going to bank.");

    // Use the 'door' semaphore to potentially limit entry rate
    doorSem.wait();
    syncPrint("Customer " + to_string(id) + " []: entering bank.");
    doorSem.signal(); 

    syncPrint("Customer " + to_string(id) + " []: getting in line.");

    // --- Find a teller ---
    int assignedTeller = -1;

    queueMutex.wait(); 
    // Check if any teller is marked as available right now
    for (int i = 0; i < NUM_TELLERS; i++) {
        if (tellerAvailable[i]) {
            assignedTeller = i;          
            tellerAvailable[i] = false;  
            tellerCustomer[i] = id;      
            break;                      
        }
    }

    if (assignedTeller == -1) {
        // No teller was free, so join the queue
        customerQueue.push(id);
        queueMutex.signal(); 

        // Wait for the signal that *some* teller is available
        tellerAvailableSem.wait();

        // Woke up! Now need to find which teller became free.
        // WARNING: Race condition potential here! Another customer might grab the teller first.
        queueMutex.wait(); 
        for (int i = 0; i < NUM_TELLERS; i++) {
            if (tellerAvailable[i]) {
                assignedTeller = i;
                tellerAvailable[i] = false;
                tellerCustomer[i] = id;
                break;
            }
        }
        // It's possible assignedTeller is still -1 if another customer was faster,
        // but this code assumes it finds one.
        queueMutex.signal(); 
    } else {
        // Found a teller immediately in the first check
        queueMutex.signal(); 
    }

    // --- Interact with the assigned teller ---
    syncPrint("Customer " + to_string(id) + " []: selecting a teller."); 
    // Specific messages for the interaction
    syncPrint("Customer " + to_string(id) + " [Teller " + to_string(assignedTeller) + "]: selects teller");
    syncPrint("Customer " + to_string(id) + " [Teller " + to_string(assignedTeller) + "] introduces itself");

    // Coordinate arrival at the window
    tellerReadySem[assignedTeller]->signal();   
    customerReadySem[assignedTeller]->signal(); 

    // Wait for the teller to ask what we want
    askTransactionSem[assignedTeller]->wait();

    // Tell the teller the transaction type
    syncPrint("Customer " + to_string(id) + " [Teller " + to_string(assignedTeller) +
              "]: asks for " + (transactionType == DEPOSIT ? "deposit" : "withdrawal") + " transaction");
    // Store transaction type where teller can find it (indexed by teller ID)
    customerTransactions[assignedTeller] = transactionType;
    tellTransactionSem[assignedTeller]->signal(); 

    // Wait for the teller to finish the transaction
    transactionDoneSem[assignedTeller]->wait();

    // --- Leave the teller window ---
    syncPrint("Customer " + to_string(id) + " [Teller " + to_string(assignedTeller) + "]: leaves teller");
    customerLeaveSem[assignedTeller]->signal(); 

    // --- Leave the bank ---
    syncPrint("Customer " + to_string(id) + " []: goes to door");
    syncPrint("Customer " + to_string(id) + " []: leaves the bank");
}

// --- Main Program Entry Point ---
int main() {
    // IMPORTANT: Manually allocate semaphores because Semaphore class is not copyable
    for (int i = 0; i < NUM_TELLERS; i++) {
        tellerReadySem.push_back(new Semaphore(0));
        customerReadySem.push_back(new Semaphore(0));
        askTransactionSem.push_back(new Semaphore(0));
        tellTransactionSem.push_back(new Semaphore(0));
        transactionDoneSem.push_back(new Semaphore(0));
        customerLeaveSem.push_back(new Semaphore(0));
    }

    // Create and launch the teller threads
    vector<thread> tellerThreads;
    for (int i = 0; i < NUM_TELLERS; i++) {
        tellerThreads.push_back(thread(teller, i));
    }

    // Wait until all tellers have signaled they are ready
    for (int i = 0; i < NUM_TELLERS; i++) {
        bankOpenSem.wait();
    }
    syncPrint("Bank is open!"); 

    // Create and launch the customer threads
    vector<thread> customerThreads;
    for (int i = 0; i < NUM_CUSTOMERS; i++) {
        customerThreads.push_back(thread(customer, i));
        randomSleep(1, 10); 
    }

    // Wait for all customer threads to complete their lifecycle
    for (int i = 0; i < NUM_CUSTOMERS; i++) {
        if(customerThreads[i].joinable()) {
             customerThreads[i].join();
        }
    }
    syncPrint("All customers have finished."); 

    // --- Simulation End Sequence ---
    // Signal tellers that it's time to shut down
    queueMutex.wait();
    for (int i = 0; i < NUM_TELLERS; i++) {
        tellerAvailable[i] = true; 
        tellerCustomer[i] = -1;    
    }
    queueMutex.signal();

    // Wake up any tellers that might be stuck waiting for customers
    for (int i = 0; i < NUM_TELLERS; i++) {
        // Signal the semaphores tellers might be waiting on in their loop
        tellerReadySem[i]->signal();
        customerReadySem[i]->signal();
    }

    // Wait for all teller threads to finish their shutdown process
    for (int i = 0; i < NUM_TELLERS; i++) {
         if(tellerThreads[i].joinable()) { 
             tellerThreads[i].join();
         }
    }

    syncPrint("The bank closes for the day.");

    // --- IMPORTANT: Clean up dynamically allocated memory ---
    // Delete the semaphores created with 'new' to prevent memory leaks
    for (int i = 0; i < NUM_TELLERS; i++) {
        delete tellerReadySem[i];
        delete customerReadySem[i];
        delete askTransactionSem[i];
        delete tellTransactionSem[i];
        delete transactionDoneSem[i];
        delete customerLeaveSem[i];
    }
    // Clear the vectors of pointers (optional, but good practice)
    tellerReadySem.clear();
    customerReadySem.clear();
    askTransactionSem.clear();
    tellTransactionSem.clear();
    transactionDoneSem.clear();
    customerLeaveSem.clear();

    return 0;
}
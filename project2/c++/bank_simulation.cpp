#include <iostream>
#include <thread>
#include <vector>
#include <random>
#include <chrono>
#include <queue>
#include "semaphore.h"

// Constants
const int NUM_TELLERS = 3;
const int NUM_CUSTOMERS = 50;
const int MAX_SAFE_CAPACITY = 2;

// Enum for transaction types
enum TransactionType {
    DEPOSIT,
    WITHDRAWAL
};

// Shared resources and synchronization semaphores
Semaphore bankOpenSem(0);        // Bank opens when all tellers are ready
Semaphore safeSem(MAX_SAFE_CAPACITY); // Max 2 tellers in safe
Semaphore managerSem(1);        // Only 1 teller can interact with manager
Semaphore doorSem(2);           // Only 2 customers can enter at a time
Semaphore printSem(1);          // For synchronized output

// Semaphores for customer line management
Semaphore queueMutex(1);        // Mutex for queue access
Semaphore tellerAvailableSem(0); // Signals when a teller becomes available
std::queue<int> customerQueue;   // Queue of waiting customers

// Teller availability
std::vector<bool> tellerAvailable(NUM_TELLERS, false);
std::vector<int> tellerCustomer(NUM_TELLERS, -1);

// Semaphores for teller-customer interaction - use pointers to avoid copy constructor issues
std::vector<Semaphore*> tellerReadySem;
std::vector<Semaphore*> customerReadySem;
std::vector<Semaphore*> askTransactionSem;
std::vector<Semaphore*> tellTransactionSem;
std::vector<Semaphore*> transactionDoneSem;
std::vector<Semaphore*> customerLeaveSem;
std::vector<TransactionType> customerTransactions(NUM_TELLERS);

// For tracking when simulation should end
int customersServed = 0;
Semaphore customerCountSem(1);

// Random number generator
std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());

// Helper function for synchronized printing
void syncPrint(const std::string& msg) {
    printSem.wait();
    std::cout << msg << std::endl;
    printSem.signal();
}

// Sleep for random duration in milliseconds
void randomSleep(int min, int max) {
    std::uniform_int_distribution<int> dist(min, max);
    std::this_thread::sleep_for(std::chrono::milliseconds(dist(rng)));
}

// Teller thread function
void teller(int id) {
    // Indicate teller is ready
    syncPrint("Teller " + std::to_string(id) + " []: ready to serve");
    
    // Signal that bank is ready to open (when all 3 tellers are ready)
    bankOpenSem.signal();
    
    while (true) {
        // Indicate teller is waiting for a customer
        syncPrint("Teller " + std::to_string(id) + " []: waiting for a customer");
        
        // Make teller available for next customer
        queueMutex.wait();
        tellerAvailable[id] = true;
        
        // If there are customers waiting in the queue, serve the next one
        if (!customerQueue.empty()) {
            int customerId = customerQueue.front();
            customerQueue.pop();
            tellerCustomer[id] = customerId;
            tellerAvailable[id] = false;
            queueMutex.signal();
            
            // Signal to the customer that this teller is ready
            tellerReadySem[id]->signal();
            
            // Wait for customer to approach
            customerReadySem[id]->wait();
        } else {
            queueMutex.signal();
            
            // Signal that this teller is available
            tellerAvailableSem.signal();
            
            // Wait for a customer to approach
            tellerReadySem[id]->wait();
            
            // Wait for customer to be ready
            customerReadySem[id]->wait();
            
            queueMutex.wait();
            // If customer ID is -1, this is a signal to end
            if (tellerCustomer[id] == -1) {
                queueMutex.signal();
                break;
            }
            queueMutex.signal();
        }
        
        int custId = tellerCustomer[id];
        
        // Serve the customer
        syncPrint("Teller " + std::to_string(id) + " [Customer " + std::to_string(custId) + "]: serving a customer");
        
        // Ask for transaction
        syncPrint("Teller " + std::to_string(id) + " [Customer " + std::to_string(custId) + "]: asks for transaction");
        askTransactionSem[id]->signal();
        
        // Wait for customer to tell the transaction
        tellTransactionSem[id]->wait();
        
        // Get transaction type
        TransactionType transType = customerTransactions[id];
        
        if (transType == DEPOSIT) {
            syncPrint("Teller " + std::to_string(id) + " [Customer " + std::to_string(custId) + "]: handling deposit transaction");
            
            // Go to safe
            syncPrint("Teller " + std::to_string(id) + " [Customer " + std::to_string(custId) + "]: going to safe");
            safeSem.wait();
            syncPrint("Teller " + std::to_string(id) + " [Customer " + std::to_string(custId) + "]: enter safe");
            
            // Process transaction
            randomSleep(10, 50);
            
            // Leave safe
            syncPrint("Teller " + std::to_string(id) + " [Customer " + std::to_string(custId) + "]: leaving safe");
            safeSem.signal();
            
            // Complete transaction
            syncPrint("Teller " + std::to_string(id) + " [Customer " + std::to_string(custId) + "]: finishes deposit transaction.");
        } else {
            syncPrint("Teller " + std::to_string(id) + " [Customer " + std::to_string(custId) + "]: handling withdrawal transaction");
            
            // Get manager permission
            syncPrint("Teller " + std::to_string(id) + " [Customer " + std::to_string(custId) + "]: going to the manager");
            managerSem.wait();
            syncPrint("Teller " + std::to_string(id) + " [Customer " + std::to_string(custId) + "]: getting manager's permission");
            
            // Interaction with manager takes time
            randomSleep(5, 30);
            
            // Got permission
            syncPrint("Teller " + std::to_string(id) + " [Customer " + std::to_string(custId) + "]: got manager's permission");
            managerSem.signal();
            
            // Go to safe
            syncPrint("Teller " + std::to_string(id) + " [Customer " + std::to_string(custId) + "]: going to safe");
            safeSem.wait();
            syncPrint("Teller " + std::to_string(id) + " [Customer " + std::to_string(custId) + "]: enter safe");
            
            // Process transaction
            randomSleep(10, 50);
            
            // Leave safe
            syncPrint("Teller " + std::to_string(id) + " [Customer " + std::to_string(custId) + "]: leaving safe");
            safeSem.signal();
            
            // Complete transaction
            syncPrint("Teller " + std::to_string(id) + " [Customer " + std::to_string(custId) + "]: finishes withdrawal transaction.");
        }
        
        // Inform customer transaction is complete
        syncPrint("Teller " + std::to_string(id) + " [Customer " + std::to_string(custId) + "]: wait for customer to leave.");
        transactionDoneSem[id]->signal();
        
        // Wait for customer to leave
        customerLeaveSem[id]->wait();
        
        // Increment served customer count
        customerCountSem.wait();
        customersServed++;
        customerCountSem.signal();
    }
    
    syncPrint("Teller " + std::to_string(id) + " []: leaving for the day");
}

// Customer thread function
void customer(int id) {
    // Decide transaction type randomly (0 = deposit, 1 = withdrawal)
    std::uniform_int_distribution<int> transaction_dist(0, 1);
    TransactionType transactionType = static_cast<TransactionType>(transaction_dist(rng));
    
    if (transactionType == DEPOSIT) {
        syncPrint("Customer " + std::to_string(id) + " []: wants to perform a deposit transaction");
    } else {
        syncPrint("Customer " + std::to_string(id) + " []: wants to perform a withdrawal transaction");
    }
    
    // Customer waits a random amount of time before coming to the bank
    randomSleep(0, 100);
    
    // Going to bank
    syncPrint("Customer " + std::to_string(id) + " []: going to bank.");
    
    // Enter through door (limited to 2 at a time)
    doorSem.wait();
    syncPrint("Customer " + std::to_string(id) + " []: entering bank.");
    doorSem.signal();
    
    // Get in line
    syncPrint("Customer " + std::to_string(id) + " []: getting in line.");
    
    // Try to find an available teller or join the queue
    int assignedTeller = -1;
    
    queueMutex.wait();
    for (int i = 0; i < NUM_TELLERS; i++) {
        if (tellerAvailable[i]) {
            assignedTeller = i;
            tellerAvailable[i] = false;
            tellerCustomer[i] = id;
            break;
        }
    }
    
    if (assignedTeller == -1) {
        // No teller available, join the queue
        customerQueue.push(id);
        queueMutex.signal();
        
        // Wait for a teller to become available
        tellerAvailableSem.wait();
        
        // Find which teller signaled
        queueMutex.wait();
        for (int i = 0; i < NUM_TELLERS; i++) {
            if (tellerAvailable[i]) {
                assignedTeller = i;
                tellerAvailable[i] = false;
                tellerCustomer[i] = id;
                break;
            }
        }
        queueMutex.signal();
    } else {
        queueMutex.signal();
    }
    
    // Going to the teller
    syncPrint("Customer " + std::to_string(id) + " []: selecting a teller.");
    syncPrint("Customer " + std::to_string(id) + " [Teller " + std::to_string(assignedTeller) + "]: selects teller");
    syncPrint("Customer " + std::to_string(id) + " [Teller " + std::to_string(assignedTeller) + "] introduces itself");
    
    // Signal to teller that customer has arrived
    tellerReadySem[assignedTeller]->signal();
    customerReadySem[assignedTeller]->signal();
    
    // Wait for teller to ask for transaction
    askTransactionSem[assignedTeller]->wait();
    
    // Tell teller the transaction type
    syncPrint("Customer " + std::to_string(id) + " [Teller " + std::to_string(assignedTeller) + 
              "]: asks for " + (transactionType == DEPOSIT ? "deposit" : "withdrawal") + " transaction");
    
    customerTransactions[assignedTeller] = transactionType;
    tellTransactionSem[assignedTeller]->signal();
    
    // Wait for transaction to complete
    transactionDoneSem[assignedTeller]->wait();
    
    // Leave the teller
    syncPrint("Customer " + std::to_string(id) + " [Teller " + std::to_string(assignedTeller) + "]: leaves teller");
    customerLeaveSem[assignedTeller]->signal();
    
    // Leave the bank
    syncPrint("Customer " + std::to_string(id) + " []: goes to door");
    syncPrint("Customer " + std::to_string(id) + " []: leaves the bank");
}

int main() {
    // Initialize semaphore arrays with pointers to avoid copy constructor issues
    for (int i = 0; i < NUM_TELLERS; i++) {
        tellerReadySem.push_back(new Semaphore(0));
        customerReadySem.push_back(new Semaphore(0));
        askTransactionSem.push_back(new Semaphore(0));
        tellTransactionSem.push_back(new Semaphore(0));
        transactionDoneSem.push_back(new Semaphore(0));
        customerLeaveSem.push_back(new Semaphore(0));
    }
    
    // Create teller threads
    std::vector<std::thread> tellerThreads;
    for (int i = 0; i < NUM_TELLERS; i++) {
        tellerThreads.push_back(std::thread(teller, i));
    }
    
    // Wait for all tellers to be ready (bank to open)
    for (int i = 0; i < NUM_TELLERS; i++) {
        bankOpenSem.wait();
    }
    
    // Create customer threads
    std::vector<std::thread> customerThreads;
    for (int i = 0; i < NUM_CUSTOMERS; i++) {
        customerThreads.push_back(std::thread(customer, i));
    }
    
    // Wait for all customer threads to complete
    for (int i = 0; i < NUM_CUSTOMERS; i++) {
        customerThreads[i].join();
    }
    
    // Signal tellers to end simulation
    queueMutex.wait();
    for (int i = 0; i < NUM_TELLERS; i++) {
        tellerAvailable[i] = true;
        tellerCustomer[i] = -1;  // Special signal to end
    }
    queueMutex.signal();
    
    // Wake up tellers that might be waiting
    for (int i = 0; i < NUM_TELLERS; i++) {
        tellerReadySem[i]->signal();
        customerReadySem[i]->signal();
    }
    
    // Wait for all teller threads to complete
    for (int i = 0; i < NUM_TELLERS; i++) {
        tellerThreads[i].join();
    }
    
    syncPrint("The bank closes for the day.");
    
    // Clean up allocated semaphores
    for (int i = 0; i < NUM_TELLERS; i++) {
        delete tellerReadySem[i];
        delete customerReadySem[i];
        delete askTransactionSem[i];
        delete tellTransactionSem[i];
        delete transactionDoneSem[i];
        delete customerLeaveSem[i];
    }
    
    return 0;
}
import threading
import random
import time
import queue

# --- Configuration ---
NUM_CUSTOMERS = 50
NUM_TELLERS = 3
MAX_LINE_CAPACITY = 20 # Max customers allowed in the bank (waiting or being served)
# Small delays to allow threads to interleave realistically - *May need tuning*
SLEEP_MICRO = 0.005 # Even smaller sleep
SLEEP_SHORT = 0.02  # Slightly adjusted sleeps
SLEEP_MEDIUM = 0.06
SLEEP_LONG = 0.12

# --- Seed the random number generator for consistent runs ---
# Using an arbitrary seed. Change this number for different consistent runs.
# This won't necessarily match the sample's randomness unless we guess the seed.
random.seed(42) # Added seed

# --- Shared Resources & Synchronization ---

print_lock = threading.Lock()
waiting_line = threading.Semaphore(MAX_LINE_CAPACITY)
free_teller_queue = queue.Queue(NUM_TELLERS)
teller_waits_for_customer = [threading.Semaphore(0) for _ in range(NUM_TELLERS)]
teller_signals_customer_done = [threading.Semaphore(0) for _ in range(NUM_TELLERS)]
assigned_customer = [None] * NUM_TELLERS
assignment_lock = threading.Lock()
safe_lock = threading.Semaphore(1)
manager_lock = threading.Semaphore(1)
customers_served_count = 0
customers_served_lock = threading.Lock()
all_customers_processed = threading.Event()

# --- Barrier for initial customer announcement ---
# All NUM_CUSTOMERS must call wait() before any proceed past it
customer_announcement_barrier = threading.Barrier(NUM_CUSTOMERS) # Added Barrier

# --- Helper Function for Thread-Safe Printing ---
def safe_print(*args, **kwargs):
    with print_lock:
        print(*args, **kwargs)

# --- Teller Thread Function (Minor timing adjustments possible) ---
def teller_thread(teller_id):
    safe_print(f"Teller {teller_id} []: ready to serve")
    free_teller_queue.put(teller_id)

    while not all_customers_processed.is_set():
        safe_print(f"Teller {teller_id} []: waiting for a customer")
        teller_waits_for_customer[teller_id].acquire()

        if all_customers_processed.is_set():
            break

        with assignment_lock:
             customer_id = assigned_customer[teller_id]

        if customer_id is None:
             # This might happen if woken up by the final shutdown signal
             # Add self back just in case to avoid queue deadlock if shutdown logic flawed
             # free_teller_queue.put(teller_id) # Cautious add back
             continue # Check loop condition again

        safe_print(f"Teller {teller_id} [Customer {customer_id}]: serving a customer")
        time.sleep(SLEEP_MICRO) # Micro sleep
        safe_print(f"Teller {teller_id} [Customer {customer_id}]: asks for transaction")
        time.sleep(SLEEP_SHORT) # Short sleep

        transaction_type = customer_data[customer_id]['transaction_type']

        if transaction_type == 'deposit':
            # (Deposit logic - minor sleep adjustments possible but keep as is for now)
            safe_print(f"Teller {teller_id} [Customer {customer_id}]: handling deposit transaction")
            time.sleep(SLEEP_MEDIUM)
            safe_print(f"Teller {teller_id} [Customer {customer_id}]: going to safe")
            time.sleep(SLEEP_SHORT)
            safe_lock.acquire()
            safe_print(f"Teller {teller_id} [Customer {customer_id}]: enter safe")
            time.sleep(SLEEP_LONG)
            safe_print(f"Teller {teller_id} [Customer {customer_id}]: leaving safe")
            safe_lock.release()
            safe_print(f"Teller {teller_id} [Customer {customer_id}]: finishes deposit transaction.")
        else: # Withdrawal
            # (Withdrawal logic - minor sleep adjustments possible but keep as is for now)
            safe_print(f"Teller {teller_id} [Customer {customer_id}]: handling withdrawal transaction")
            time.sleep(SLEEP_MEDIUM)
            safe_print(f"Teller {teller_id} [Customer {customer_id}]: going to the manager")
            time.sleep(SLEEP_SHORT)
            manager_lock.acquire()
            safe_print(f"Teller {teller_id} [Customer {customer_id}]: getting manager's permission")
            time.sleep(SLEEP_LONG)
            safe_print(f"Teller {teller_id} [Customer {customer_id}]: got manager's permission")
            manager_lock.release()
            time.sleep(SLEEP_MICRO) # Micro sleep
            safe_print(f"Teller {teller_id} [Customer {customer_id}]: going to safe")
            time.sleep(SLEEP_SHORT)
            safe_lock.acquire()
            safe_print(f"Teller {teller_id} [Customer {customer_id}]: enter safe")
            time.sleep(SLEEP_LONG)
            safe_print(f"Teller {teller_id} [Customer {customer_id}]: leaving safe")
            safe_lock.release()
            safe_print(f"Teller {teller_id} [Customer {customer_id}]: finishes withdrawal transaction.")

        time.sleep(SLEEP_MICRO) # Micro sleep
        safe_print(f"Teller {teller_id} [Customer {customer_id}]: wait for customer to leave.")
        teller_signals_customer_done[teller_id].release()

        # Clear assignment ONLY AFTER signaling customer, and make teller available
        # Customer handles served count. Teller just becomes available.
        with assignment_lock:
            assigned_customer[teller_id] = None
        # Only add back to queue if not shutting down
        # Check flag *before* putting back might be safer, but queue is small
        # If the queue is full and we block here during shutdown, it's a problem.
        # Let's assume queue won't fill during normal operation. Put it back.
        if not all_customers_processed.is_set():
             try:
                 free_teller_queue.put(teller_id, block=False) # Don't block if full
             except queue.Full:
                 safe_print(f"Teller {teller_id} []: Error - Free teller queue full?") # Should not happen


    safe_print(f"Teller {teller_id} []: leaving for the day")


# --- Customer Thread Function (Barrier Added) ---
def customer_thread(customer_id, transaction_type):
    global customers_served_count

    customer_data[customer_id]['transaction_type'] = transaction_type
    safe_print(f"Customer {customer_id} []: wants to perform a {transaction_type} transaction")

    # --- Wait for all customers to announce ---
    try:
        # All customer threads wait here until NUM_CUSTOMERS have arrived.
        customer_announcement_barrier.wait()
    except threading.BrokenBarrierError:
        # This can happen if the barrier is reset or broken, unlikely here
        safe_print(f"Customer {customer_id} []: Barrier broken!")
        pass

    # --- Proceed AFTER barrier ---
    # Add a small random delay *after* the barrier to slightly stagger bank arrivals
    time.sleep(random.uniform(0.001, 0.05))

    safe_print(f"Customer {customer_id} []: going to bank.")
    waiting_line.acquire()
    safe_print(f"Customer {customer_id} []: entering bank.")
    time.sleep(SLEEP_MICRO)
    safe_print(f"Customer {customer_id} []: getting in line.")
    time.sleep(SLEEP_SHORT)

    safe_print(f"Customer {customer_id} []: selecting a teller.")
    teller_id = free_teller_queue.get() # Wait for free teller

    with assignment_lock:
        assigned_customer[teller_id] = customer_id

    safe_print(f"Customer {customer_id} [Teller {teller_id}]: selects teller")
    time.sleep(SLEEP_MICRO)
    safe_print(f"Customer {customer_id} [Teller {teller_id}] introduces itself")
    time.sleep(SLEEP_SHORT) # Give teller time to print "serving"

    teller_waits_for_customer[teller_id].release() # Signal the specific teller

    time.sleep(SLEEP_MEDIUM) # Wait for teller to ask for transaction
    safe_print(f"Customer {customer_id} [Teller {teller_id}]: asks for {transaction_type} transaction")

    teller_signals_customer_done[teller_id].acquire() # Wait for teller to finish

    safe_print(f"Customer {customer_id} [Teller {teller_id}]: leaves teller")
    time.sleep(SLEEP_MICRO)
    safe_print(f"Customer {customer_id} []: goes to door")
    time.sleep(SLEEP_SHORT)
    safe_print(f"Customer {customer_id} []: leaves the bank")

    waiting_line.release() # Release spot in bank

    with customers_served_lock:
        customers_served_count += 1
        # safe_print(f"DEBUG: Customers served: {customers_served_count}/{NUM_CUSTOMERS}")
        if customers_served_count == NUM_CUSTOMERS:
            all_customers_processed.set()
            # Wake up any potentially waiting tellers so they can check the flag and exit
            for i in range(NUM_TELLERS):
                 # Check if teller might be waiting by seeing if semaphore value is 0
                 # This is slightly hacky - semaphore value check isn't guaranteed atomic
                 # A cleaner way might involve another signal, but releasing is usually ok.
                 # try:
                 #    if teller_waits_for_customer[i].get_value() == 0:
                 #        teller_waits_for_customer[i].release()
                 # except AttributeError: # get_value might not be available everywhere
                 # Just release all - extra releases on a Semaphore > 0 are ok.
                 teller_waits_for_customer[i].release() # Wake up potentially idle tellers


# --- Main Execution ---
if __name__ == "__main__":
    customer_threads = []
    teller_threads = []
    customer_data = {}

    # --- Create Threads ---
    for i in range(NUM_TELLERS):
        thread = threading.Thread(target=teller_thread, args=(i,))
        teller_threads.append(thread)

    for i in range(NUM_CUSTOMERS):
        transaction = random.choice(['deposit', 'withdrawal'])
        customer_data[i] = {'id': i, 'transaction_type': transaction}
        thread = threading.Thread(target=customer_thread, args=(i, transaction))
        customer_threads.append(thread)

    # --- Start Threads ---
    for thread in teller_threads:
        thread.start()

    time.sleep(0.2) # Increased sleep slightly to ensure tellers are ready

    for thread in customer_threads:
        thread.start()
        # Removed the stagger here - let the barrier and post-barrier sleep handle timing
        # time.sleep(random.uniform(0.005, 0.02))

    # --- Wait for Completion ---
    for thread in customer_threads:
        thread.join()

    # safe_print("DEBUG: All customers joined. Waiting for tellers.") # Keep debug off for final

    # Final check to wake up tellers (redundant if logic inside customer thread works)
    if not all_customers_processed.is_set():
         all_customers_processed.set()
         for sem in teller_waits_for_customer:
            try:
                sem.release()
            except Exception: pass # Ignore errors on potentially over-released sem

    for thread in teller_threads:
        thread.join()

    safe_print("The bank closes for the day.")
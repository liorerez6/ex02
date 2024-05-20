#include <iostream>
#include <pthread.h>

using namespace std;

// Define Block class
class Block {
public:
    int ID = 0;
    // Add other block attributes as needed
    // For simplicity, let's assume only ID is needed for now
    Block(int id) : ID(id) {}
};

void* serverLoop(void* arg);
void* minerLoop(void* arg);


// Server thread function
void* serverLoop(void* arg) {
    // Initial block
    Block initBlock(0);

    // Create miner threads
    pthread_t miner1, miner2;
    pthread_create(&miner1, NULL, minerLoop, (void*) &initBlock);
    pthread_create(&miner2, NULL, minerLoop, (void*) &initBlock);

    // Wait for miner threads to finish
    pthread_join(miner1, NULL);
    pthread_join(miner2, NULL);

    return nullptr;
}

// Miner thread function
void* minerLoop(void* arg) {
    // Cast argument to Block pointer
    Block* blockPtr = (Block*)arg;
    // Access block attributes
    int blockID = blockPtr->ID;

   return nullptr;
}

int main() {
    // Create server thread
    pthread_t server;
    pthread_create(&server, NULL, serverLoop, NULL);

    // Wait for server thread to finish
    pthread_join(server, NULL);
    cout << "hello world!" << endl;




    return 0;
}

#include <iostream>
#include <pthread.h>
#include <zlib.h>
#include <list>
#include <unistd.h> // For sleep function
#include <chrono>

using namespace std;

int g_Difficulty = 1;



class BlockForHash{

    public:

    int height;
    int timestamp;
    unsigned int prev_hash;
    int difficulty;
    int nonce;
    int relayed_by;

    // Constructor
    BlockForHash(int i_height, int i_timestamp, unsigned int i_prev_hash, int i_difficulty, int i_nonce, int i_relayed_by)
        : height(i_height), timestamp(i_timestamp), prev_hash(i_prev_hash),
          difficulty(i_difficulty), nonce(i_nonce), relayed_by(i_relayed_by) {}


    void updateTimestamp() {
        // Get the current time as an integer (seconds since epoch)
        timestamp = static_cast<int>(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()));
    }

    BlockForHash() : height(0), timestamp(0), prev_hash(0), difficulty(0), nonce(0), relayed_by(0) {}

};



class Block
{
    public:

    BlockForHash m_Block;
    unsigned int hash;
    Block(BlockForHash i_Block, int i_hash) : m_Block(i_Block), hash(i_hash) {};
    Block() : m_Block(), hash(0) {}


};

// Define a list to hold blockchain
list<Block> blockchain;

Block testingBlock; // testing the block if its good --> push to list. 

// Define mutex for synchronizing access to blockchain
pthread_mutex_t blockchain_mutex;

pthread_cond_t newBlockCreated;

// Function to calculate CRC32
uLong calculateCRC32(BlockForHash block) {
    uLong crc = crc32(0L, Z_NULL, 0);
    crc = crc32(crc, (const Bytef*)&block, sizeof(block));
    return crc;
}


bool maskCheckForDifficulty(int i_Difficulty, int i_Crc) {   // true = proper difficulty. false = not good difficulty
    // Initialize mask with 1s in the most significant bit and zeros in the rest
     uLong mask = 0xFFFFFFFF;
    mask <<= (32 - i_Difficulty); // 100000000000
    return (!(mask & i_Crc));
    // crc - 01239495043 
    
}

bool proofOfWork(const Block& i_Block)
{
    Block curr = blockchain.back();
    if(curr.m_Block.height >= i_Block.m_Block.height){
        return false;
    }
    int hashOfCRC32 = calculateCRC32(i_Block.m_Block);
    return (maskCheckForDifficulty(i_Block.m_Block.difficulty, hashOfCRC32));

}


// Server thread function
void* serverLoop(void* arg) {
    // Generate genesis block

    pthread_mutex_lock(&blockchain_mutex);

    BlockForHash genesisBlock1(0, time(NULL), 0, 3, 1, 0);
    genesisBlock1.updateTimestamp();
    Block genesisBlock(genesisBlock1, 0);

    // Append genesis block to blockchain
    blockchain.push_back(genesisBlock);

    //pthread_mutex_unlock(&blockchain_mutex);

    while (true) {

        pthread_cond_wait(&newBlockCreated, &blockchain_mutex); // waiting for new Block
        if(proofOfWork(testingBlock))
        {
            cout << "\n\nminer ID: " << testingBlock.m_Block.relayed_by << "\nheight: " << testingBlock.m_Block.height << "\nHash: " << testingBlock.hash << "\n\n\n";
            blockchain.push_back(testingBlock);
        }
        if(blockchain.back().m_Block.height % 50 == 0)
        {
            
        }
        
    }

    return nullptr;
}




// Miner thread function
void* minerLoop(void* arg) {
    // Miner ID
    int minerID = *((int*)arg);




    while (true) {

        pthread_mutex_lock(&blockchain_mutex);
        Block currentBlock = blockchain.back();
        pthread_mutex_unlock(&blockchain_mutex);
        currentBlock.m_Block.height++;
        currentBlock.m_Block.prev_hash = currentBlock.hash;
        currentBlock.m_Block.relayed_by = minerID;

        




        while(true)
        {


            currentBlock.m_Block.updateTimestamp();
            currentBlock.m_Block.nonce++;
            currentBlock.hash = calculateCRC32(currentBlock.m_Block);
            if(maskCheckForDifficulty(currentBlock.m_Block.difficulty, currentBlock.hash) == true)
            {
                break;
            }
        
        }

        pthread_mutex_lock(&blockchain_mutex);
        testingBlock = currentBlock;
        pthread_cond_signal(&newBlockCreated);
        pthread_mutex_unlock(&blockchain_mutex);

    }

    return nullptr;
}

int main() {
    // Create server thread
    pthread_t server;
    pthread_create(&server, NULL, serverLoop, NULL);

    pthread_cond_init(&newBlockCreated, NULL);
    pthread_mutex_init(&blockchain_mutex, NULL);

    // Create miner threads
    const int NUM_MINERS = 4;
    pthread_t miners[NUM_MINERS];
    for (int i = 1; i <= NUM_MINERS; ++i) {
        int* minerID = new int(i); // Allocate miner ID dynamically
        pthread_create(&miners[i], NULL, minerLoop, minerID);
    }

    // Join server thread
    pthread_join(server, NULL);

    // Join miner threads
    for (int i = 0; i < NUM_MINERS; ++i) {
        pthread_join(miners[i], NULL);
    }

    return 0;
}

#include <iostream>
#include <pthread.h>
#include <zlib.h>
#include <list>
#include <unistd.h> // For sleep function
#include <chrono>

using namespace std;

int g_Difficulty = 21; //TODO change for input



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

list <Block> testingBlock; // testing the block if its good --> push to blockchain list. 

// Define mutex for synchronizing access to blockchain
pthread_mutex_t blockchain_mutex;

pthread_cond_t newBlockCreated;

pthread_cond_t waitingForServer;

// Function to calculate CRC32
uLong calculateCRC32(BlockForHash block) {
    uLong crc = crc32(0L, Z_NULL, 0);
    crc = crc32(crc, (const Bytef*)&block, sizeof(block));
    return crc;
}



bool maskCheckForDifficulty(int i_Difficulty, int i_hash) {   // true = proper difficulty. false = not good difficulty
    // Initialize mask with 1s in the most significant bit and zeros in the rest
     uLong mask = 0xFFFFFFFF;
    mask <<= (32 - i_Difficulty); // 100000000000
    return (!(mask & i_hash));
    
    
}

bool proofOfWork(const Block& i_Block) 
{
    Block curr = blockchain.back();
    if(curr.m_Block.height >= i_Block.m_Block.height){
        cout << "miner " << curr.m_Block.relayed_by << " gets a head of miner: " << i_Block.m_Block.relayed_by << endl;

        return false;
        
    }
    int hashOfCRC32 = calculateCRC32(i_Block.m_Block);
    if(hashOfCRC32 != i_Block.hash)
    {
    cout << "Wrong hash for block #" << i_Block.m_Block.height << " by miner " << i_Block.m_Block.relayed_by << ", recived" << i_Block.hash << " but calculated " << hashOfCRC32 << endl;
    }
    return (maskCheckForDifficulty(i_Block.m_Block.difficulty, hashOfCRC32));

}

void setNewBlock(Block& i_newBlockForUpdate, int i_MinerId)
{

        i_newBlockForUpdate.m_Block.height++;
        i_newBlockForUpdate.m_Block.prev_hash = i_newBlockForUpdate.hash;
        i_newBlockForUpdate.m_Block.relayed_by = i_MinerId;
        i_newBlockForUpdate.m_Block.difficulty = g_Difficulty;
}


void* testerFunc(void* arg)
{
    int minerID = *((int*)arg);

    while(true)
    {
        pthread_mutex_lock(&blockchain_mutex);
        Block currentBlock = blockchain.back();
        setNewBlock(currentBlock, minerID);
        pthread_mutex_unlock(&blockchain_mutex);



        currentBlock.m_Block.updateTimestamp();
        currentBlock.m_Block.nonce++;
        currentBlock.hash = 0xFFFFFFFF; //wrong hash

        pthread_mutex_lock(&blockchain_mutex);
        cout << "Tester Miner #" << currentBlock.m_Block.relayed_by << ": Mined a new block #" << currentBlock.m_Block.height << ", with the hash "<< currentBlock.hash << endl;
        testingBlock.push_back(currentBlock);
        pthread_cond_signal(&newBlockCreated);
        pthread_cond_wait(&waitingForServer, &blockchain_mutex);
        
        pthread_mutex_unlock(&blockchain_mutex);

        sleep(1);



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
        setNewBlock(currentBlock, minerID);
        pthread_mutex_unlock(&blockchain_mutex);
        
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
        cout << "Miner #" << currentBlock.m_Block.relayed_by << ": Mined a new block #" << currentBlock.m_Block.height << ", with the hash ";
        cout << "0x" << hex << currentBlock.hash << endl;
        testingBlock.push_back(currentBlock);
        pthread_cond_signal(&newBlockCreated);
        pthread_cond_wait(&waitingForServer, &blockchain_mutex);
        
        pthread_mutex_unlock(&blockchain_mutex);
        
    }

    return nullptr;
}


// Server thread function
void* serverLoop(void* arg) {
    // Generate genesis block

    pthread_mutex_lock(&blockchain_mutex);

    BlockForHash genesisBlock1(0, time(NULL), 0, g_Difficulty, 1, 0);
    genesisBlock1.updateTimestamp();
    Block genesisBlock(genesisBlock1, 0);

    // Append genesis block to blockchain
    blockchain.push_back(genesisBlock);

    //pthread_mutex_unlock(&blockchain_mutex);

    const int NUM_MINERS = 4;
    pthread_t miners[NUM_MINERS];
    for (int i = 1; i <= NUM_MINERS; ++i) {
        int* minerID = new int(i); // Allocate miner ID dynamically
        pthread_create(&miners[i], NULL, minerLoop, minerID);
    }

    pthread_t testingMiner;
    int* testingMinerID = new int(5);
    pthread_create(&testingMiner, NULL,testerFunc ,testingMinerID);




    while (true) {

        pthread_cond_wait(&newBlockCreated, &blockchain_mutex); // waiting for new Block
        
        for( Block i : testingBlock){
            if(proofOfWork(i))
                {
                    
                    blockchain.push_back(i);
                    cout << "Serever: New block added by " << i.m_Block.relayed_by << " with height: " << i.m_Block.height << endl;
                    
                }
        }
        testingBlock.clear();
        pthread_cond_broadcast(&waitingForServer);
        
    }

    return nullptr;
}







int main() {
    // Create server thread
    pthread_t server;
    pthread_cond_init(&newBlockCreated, NULL);
    pthread_mutex_init(&blockchain_mutex, NULL);
    pthread_cond_init(&waitingForServer, NULL);

    pthread_create(&server, NULL, serverLoop, NULL);


    // Join server thread
    pthread_join(server, NULL);

    return 0;
}

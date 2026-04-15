#include <iostream>
#include <vector>
#include <unordered_map>


class MemoryPool {
public:
    MemoryPool(size_t blockSize, size_t numBlocks);
    void* allocate();
    void deallocate(void* ptr);
    ~MemoryPool();

  private:
};

MemoryPool::MemoryPool(size_t blockSize, size_t numBlocks){



}


int main(int argc, char* argv[]){

  std::cout <<"hello world"<< std::endl;
  return 0;
}

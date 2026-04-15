#include <iostream>





template<typename T>
class Deque{
  private:
    struct Slot{
      T value_;
      int index_;

    }; 
  
    Slot* internal_; size_t size_;
    int head_; int tail_;
    size_t capacity_;
    

  public:
    Deque(size_t capacity) : head_(0), tail_(0), size_(0), capacity_(capacity) {
      internal_ = new Slot[capacity_];

    }
    
    void push(T value){
            

    }

    T pop(){

    } 
    
    T front() const{


    }



};

int main (int argc, char *argv[]) {
  std::cout << "deque" << std::endl; 
  return 0;
}

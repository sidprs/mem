/*
implement a dynamic linked list without using std libraries
*/

#include <iostream>


template<typename T>
class DynamicLL{
  private:
    size_t capacity_;
    int64_t size_;
    struct Node{
      Node* next = nullptr;
      Node* prev = nullptr;
      T value;
      Node(T x): value(x), prev(nullptr), next(nullptr) {}

    }; 
    Node* head_;
    Node* tail_;

  public:
    // head ->          <- tail   
    explicit DynamicLL(size_t capacity) : capacity_(capacity),
        head_(nullptr), tail_(nullptr), size_(0) {}
   

    Node* find(T value) const{
      int idx = 0;
      Node* curr = head_;
      while(curr != nullptr){
        if(curr->value == value) return curr;
        idx++;
        curr = curr->next;
      } 
      return throw std::runtime_error("no node"); 
    }    
    
    void insert(T value, int pos){
      Node* found = find(pos);
      if(found == -1) return;
       

    }

    void push_back(T value){
      Node* newNode = new Node(value);
      
      newNode->next = nullptr;
      newNode->prev = tail_;
      if(tail_){
        tail_->next = newNode;
      }
      else{
        head_ = newNode;
      }
      tail_ = newNode;
      ++size_;                   
    }

    void push_front(T value){
      Node* newNode = new Node(value);
      newNode->next = head_;
      newNode->prev = nullptr;
      if(head_){
        head_->prev = newNode;
      }
      else{
         tail_ = newNode;
      }
      head_ = newNode;
      ++size_;                         
    }

    bool pop_front() {
      if (!head_) return false;          // empty

      Node* victim = head_;
      head_ = head_->next;               // move head forward

      if (head_) {
        head_->prev = nullptr;           // new head has no prev
      } else {
        tail_ = nullptr;                 // list is now empty
      }

      delete victim;
      --size_;
      return true;
    }

    bool pop_back(){
      if(!tail) return false;
      Node* victim = tail_;
      tail_ = tail_->prev;
      
      if (tail_) {
        tail_->next = nullptr;           // new head has no prev
      } else {
        head_ = nullptr;                 // list is now empty
      }

      delete victim;
      --size_;
      return true;

    }

};


int main(){


  return 0;
}

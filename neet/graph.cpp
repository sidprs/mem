#include <vector>
#include <iostream>
#include <unordered_set>
#include <unordered_map>
#include <deque>
#include <stack>
#include <algorithm>



template <typename NodeT, typename WeightT = int>
class Graph{
  private:
    std::unordered_map<NodeT, std::unordered_map<NodeT, WeightT>> adjList_;
    bool is_directed; 
  public:
    
    explicit Graph(bool is_directed) : is_directed(is_directed) {}
    void addNode(const NodeT& src) {
      if(adjList_.find(src) == adjList_.end()){
        adjList_[src];
      }
    }
    
    void addEdge(const NodeT& src, const NodeT& dst, const WeightT& weight){

    }
};


template<typename T>
class GraphN{
private:
  std::vector<std::vector<T>> adjList_;
  std::unordered_map<T, std::vector<T>> adjMap_;
  size_t capacity_;
  bool is_directed;
public:
  explicit GraphN(size_t capacity, bool directed = false): capacity_(capacity),
    is_directed(directed)
  
  {}

  void addNode(const T& src, const T& dst){
    adjList_[src].push_back(dst);
    if(adjMap_.find(src) == adjMap_.end()){
      adjMap_[src] = {};
    }
    adjMap_[src].push_back(dst);
    

    if(!is_directed){
      adjList_[dst].push_back(src);
      adjMap_[dst].push_back(src);
      
    }
  }
  void printAdjList() const {
    std::cout << "Adjacency List:\n";
    for (size_t i = 0; i < adjList_.size(); ++i) {
      std::cout << i << ": ";
      for (const auto& neighbor : adjList_[i]) {
        std::cout << neighbor << " ";
      }
      std::cout << "\n";
    }
  }
  
  void printAdjMap() const {
    std::cout << "Adjacency Map:\n";
    for (const auto& [node, neighbors] : adjMap_) {
      std::cout << node << ": ";
      for (const auto& neighbor : neighbors) {
        std::cout << neighbor << " ";
      }
      std::cout << "\n";
    }
  }
 

};


int main (int argc, char *argv[]) {
  
  return 0;
}

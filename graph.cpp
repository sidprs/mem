#include <iostream>
#include <vector>



template<typename T>
class Graph{
private:
  bool isDirected_;
  int capacity_;
  struct Vertex{
      T source;
      T destination;
    };
  
  std::vector<std::vector<Vertex>> adjList_;
  void addEdge(T source, T destination){
    
    adjList_[source].push_back(destination);
    if(!isDirected_) adjList_[destination].push_back(source);
  }
  void dfs_rec(std::vector<Vertex>& visited, T node){
    for(const auto& curr : adjList_[node]){
        visited.push_back(curr);
        dfs_rec(visited, adjList_[curr]);
    }
  }


public:
  explicit Graph(size_t capacity, bool isDirected) : capacity_(static_cast<int>(capacity)),
        isDirected_(isDirected) {}
  
  std::vector<Vertex> dfs(T source){
    std::vector<Vertex> visited;
    visited.push_back(source);
    dfs(visited, source);
    return visited;
  }


};

int main(int argc, char* argv[]){

  std::cout <<"hello" << std::endl;
  return 0;
}

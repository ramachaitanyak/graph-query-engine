#include "src/include/graph.h"
#include <queue>

namespace GraphQueryEngine {

uint32_t
Graph::MinEdgeBfs(int src, int dest) {
    // visited[num_nodes] for keeping track of visited
    // node in BFS
    //bool visited[num_nodes] = {0};
    std::vector<bool> visited;
    visited.resize(num_nodes, false);

    // Initialize distances as 0
    std::vector<uint32_t> distance;
    distance.resize(num_nodes, 0);
    //int distance[num_nodes] = {0};

    // queue to do BFS.
    std::queue<uint32_t> Q;
    distance[src] = 0;

    Q.push(src);
    visited[src] = true;
    while (!Q.empty())
    {
        int x = Q.front();
        Q.pop();

        for (int i=0; i<adjacency_list[x].size(); i++)
        {
            if (visited[adjacency_list[x][i]])
                continue;

            // update distance for i
            distance[adjacency_list[x][i]] = distance[x] + 1;
            Q.push(adjacency_list[x][i]);
            visited[adjacency_list[x][i]] = 1;
        }
    }

  return distance[dest];

}

std::string
GraphEngine::ProcessRequest(graph::Request& request) {
  return "OK";
}

} // end GraphQueryEngine namespace

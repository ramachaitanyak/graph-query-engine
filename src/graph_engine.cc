#include "src/include/graph.h"
#include <limits>
#include <queue>

namespace GraphQueryEngine {

uint32_t Graph::MinEdgeBfs(int src, int dest) {
  // visited[num_nodes] for keeping track of visited
  // node in BFS
  // bool visited[num_nodes] = {0};
  std::vector<bool> visited;
  visited.resize(num_nodes, false);

  // Initialize distances as 0
  std::vector<uint32_t> distance;
  distance.resize(num_nodes, std::numeric_limits<uint32_t>::max());

  // queue to do BFS.
  std::queue<uint32_t> Q;
  distance[src] = 0;

  Q.push(src);
  visited[src] = true;
  while (!Q.empty()) {
    int x = Q.front();
    Q.pop();

    for (int i = 0; i < adjacency_list[x].size(); i++) {
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

std::string GraphEngine::PostGraphRequest(graph::Request &request) {

  // Parse Adjacency List
  std::vector<GraphQueryEngine::Graph::Edge> edges;
  for (int i = 0; i < request.adjacency_list_size(); i++) {
    const graph::Edges &edge_pb = request.adjacency_list(i);
    GraphQueryEngine::Graph::Edge edge(edge_pb.src(), edge_pb.dest());
    edges.push_back(edge);
  }

  // Get total number of nodes
  uint32_t num_nodes = request.graph_total_nodes();

  // Build Adjacency List
  std::vector<std::vector<uint32_t>> adj_list;
  adj_list.resize(num_nodes);
  for (auto edge : edges) {
    adj_list[edge.src].push_back(edge.dest);
  }

  // Compute hash value based on the graph_name to handle collisions
  std::string graph_name = request.graph_name();

  // Build Graph
  GraphSharedPtr graph_shared_ptr =
      std::make_shared<Graph>(num_nodes, adj_list, graph_name);

  // Compute the hash value from graph name to generate graph id
  uint64_t hash_val = hash_fn(graph_name);

  // Lock the graph db to check for duplicate graphs with hash_val
  {
    std::lock_guard<std::mutex> guard(graph_db_mutex);
    auto it = graph_db.find(hash_val);
    if (it == graph_db.end()) {
      // If there are no duplicates add to graph DB
      graph_db.insert(std::make_pair(hash_val, graph_shared_ptr));
    } else {
      return "ERROR: Graph already in DB";
    }
  }

  return std::to_string(hash_val);
}

std::string GraphEngine::DeleteGraphRequest(graph::Request &request) {
  // Parse graph id from the request
  uint64_t hash_id = request.delete_graph().map_id();
  // Lock the graph db to check for the graph with hash_id
  {
    std::lock_guard<std::mutex> guard(graph_db_mutex);
    auto it = graph_db.find(hash_id);
    if (it != graph_db.end()) {
      graph_db.erase(it);
    } else {
      return "ERROR: Graph not present in DB";
    }
  }
  return "OK, deleted graph with ID: " + std::to_string(hash_id);
}

std::string GraphEngine::MinDistanceGraphRequest(graph::Request &request) {
  // Parse graph id, source and destination node from request
  uint64_t graph_id = request.min_distance().map_id();
  uint32_t source_node = request.min_distance().begin_node();
  uint32_t end_node = request.min_distance().end_node();
  // Lock the graph db to check for the graph
  {
    std::lock_guard<std::mutex> guard(graph_db_mutex);
    auto it = graph_db.find(graph_id);
    if (it != graph_db.end()) {
      uint32_t min_dist = it->second->MinEdgeBfs(source_node, end_node);
      return "OK, found minimum distance between " +
             std::to_string(source_node) + " " + std::to_string(end_node) +
             " to be " + std::to_string(min_dist);
    } else {
      return "ERROR: Graph not present in DB";
    }
  }
}

std::string GraphEngine::ProcessRequest(graph::Request &request) {

  switch (request.request_type()) {
  case graph::POST_GRAPH:
    return PostGraphRequest(request);
  case graph::DELETE_GRAPH:
    return DeleteGraphRequest(request);
  case graph::GET_MIN_DISTANCE:
    return MinDistanceGraphRequest(request);
  default:
    return "ERROR";
  }
}

} // namespace GraphQueryEngine

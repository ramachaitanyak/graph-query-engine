/*
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

#include <grpc/support/log.h>
#include <grpcpp/grpcpp.h>
#include <thread>

#include "src/include/graph.h"

#ifdef BAZEL_BUILD
#include "protos/graph.grpc.pb.h"
#else
#include "graph.grpc.pb.h"
#endif

using graph::GraphEngine;
using graph::Request;
using graph::Response;
using grpc::Channel;
using grpc::ClientAsyncResponseReader;
using grpc::ClientContext;
using grpc::CompletionQueue;
using grpc::Status;

class GraphEngineClient {
public:
  explicit GraphEngineClient(std::shared_ptr<Channel> channel)
      : stub_(GraphEngine::NewStub(channel)) {}

  // Assembles the client's payload and sends it to the server.
  void PostGraphRequest(const std::string &graph_name,
                        std::vector<GraphQueryEngine::Graph::Edge> &adj_list,
                        const uint32_t &num_nodes) {

    // Data we are sending to the server.
    Request request;
    request.set_graph_name(graph_name);
    request.set_graph_total_nodes(num_nodes);
    request.set_request_type(graph::POST_GRAPH);

    // Construct the adjacency list in protobuf format
    for (auto input_edge : adj_list) {
      graph::Edges *edge = request.add_adjacency_list();
      edge->set_src(input_edge.src);
      edge->set_dest(input_edge.dest);
    }

    // Call object to store rpc data
    AsyncClientCall *call = new AsyncClientCall;

    // stub_->PrepareAsyncSayHello() creates an RPC object, returning
    // an instance to store in "call" but does not actually start the RPC
    // Because we are using the asynchronous API, we need to hold on to
    // the "call" instance in order to get updates on the ongoing RPC.
    call->response_reader =
        stub_->PrepareAsyncGraphEngineRequest(&call->context, request, &cq_);

    // StartCall initiates the RPC call
    call->response_reader->StartCall();

    // Request that, upon completion of the RPC, "reply" be updated with the
    // server's response; "status" with the indication of whether the operation
    // was successful. Tag the request with the memory address of the call
    // object.
    call->response_reader->Finish(&call->reply, &call->status, (void *)call);
  }

  // Assembles the client's payload for deleting a stored graph and sends it to
  // server
  void DeleteGraphRequest(const uint64_t &graph_id) {
    Request request;
    request.set_request_type(graph::DELETE_GRAPH);
    request.mutable_delete_graph()->set_map_id(graph_id);

    // Call object to store rpc data
    AsyncClientCall *call = new AsyncClientCall;

    // stub_->PrepareAsyncSayHello() creates an RPC object, returning
    // an instance to store in "call" but does not actually start the RPC
    // Because we are using the asynchronous API, we need to hold on to
    // the "call" instance in order to get updates on the ongoing RPC.
    call->response_reader =
        stub_->PrepareAsyncGraphEngineRequest(&call->context, request, &cq_);

    // StartCall initiates the RPC call
    call->response_reader->StartCall();

    // Request that, upon completion of the RPC, "reply" be updated with the
    // server's response; "status" with the indication of whether the operation
    // was successful. Tag the request with the memory address of the call
    // object.
    call->response_reader->Finish(&call->reply, &call->status, (void *)call);
  }

  // Assembles the client's payload for calculating the minimum distance between
  // two nodes in a stored graph, identified by graph_id
  void CalculateMinDistanceRequest(const uint64_t &graph_id, const uint32_t src,
                                   const uint32_t dest) {
    Request request;
    request.set_request_type(graph::GET_MIN_DISTANCE);
    request.mutable_min_distance()->set_begin_node(src);
    request.mutable_min_distance()->set_end_node(dest);
    request.mutable_min_distance()->set_map_id(graph_id);

    // Call object to store rpc data
    AsyncClientCall *call = new AsyncClientCall;

    // stub_->PrepareAsyncSayHello() creates an RPC object, returning
    // an instance to store in "call" but does not actually start the RPC
    // Because we are using the asynchronous API, we need to hold on to
    // the "call" instance in order to get updates on the ongoing RPC.
    call->response_reader =
        stub_->PrepareAsyncGraphEngineRequest(&call->context, request, &cq_);

    // StartCall initiates the RPC call
    call->response_reader->StartCall();

    // Request that, upon completion of the RPC, "reply" be updated with the
    // server's response; "status" with the indication of whether the operation
    // was successful. Tag the request with the memory address of the call
    // object.
    call->response_reader->Finish(&call->reply, &call->status, (void *)call);
  }

  // Loop while listening for completed responses.
  // Prints out the response from the server.
  void AsyncCompleteRpc() {
    void *got_tag;
    bool ok = false;

    // Block until the next result is available in the completion queue "cq".
    while (cq_.Next(&got_tag, &ok)) {
      // The tag in this example is the memory location of the call object
      AsyncClientCall *call = static_cast<AsyncClientCall *>(got_tag);

      // Verify that the request was completed successfully. Note that "ok"
      // corresponds solely to the request for updates introduced by Finish().
      GPR_ASSERT(ok);

      std::string response;
      response.clear();
      if (call->status.ok()) {
        response = call->reply.message();
        std::cout << "Client received: " << response << std::endl;
      } else {
        std::cout << "RPC failed" << std::endl;
      }

      // Capture only the graph_id stored, exclude ERROR and OK
      size_t found_error = response.find("ERROR");
      if (found_error == std::string::npos) {
        size_t found_ok = response.find("OK");
        if (found_ok == std::string::npos) {
          graph_ids.push_back(std::stoull(response));
        }
      }

      // Once we're complete, deallocate the call object.
      delete call;
    }
  }

  // The response vector with posted graph-ids
  std::vector<uint64_t> graph_ids;

private:
  // struct for keeping state and data information
  struct AsyncClientCall {
    // Container for the data we expect from the server.
    Response reply;

    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    ClientContext context;

    // Storage for the status of the RPC upon completion.
    Status status;

    std::unique_ptr<ClientAsyncResponseReader<Response>> response_reader;
  };

  // Out of the passed in Channel comes the stub, stored here, our view of the
  // server's exposed services.
  std::unique_ptr<GraphEngine::Stub> stub_;

  // The producer-consumer queue we use to communicate asynchronously with the
  // gRPC runtime.
  CompletionQueue cq_;
};

int ProcessCliPost(GraphEngineClient &client, std::string &graph_name,
                   std::string &file_path) {
  // Check file-path is valid
  struct stat buffer;
  if (stat(file_path.c_str(), &buffer) == 0) {
    // Valid file
  } else {
    std::cout << "Invalid command with file-path, please check" << std::endl;
    return 0;
  }
  // Open file and read contents
  std::fstream newfile;
  int i = 1;
  uint32_t nodes = 0;
  std::vector<GraphQueryEngine::Graph::Edge> adj_list;
  newfile.open(file_path.c_str(), std::ios::in);
  if (newfile.is_open()) {
    std::string tp;
    while (getline(newfile, tp)) {
      // If first line, extract number of nodes
      if (i == 1) {
        try {
          nodes = std::stoi(tp);
        } catch (...) {
          std::cout << "Invalid file format" << std::endl;
          return 0;
        }
      } else {
        // For all other lines, extract adjacent nodes
        std::cout << tp << std::endl;
        uint64_t src_node = 0;
        uint64_t dest_node = 0;
        size_t it = tp.find_first_of(" ");
        std::string src = tp.substr(0, it);
        try {
          src_node = std::stoi(src);
        } catch (...) {
          std::cout << "Invalid file format" << std::endl;
          return 0;
        }
        tp = tp.substr(it + 1, tp.length() - it);
        try {
          dest_node = std::stoi(tp);
        } catch (...) {
          std::cout << "Invalid file format" << std::endl;
          return 0;
        }
        adj_list.push_back(GraphQueryEngine::Graph::Edge(src_node, dest_node));
        tp.clear();
      }
      i++;
    }
    newfile.close(); // close the file object.
  }
  client.PostGraphRequest(graph_name, adj_list, nodes);

  return 0;
}

int ProcessCliInput(GraphEngineClient &client, std::string &input) {

  // Parse command
  size_t it = input.find_first_of(" ");
  std::string command = input.substr(0, it);
  input = input.substr(it + 1, input.length() - it);

  if (command.compare("POST_GRAPH") == 0) {
    // Extract graph-name
    size_t it_n = input.find_first_of(" ");
    std::string graph_name = input.substr(0, it_n);
    input = input.substr(it_n + 1, input.length() - it_n);
    return ProcessCliPost(client, graph_name, input);
  } else if (command.compare("MIN_DISTANCE") == 0) {
    // Extract graph-id
    size_t it_g = input.find_first_of(" ");
    std::string graph_id = input.substr(0, it_g);
    uint64_t id = 0;
    try {
      id = std::stoull(graph_id);
    } catch (...) {
      std::cout << "Invalid command, please check" << std::endl;
      return 0;
    }
    input = input.substr(it_g + 1, input.length() - it_g);
    // Extract source
    size_t it_s = input.find_first_of(" ");
    std::string src = input.substr(0, it_s);
    uint32_t src_node = 0;
    try {
      src_node = std::stoi(src);
    } catch (...) {
      std::cout << "Invalid command, please check" << std::endl;
      return 0;
    }
    input = input.substr(it_s + 1, input.length() - it_s);
    // Extract destination
    uint32_t dest_node = 0;
    try {
      dest_node = std::stoi(input);
    } catch (...) {
      std::cout << "Invalid command, please check" << std::endl;
      return 0;
    }
    // Make RPC call after extraction
    client.CalculateMinDistanceRequest(id, src_node, dest_node);
    return 0;
  } else if (command.compare("DELETE_GRAPH") == 0) {
    // Extract graph-id
    uint64_t id = 0;
    try {
      id = std::stoull(input);
    } catch (...) {
      std::cout << "Invalid command, please check" << std::endl;
      return 0;
    }
    // Make RPC call after extraction
    client.DeleteGraphRequest(id);
    return 0;
  } else if (command.compare("QUIT") == 0) {
    return 1;
  } else {
    std::cout << "Invalid command, please check" << std::endl;
    return 0;
  }

  return 1;
}

int main(int argc, char **argv) {

  // Instantiate the client. It requires a channel, out of which the actual RPCs
  // are created. This channel models a connection to an endpoint (in this case,
  // localhost at port 50051). We indicate that the channel isn't authenticated
  // (use of InsecureChannelCredentials()).
  GraphEngineClient graph_client(grpc::CreateChannel(
      "localhost:50051", grpc::InsecureChannelCredentials()));

  // Spawn reader thread that loops indefinitely
  std::thread thread_ =
      std::thread(&GraphEngineClient::AsyncCompleteRpc, &graph_client);

  std::string user_input;

  std::cout << "Graph Engine CLI Usage: " << std::endl;
  std::cout << "<CMD> [options]" << std::endl;
  std::cout << "POST_GRAPH <graph-name> <path-to-graph-file>" << std::endl;
  std::cout << "MIN_DISTANCE <graph-id> <source_node> <destination_node>"
            << std::endl;
  std::cout << "DELETE_GRAPH <graph-id>" << std::endl;
  std::cout << "QUIT" << std::endl << std::endl;
  std::cout << "Waiting on user input ..." << std::endl;
  while (1) {
    user_input.clear();
    std::getline(std::cin, user_input);
    int status = ProcessCliInput(graph_client, user_input);
    if (status == 1) {
      break;
    }
  }

  std::cout << "Press control-c to quit" << std::endl << std::endl;
  thread_.join(); // blocks forever

  return 0;
}

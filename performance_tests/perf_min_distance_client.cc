/*
 *
 * Copyright 2015 gRPC authors.
 *
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

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
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

using namespace std;
using namespace std::chrono;

// Global variable to keep count of total minimum distance computed response
int nodes_minimum_distance_count;

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

      // Capture minimum distance responses
      size_t found_min = response.find("minimum");
      if (found_min != std::string::npos) {
        nodes_minimum_distance_count++;
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

  // Post graph request
  // Adjacency List
  std::vector<GraphQueryEngine::Graph::Edge> adj_list;
  adj_list.push_back(GraphQueryEngine::Graph::Edge(0, 1));
  adj_list.push_back(GraphQueryEngine::Graph::Edge(0, 7));
  adj_list.push_back(GraphQueryEngine::Graph::Edge(1, 7));
  adj_list.push_back(GraphQueryEngine::Graph::Edge(1, 2));
  adj_list.push_back(GraphQueryEngine::Graph::Edge(2, 3));
  adj_list.push_back(GraphQueryEngine::Graph::Edge(2, 5));
  adj_list.push_back(GraphQueryEngine::Graph::Edge(2, 8));
  adj_list.push_back(GraphQueryEngine::Graph::Edge(3, 4));
  adj_list.push_back(GraphQueryEngine::Graph::Edge(4, 5));
  adj_list.push_back(GraphQueryEngine::Graph::Edge(5, 6));
  adj_list.push_back(GraphQueryEngine::Graph::Edge(6, 7));
  adj_list.push_back(GraphQueryEngine::Graph::Edge(7, 8));
  adj_list.push_back(GraphQueryEngine::Graph::Edge(0, 9));
  adj_list.push_back(GraphQueryEngine::Graph::Edge(10, 11));
  adj_list.push_back(GraphQueryEngine::Graph::Edge(10, 1));
  adj_list.push_back(GraphQueryEngine::Graph::Edge(11, 17));
  adj_list.push_back(GraphQueryEngine::Graph::Edge(11, 12));
  adj_list.push_back(GraphQueryEngine::Graph::Edge(12, 13));
  adj_list.push_back(GraphQueryEngine::Graph::Edge(12, 15));
  adj_list.push_back(GraphQueryEngine::Graph::Edge(13, 4));
  adj_list.push_back(GraphQueryEngine::Graph::Edge(13, 14));
  adj_list.push_back(GraphQueryEngine::Graph::Edge(15, 16));
  adj_list.push_back(GraphQueryEngine::Graph::Edge(16, 17));
  adj_list.push_back(GraphQueryEngine::Graph::Edge(17, 3));

  // Initialize minimum distance response count to 0
  nodes_minimum_distance_count = 0;

  // Post graph operation
  std::string graph_name = "datacenter_network";
  graph_client.PostGraphRequest(graph_name, adj_list,
                                18); // The actual RPC call!

  // Calculate minimum distance between nodes for Graph
  uint64_t graph_id = 0;
  while (1) {
    if (graph_client.graph_ids.size() != 0) {
      graph_id = graph_client.graph_ids[0];
      break;
    } else {
      usleep(100);
    }
  }

  // Perform 10000 minimum distance operations
  auto start = high_resolution_clock::now();
  for (int i = 0; i < 10000; i++) {
    // Generate random source between 0-17
    int src = rand() % 18;
    // Generate random destination between 0-17
    int dest = rand() % 18;
    // Calculate minimum distance
    graph_client.CalculateMinDistanceRequest(graph_id, src, dest);
  }

  while (1) {
    if (nodes_minimum_distance_count == 10000) {
      break;
    }
  }
  auto stop = high_resolution_clock::now();
  auto duration = duration_cast<microseconds>(stop - start);
  std::cout << "Time taken to perform 10000 minimum distance queries for a 18 "
               "node graph is "
            << duration.count() << " microseconds" << std::endl;

  // Delete graph from server with graph_id
  graph_client.DeleteGraphRequest(graph_id);

  std::cout << "Press control-c to quit" << std::endl << std::endl;
  thread_.join(); // blocks forever

  return 0;
}

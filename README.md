# graph_query_engine

## Overview
Graph Query Engine with grpc/protobuf support (in C++).
```
Operations supported by server:

- Post a graph, returning an ID to be used in subsequent operations
- Get the shortest path between two vertices in a previously posted graph
- Delete a graph from the server
```


## Building the Code

Build requires Bazel installation and the development is done on MacOS.

Here is the MacOS version used for development and test:
```
$:graph-query-engine rkavuluru$ system_profiler SPSoftwareDataType
Software:

    System Software Overview:

      System Version: macOS 10.14.6 (18G4032)
      Kernel Version: Darwin 18.7.0
      Boot Volume: Macintosh HD
      Boot Mode: Normal
      Computer Name: $
      User Name: Kavuluru, Ramachaitanya (rkavuluru)
      Secure Virtual Memory: Enabled
      System Integrity Protection: Enabled
      Time since boot: 36 days 7:59

$:graph-query-engine rkavuluru$ 
```
Here is the CPU Information used for performance measurements:
```
$:graph-query-engine rkavuluru$ sysctl -n machdep.cpu.brand_string
Intel(R) Core(TM) i7-7920HQ CPU @ 3.10GHz
$:graph-query-engine rkavuluru$ 
```
Here are the instructions to install Bazel on MacOS:
```
$ curl -LO https://github.com/bazelbuild/bazel/releases/download/2.0.0/bazel-2.0.0-installer-darwin-x86_64.sh
$ chmod +x bazel-2.0.0-installer-darwin-x86_64.sh
$ ./bazel-2.0.0-installer-darwin-x86_64.sh --user
```
Remember to update your $PATH after installation!

To build all binaries, run the following from project root:
```
bazel build :all
```
Once this is complete, all the binaries will be in `./bazel-bin` directory.

To build a specific package, refer `BUILD` file:
```
eg: To build async_server alone, run:
bazel build :async_server
```
## Executing and Running the Code

To build and run the server and client, run the following:
```
bazel run :async_server
bazel run :async_client
```

## Testing the code

There are two directories to perform unit and framework tests in the repository.

The unit tests are primarily for the graph engine. Following are the cases validated:
```
1. Posting same graphs twice should result in error
2. Deleting a non-existent graph from graph-db should result in error
3. Computing minimum distance from a node to itself should result in 0
4. Computing minimum distance for an unreachable node in a disconnected graph should
   result in `std::numeric_limits<uint32_t>::max()`
```

The framework tests are to verify end-to-end behavior. Following are the cases validated:
```
1. Posting a graph successfully to server
2. Computing minimum distance between 2 nodes of a graph posted on server
3. Deleting a graph from server
```

## Performance analysis

The performance tests directory measures time taken to peform operations. Following are the
cases validated:
```
1. Perform 100000 loads and deletes from the server
2. Perform 10000 minimum distance queries to the server
```
Here are some results of the experiments run on the machine specified above:
```
$:graph-query-engine rkavuluru$ ./bazel-bin/perf_load_client
Time taken to perform 30000 loads for a 9 node unique graphs is 1614684 microseconds
Time taken to perform 30000 deletes for a 9 node unique graphs is 2727893 microseconds

$:graph-query-engine rkavuluru$ ./bazel-bin/perf_load_client
Time taken to perform 300000 loads for a 9 node unique graphs is 20612870 microseconds
Time taken to perform 300000 deletes for a 9 node unique graphs is 55352333 microseconds

$:graph-query-engine rkavuluru$ ./bazel-bin/perf_min_distance_client
...
Client received: OK, found minimum distance between 15 5 to be 5
Client received: OK, found minimum distance between 10 9 to be 4294967295
Client received: OK, found minimum distance between 8 8 to be 0
Time taken to perform 10000 minimum distance queries for a 18 node graph is 918873 microseconds

std::outs for minimum distance performance have been left to present a view of actual execution.
These can be muted, to measure the performance accurately.
```

## Concurrency
To perform concurrent experiments, please follow the below procedure:
```
1. Start async_server on a single terminal -> ./bazel-bin/async_server
2. Start perf_load_client on another terminal -> ./bazel-bin/perf_load_client
3. Start perf_min_distance_client simultaneously on another terminal -> ./bazel-bin/perf_min_distance_client
```

Following are some performance measurements during concurrency on the machine specified above
```
$:graph-query-engine rkavuluru$ ./bazel-bin/perf_load_client
Time taken to perform 300000 loads for a 9 node unique graphs is 22801015 microseconds
Time taken to perform 300000 deletes for a 9 node unique graphs is 77312802 microseconds
Press control-c to quit

$:graph-query-engine rkavuluru$ ./bazel-bin/perf_min_distance_client
...
client received: OK, found minimum distance between 11 17 to be 1
Client received: OK, found minimum distance between 5 10 to be 4294967295
Time taken to perform 10000 minimum distance queries for a 18 node graph is 5105756 microseconds
Press control-c to quit

Client received: OK, deleted graph with ID: 11611133480338205011
```

## Enhancements and Future Work
Following is identified for future work:
```
1. Graph input error handling
   Here, the system assumes that the user provides good and clean input,
   wherein, for n nodes; the nodes are numbered from 0 to n-1 and has a
   corresponding definition for adjacency list.
   In cases, where the user provides wrong input, the system behavior is
   not documented and needs to be handled gracefully.
2. Graceful shutdown of RPC server and client.
   Here, there should be a separate `RequestType` called `SHUTDOWN`, on
   which the server will stop receiving `requests` and drain existing
   `responses`, close the connection and shutdown. There are errors
   generated by the system sometimes during high load and abrupt interrupts
   2.a. Graceful signal handling
        This is similar to above, except signals like SIG_INT, SIG_ABORT are
        gracefully handled instead of shutting down or crashing the system.
3. Graceful handling of RPC statuses from the server.
   Instead of sending string responses, the status code of the RPC could be
   utilized for further procedural handling of the system.
4. Load testing the server with scaling graph nodes.
   Current tests validate the performance for a graph of maximum 18 nodes.
   Despite the system could do more than that, the limits are not documented.
5. Obtain CPU and Memory utilization of the server, currently only time spent
   on CPU is recorded for performance. For memory management, we could run
   `valgrind` to identify leaks.
6. Setup CI job, to check for memory leaks using coverity/valgrind.
```

## Known Bugs:
Sometime when the system is tested for its limits, we run into the following
error, that crashes the server:
```
E0816 11:18:41.651092000 4589630912 async_server.cc:163]       assertion failed: ok
Abort trap: 6

This is due to the lack of graceful handling of signals. On those ocassions, please
increase the VM limits or reduce the load experimenting with.
```

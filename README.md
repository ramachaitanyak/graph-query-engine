# graph_query_engine

## Overview
Graph Query Engine with grpc/protobuf support (in C++).

## Building and Running the Code

Build requires Bazel installation and the development is done on MacOS.

Here is the MacOS version used for development and test:
```
LM-SJL-11014171:graph-query-engine rkavuluru$ system_profiler SPSoftwareDataType
Software:

    System Software Overview:

      System Version: macOS 10.14.6 (18G4032)
      Kernel Version: Darwin 18.7.0
      Boot Volume: Macintosh HD
      Boot Mode: Normal
      Computer Name: LM-SJL-11014171
      User Name: Kavuluru, Ramachaitanya (rkavuluru)
      Secure Virtual Memory: Enabled
      System Integrity Protection: Enabled
      Time since boot: 36 days 7:59

LM-SJL-11014171:graph-query-engine rkavuluru$ 
```
Here is the CPU Information used for performance measurements:
```
LM-SJL-11014171:graph-query-engine rkavuluru$ sysctl -n machdep.cpu.brand_string
Intel(R) Core(TM) i7-7920HQ CPU @ 3.10GHz
LM-SJL-11014171:graph-query-engine rkavuluru$ 
```
Here are the instructions to install Bazel on MacOS:
```
$ curl -LO https://github.com/bazelbuild/bazel/releases/download/2.0.0/bazel-2.0.0-installer-darwin-x86_64.sh
$ chmod +x bazel-2.0.0-installer-darwin-x86_64.sh
$ ./bazel-2.0.0-installer-darwin-x86_64.sh --user
```
Remember to update your $PATH after installation!

To only build the server and client, run the following:
```
bazel build :async_server
bazel build :async_client
```

To build and run the server and client, run the following:
```
bazel run :async_server
bazel run :async_client
```

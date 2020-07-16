#include <assert.h>
#include <iostream>
#include <stdarg.h>
#include <unistd.h>
#include <set>
#include "viaduct_runtime.h"

#include <ENCRYPTO_utils/socket.h>

// ViaductProcessRuntime
ViaductProcessRuntime::ViaductProcessRuntime(
    ViaductRuntime& runtime,
    pid self_id,
    hostid host,
    ViaductProcess& process
) : runtime(runtime), self_id(self_id), host(host), process(process) {}

int ViaductProcessRuntime::receive(pid sender) {
  return this->runtime.receive(this->self_id, sender);
}

void ViaductProcessRuntime::send(pid receiver, int val) {
  runtime.send(this->self_id, receiver, val);
}

pid ViaductProcessRuntime::getId() { return this->self_id; }

hostid ViaductProcessRuntime::getHost() { return this->host; }

int ViaductProcessRuntime::input() {
  int in;
  std::cin >> in;
  return in;
}

void ViaductProcessRuntime::output(int value) {
  std::cout << "out: " << value << "\n";
}

void ViaductProcessRuntime::operator()() {
  this->process.run(*this);
}

// ViaductRuntimeSenderThread
ViaductRuntimeSenderThread::ViaductRuntimeSenderThread(
  CSocket* socket,
  SyncQueue<Message>* msgQueue,
  ViaductRuntime& runtime
) : socket(socket), msgQueue(msgQueue), runtime(runtime) {}

void ViaductRuntimeSenderThread::operator()() {
  while (true) {
    Message msg = this->msgQueue->dequeue();

    // process-to-process message, send over socket
    if (msg.sender != VIADUCT_RUNTIME_SYSTEM_PID) {
      size_t sent_bytes = this->socket->Send(&msg, sizeof(msg));
      assert(sent_bytes == sizeof(msg));

    } else { // internal runtime message
      switch (msg.value) {
        // runtime is shutting down, break out of loop
        case VIADUCT_RUNTIME_SHUTDOWN:
          return;
          break;
        
        default:
          break;
      }
    }
  }
}

// ViaductRuntimeReceiver
ViaductRuntimeReceiverThread::ViaductRuntimeReceiverThread(
  CSocket* socket,
  SyncQueue<Message>* msgQueue,
  ViaductRuntime& runtime
) : socket(socket), msgQueue(msgQueue), runtime(runtime) {}

void ViaductRuntimeReceiverThread::operator()() {
  while (true) {
    Message msg = this->msgQueue->dequeue();

    if (msg.receiver != VIADUCT_RUNTIME_SYSTEM_PID) {
      Message receivedMsg;
      size_t receivedSize = this->socket->Receive(&receivedMsg, sizeof(receivedMsg));
      assert(receivedSize == sizeof(receivedMsg));

      this->runtime.send(receivedMsg.sender, receivedMsg.receiver, receivedMsg.value);

    } else {
      switch (msg.value) {
        // runtime is shutting down, break out of loop
        case VIADUCT_RUNTIME_SHUTDOWN:
          return;
          break;
        
        default:
          break;
      }
    }
  }
}

// ViaductRuntime
ViaductRuntime::ViaductRuntime(hostid host)
  : hostMap(), channelMap(), processMap(), host(host) {}

ViaductRuntime::~ViaductRuntime() {}

void ViaductRuntime::registerHost(hostid id, std::string& ip, uint16_t port) {
  auto hinfo = std::make_unique<HostInfo>();
  hinfo->id = id;
  hinfo->ip_address = &ip;
  hinfo->port = port;
  hinfo->sendQueue = std::make_unique<SyncQueue<Message>>();
  hinfo->recvQueue = std::make_unique<SyncQueue<Message>>();
  hostMap[id] = std::move(hinfo);
}

void ViaductRuntime::registerProcess(
    pid proc_id, hostid host, ViaductProcess& process
) {
  if (processMap.count(proc_id) == 0) {
    auto procRuntime = std::make_unique<ViaductProcessRuntime>(*this, proc_id, host, process);
    processMap[proc_id] = std::move(procRuntime);

    // create channels
    std::vector<pid> otherProcesses;
    for (channel_map::iterator it = this->channelMap.begin(); it != this->channelMap.end(); it++)
    {
      it->second[proc_id] = std::make_unique<SyncQueue<int>>();
      otherProcesses.push_back(it->first);
    }

    // create empty map
    this->channelMap[proc_id];
    for (std::vector<pid>::iterator it = otherProcesses.begin(); it != otherProcesses.end(); it++)
    {
      this->channelMap[proc_id][*it] = std::make_unique<SyncQueue<int>>();
    }
  }
}

int ViaductRuntime::receive(pid self, pid sender) {
  hostid senderHost = this->processMap[sender]->getHost();

  if (senderHost != this->host) { // remote communication
    // send request to receiver thread to read from socket
    Message msg;
    msg.sender = sender;
    msg.receiver = self;
    msg.value = 0;

    this->hostMap[senderHost]->recvQueue->enqueue(msg);
  } 

  return this->channelMap[sender][self]->dequeue();
}

void ViaductRuntime::send(pid self, pid receiver, int val) {
  hostid receiverHost = this->processMap[receiver]->getHost();

  if (receiverHost == this->host) { // local communication
    this->channelMap[self][receiver]->enqueue(val);

  } else { // remote communication
    Message msg;
    msg.sender = self;
    msg.receiver = receiver;
    msg.value = val;

    hostid receiverHost = this->processMap[receiver]->getHost();
    this->hostMap[receiverHost]->sendQueue->enqueue(msg);
  }
}

hostid ViaductRuntime::getHost() { return this->host; }

// create pairwise connections between all hosts
// for hosts i and j where i < j, i listens and j connects
bool ViaductRuntime::createRemoteConnections() {
  std::set<hostid> hostsToConnect;

  for (host_map::iterator it = hostMap.begin(); it != hostMap.end(); it++) {
    // connect to hosts with lower ID
    if (it->first < this->host) {
      auto socket = std::make_unique<CSocket>();

      bool connected = false;
      for (uint32_t i = 0; i < VIADUCT_CONNECTION_NUM_RETRY && !connected; i++) {
        connected = socket->Connect(*it->second->ip_address, it->second->port);
        usleep(VIADUCT_CONNECTION_RETRY_DELAY * 1000);
      }

      if (!connected) {
        return false;
      }

      // handshake: send host ID to identify yourself
      socket->Send(&this->host, sizeof(this->host));

      // add to socket map
      this->hostMap[it->first]->socket = std::move(socket);

    } else if (it->first > this->host) {
      hostsToConnect.insert(it->first);
    }
  }

  // listen to all hosts with higher ID
  if (!hostsToConnect.empty()) {
    auto listen_socket = std::make_unique<CSocket>();

    std::string* host_address = this->hostMap[this->host]->ip_address;
    uint16_t host_port = this->hostMap[this->host]->port;

    if (!listen_socket->Bind(*host_address, host_port)) {
      std::cerr << "Error: a socket could not be bound\n";
      return false;
    }

    if (!listen_socket->Listen()) {
      std::cerr << "Error: could not listen on the socket \n";
      return false;
    }

    while (!hostsToConnect.empty()) {
      auto sock = listen_socket->Accept();
      if (!sock) {
        std::cerr << "Error: could not accept connection\n";
        return false;
      }

      // receive host ID as handshake
      hostid connectingHost;
      sock->Receive(&connectingHost, sizeof(connectingHost));

      // add to socket map
      this->hostMap[connectingHost]->socket = std::move(sock);

      hostsToConnect.erase(connectingHost);
    }
  }

  return true;
}

bool ViaductRuntime::run() {
  if (!this->createRemoteConnections()) {
    std::cerr << "could not create all needed connections" << std::endl;
    return false;
  }

  // create sender and receiver threads for remote communication
  std::vector<std::thread> remoteCommunicationThreads;
  for (host_map::iterator it = hostMap.begin(); it != hostMap.end(); it++) {
    if (it->first != this->host) {
      SyncQueue<Message>* sendQueue = hostMap[it->first]->sendQueue.get();
      SyncQueue<Message>* recvQueue = hostMap[it->first]->recvQueue.get();
      CSocket* socket = hostMap[it->first]->socket.get();

      auto senderThread = std::thread(ViaductRuntimeSenderThread(socket, sendQueue, *this));
      auto receiverThread = std::thread(ViaductRuntimeReceiverThread(socket, recvQueue, *this));

      remoteCommunicationThreads.push_back(std::move(senderThread));
      remoteCommunicationThreads.push_back(std::move(receiverThread));
    }
  }

  // create process threads
  std::vector<std::thread> processThreads;
  for (process_map::iterator it = processMap.begin(); it != processMap.end(); it++) {
    if (it->second->getHost() == this->host) {
      processThreads.push_back(std::thread(*it->second));
    }
  }

  // cleanup
  for (uint32_t i = 0; i < processThreads.size(); i++) {
    processThreads[i].join();
  }

  // send shutdown message to break communication threads out of their loops
  for (host_map::iterator it = hostMap.begin(); it != hostMap.end(); it++) {
    Message shutdownMsg;
    shutdownMsg.sender = VIADUCT_RUNTIME_SYSTEM_PID;
    shutdownMsg.receiver = VIADUCT_RUNTIME_SYSTEM_PID;
    shutdownMsg.value = VIADUCT_RUNTIME_SHUTDOWN;

    hostMap[it->first]->sendQueue->enqueue(shutdownMsg);
    hostMap[it->first]->recvQueue->enqueue(shutdownMsg);
  }

  for (uint32_t i = 0; i < remoteCommunicationThreads.size(); i++) {
    remoteCommunicationThreads[i].join();
  }

  return true;
}

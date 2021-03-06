#ifndef __VIADUCT_RUNTIME_H__
#define __VIADUCT_RUNTIME_H__

#include <map>
#include <set>
#include <thread>
#include <vector>

#include <viaduct/SyncQueue.h>
#include <viaduct/Socket.h>

// number of times to retry connection before failing
#define VIADUCT_CONNECTION_NUM_RETRY    100

// number of milliseconds to delay connection retry
#define VIADUCT_CONNECTION_RETRY_DELAY  100 

// pid to use for internal runtime messages
#define VIADUCT_RUNTIME_SYSTEM_PID      -1

// internal runtime messages
#define VIADUCT_RUNTIME_SHUTDOWN        0

using hostid = uint32_t;
using pid = int32_t;

class ViaductProcessRuntime;

// Executes the code for the process.
class ViaductProcess {
public:
  virtual void run(ViaductProcessRuntime& runtime)=0;
};

class ViaductDefaultProcess: public ViaductProcess {
  void (*procFunction)(ViaductProcessRuntime& runtime);

public:
  ViaductDefaultProcess(void (*procFunction)(ViaductProcessRuntime&))
    : procFunction(procFunction)
  {}

  void run(ViaductProcessRuntime& runtime) {
    (*this->procFunction)(runtime);
  }
};

class ViaductRuntime;

// Runtime wrapper for a process.
class ViaductProcessRuntime {
  ViaductRuntime& runtime;
  pid self_id;
  hostid host;
  ViaductProcess& process;
public:
  ViaductProcessRuntime(
      ViaductRuntime& runtime,
      pid self_id,
      hostid host,
      ViaductProcess& process
  );

  hostid getHost();

  int receive(pid sender);

  void send(pid receiver, int val);

  hostid getParty();

  pid getId();

  int input();

  void output(int value);

  void operator()();
};

// communication

struct Message {
  pid sender;
  pid receiver;
  int value;
};

// Thread that sends data to a host socket.
class ViaductRuntimeSenderThread {
  CSocket* socket;
  SyncQueue<Message>* msgQueue;
  ViaductRuntime& runtime;

public:
  ViaductRuntimeSenderThread(CSocket* socket, SyncQueue<Message>* msgQueue, ViaductRuntime& runtime);

  void operator()();
};

// Thread that receives data from a host socket.
class ViaductRuntimeReceiverThread {
  CSocket* socket;
  SyncQueue<Message>* msgQueue;
  ViaductRuntime& runtime;

public:
  ViaductRuntimeReceiverThread(CSocket* socket, SyncQueue<Message>* msgQueue, ViaductRuntime& runtime);

  void operator()();
};

struct HostInfo {
  hostid id;
  std::string* ip_address;
  uint16_t port;
  std::unique_ptr<SyncQueue<Message>> sendQueue;
  std::unique_ptr<SyncQueue<Message>> recvQueue;
  std::unique_ptr<CSocket> socket;
};

using host_map = std::map<hostid, std::unique_ptr<HostInfo>>;
using channel_map = std::map<pid, std::map<pid, std::unique_ptr<SyncQueue<int>>>>;
using process_map = std::map<pid, std::unique_ptr<ViaductProcessRuntime>>;

// Manages host registry, process registry, and communication b/w processes.
class ViaductRuntime {
  hostid host;
  host_map hostMap;
  channel_map channelMap;
  process_map processMap;

  bool createRemoteConnections();

public:
  ViaductRuntime(hostid party);

  ~ViaductRuntime();

  void registerHost(hostid id, std::string& ip, uint16_t port);

  void registerProcess(pid proc_id, hostid host, ViaductProcess& process);

  int receive(pid self, pid sender);

  void send(pid self, pid receiver, int val);

  hostid getHost();

  bool run();
};

#endif

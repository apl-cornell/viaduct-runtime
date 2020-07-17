#include <viaduct/ViaductRuntime.h>

#define alice 0
#define bob 1

#define Local__alice__at__alice 0
#define Local__bob__at__bob 5

void func_Local__alice__at__alice(ViaductProcessRuntime& runtime) {
    runtime.send(Local__bob__at__bob, 10);
    int y = runtime.receive(Local__bob__at__bob);
    runtime.output(y);
}

void func_Local__bob__at__bob(ViaductProcessRuntime& runtime) {
    int x = runtime.receive(Local__alice__at__alice);
    runtime.send(Local__alice__at__alice, x*2);
    runtime.output(x);
}

void start(ViaductRuntime& runtime) {
    ViaductDefaultProcess proc_Local__alice__at__alice(func_Local__alice__at__alice);
    ViaductDefaultProcess proc_Local__bob__at__bob(func_Local__bob__at__bob);

    std::string localhost = "127.0.0.1";
    runtime.registerHost(alice, localhost, 5000);
    runtime.registerHost(bob, localhost, 5001);
    runtime.registerProcess(Local__alice__at__alice, 0, proc_Local__alice__at__alice);
    runtime.registerProcess(Local__bob__at__bob, 1, proc_Local__bob__at__bob);
    runtime.run();
}


int main(int argc, char** argv) {
  hostid host = atoi(argv[1]);
  ViaductRuntime runtime(host);
  start(runtime);

  return 0;
}
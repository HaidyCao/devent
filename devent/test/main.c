#include <stdio.h>
#include <signal.h>
#include "echo_server_test.h"
#include "telnet_test.h"
#include "buffer_test.h"
#include "ssl_telnet_test.h"
#include "ssl_echo_server_test.h"

int main() {
  signal(SIGPIPE, SIG_IGN);

//    echo_server_start();
//    telnet_start();
//    DocketBuffer_moveto_test();
//  ssl_telnet_start();
//  echo_ssl_server_start();

  return 0;
}

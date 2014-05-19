#include "dht_protocol.h"

%%{
  machine Protocol;

  import "dht_protocol.h";

  main := Protocol;
}%%

%% write data;

dht_proto *
parse(char *data, int length){
  char *p  = data;
  char *pe = p + length;
  %% write exec;

}
#ifndef DHT_NODE_H_
#define DHT_NODE_H_

void
dht_xor(unsigned char[32], unsigned char[32], unsigned char[32]);

int
dht_compare(unsigned char[32], unsigned char[32]);

#endif
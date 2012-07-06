#define KEY_LENGTH 256
#define NONCE_LENGTH 20
#define DISCARD_BYTES 1024
#define DIFFICULTY 200000 /* We throw away DISCARD_BYTES*DIFFICULTY bytes */

typedef unsigned char byte;
typedef unsigned long long uint64;


void initKey(char *password, byte *nonce, int nonceLength);
void throwAwaySomeBytes(int numBytes);
void clearKey(void);
byte hashChar(byte c);

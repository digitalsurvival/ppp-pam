/* Entropy source tests; to be piped into rngtest from rng-tools
 * (C) 2009 by Tomasz bla Fortuna
 */

#include <stdio.h>
#include <string.h>
#include <uuid/uuid.h>
#include <openssl/rand.h>
#include "../sha2/sha2.h"

int progressRead(const char *msg, unsigned char *buf, const int count)
{
	const char spinner[] = "|/-\\"; //".oO0Oo. ";
	const int size = strlen(spinner);
	int i;
	FILE *f;
	f = fopen("/dev/random", "r");
	if (!f) {
		return 0;
	}

	for (i=0; i<count; i++) {
		buf[i] = fgetc(f);
		if (i%11 == 0) {
			printf("\r%s %3d%%  %c ", msg, i*100 / count, spinner[i/11 % size]);
			fflush(stdout);
		}
	}
	printf("\r%s 100%%  %c \n", msg, spinner[i/11 % size]);
	return 1; // Success;
}

int main(int argc, char **argv)
{
	uuid_t uuid;
	int i;
	FILE *f;
	unsigned char entropy_pool[256];
	int entropy;
	unsigned char bytes[256];
	int time = 0;
	memset(entropy_pool, 0, sizeof(entropy_pool));

//	f = fopen("/dev/random", "r"); // Slow but best
	f = fopen("/dev/urandom", "r"); // Fast but not so good
	if (!f)
		return 1;

	int method = 1;
	for (;;) {
		switch (method) {
		case 0:
			/* Dummy input data; with sha 44/50000 fail rate
			 * without sha - 50000/50000 fail rate  */
			memcpy(entropy_pool, &time, sizeof(time));
			time++;
			entropy = 32;
			break;
		case 1:
			/* With sha ~35/50000 fail rate; without 50000/50000 */
			uuid_generate_time(uuid);
			memcpy(entropy_pool, (unsigned char *) &uuid, sizeof(uuid));

			uuid_generate_random(uuid);
			memcpy(entropy_pool+16, (unsigned char *) &uuid, sizeof(uuid));

			entropy = 32;
			break;
		case 2:
			/* With and without similar fail rate of 40/50000 */
			if (RAND_bytes(entropy_pool, 64) == 0)
				return 1;
			entropy = 64;
			break;

		case 3:
			/* Slowest, but good */
			if (fread(entropy_pool, 32, 1, f) != 1)
				return 1;
			entropy = 32;
		}
	
		// Do the sha256
/*		sha256(entropy_pool, entropy, bytes);
		if (fwrite(bytes, 32, 1, stdout) != 1)
			return 1;
*/

		// Instead of sha, just copy the bytes		
		memcpy(bytes, entropy_pool, entropy);
		if (fwrite(bytes, entropy, 1, stdout) != 1)
			return 1;

	}

	return 0;
}

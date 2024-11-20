#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#define MODE     1          // 1 = hash each word individually; 2 = hash the entire file
#define MAXLEN   50         // maximum length of a word
#define MAXWORDS 500000     // maximum number of words in a file
#define SEED     0x12345678


#define ROTL(x,bits) x<<bits | x>>(32-bits);
#if MODE == 1
char words[MAXWORDS][MAXLEN];
#else
char file[MAXWORDS * MAXLEN];
#endif

uint32_t DJB2_hash(const uint8_t *str)
{
    uint32_t hash = 5381;
    uint8_t c;
    while ((c = *str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    return hash;
}

uint32_t FNV(const void* key, uint32_t h)
{
    // See: https://github.com/aappleby/smhasher/blob/master/src/Hashes.cpp
    h ^= 2166136261UL;
    const uint8_t* data = (const uint8_t*)key;
    for(int i = 0; data[i] != '\0'; i++)
    {
        h ^= data[i];
        h *= 16777619;
    }
    return h;
}

uint32_t MurmurOAAT_32(const char* str, uint32_t h)
{
    // One-byte-at-a-time hash based on Murmur's mix
    // Source: https://github.com/aappleby/smhasher/blob/master/src/Hashes.cpp
    for (; *str; ++str) {
        h ^= *str;
        h *= 0x5bd1e995;
        h ^= h >> 15;
    }
    return h;
}

uint32_t KR_v2_hash(const char *s)
{
    // Source: https://stackoverflow.com/a/45641002/5407270
    // a.k.a. Java String hashCode()
    uint32_t hashval = 0;
    for (hashval = 0; *s != '\0'; s++)
        hashval = *s + 31*hashval;
    return hashval;
}

uint32_t Jenkins_one_at_a_time_hash(const char *str)
{
    uint32_t hash, i;
    for (hash = i = 0; str[i] != '\0'; ++i)
    {
        hash += str[i];
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }
    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);
    return hash;
}

uint32_t crc32b(const uint8_t *str) {
    // Source: https://stackoverflow.com/a/21001712
    unsigned int byte, crc, mask;
    int i = 0, j;
    crc = 0xFFFFFFFF;
    while (str[i] != 0) {
        byte = str[i];
        crc = crc ^ byte;
        for (j = 7; j >= 0; j--) {
            mask = -(crc & 1);
            crc = (crc >> 1) ^ (0xEDB88320 & mask);
        }
        i = i + 1;
    }
    return ~crc;
}

inline uint32_t _rotl32(uint32_t x, int32_t bits)
{
    return x<<bits | x>>(32-bits);      // C idiom: will be optimized to a single operation
}

uint32_t Coffin_hash(char const *input) { 
    // Source: https://stackoverflow.com/a/7666668/5407270
    uint32_t result = 0x55555555;
    while (*input) { 
        result ^= *input++;
        result = ROTL(result, 5);
    }
    return result;
}

uint32_t x17(const void * key, uint32_t h)
{
    // See: https://github.com/aappleby/smhasher/blob/master/src/Hashes.cpp
    const uint8_t * data = (const uint8_t*)key;
    for (int i = 0; data[i] != '\0'; ++i)
    {
        h = 17 * h + (data[i] - ' ');
    }
    return h ^ (h >> 16);
}

uint32_t large_prime(const uint8_t *s, uint32_t hash)
{
    // Better alternative to x33: multiply by a large co-prime instead of a small one
    while (*s != '\0')
        hash = 17000069*hash + *s++;
    return hash;
}

void readfile(FILE* fp, char* buffer) {
    fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);
    rewind(fp);
    fread(buffer, size, 1, fp);
    buffer[size] = '\0';
}

struct timeval stop, start;
void timing_start() {
    gettimeofday(&start, NULL);
}

void timing_end() {
    gettimeofday(&stop, NULL);
    printf("took %lu us\n", (stop.tv_sec - start.tv_sec) * 1000000 + stop.tv_usec - start.tv_usec);
}

uint32_t apply_hash(int hash, const char* line)
{
    uint32_t result;
#if MODE == 2
    timing_start();
#endif
    switch (hash) {
        case 1: result = crc32b((const uint8_t*)line); break;
        case 2: result = large_prime((const uint8_t *)line, SEED); break;
        case 3: result = MurmurOAAT_32(line, SEED); break;
        case 4: result = FNV(line, SEED); break;
        case 5: result = Jenkins_one_at_a_time_hash(line); break;
        case 6: result = DJB2_hash((const uint8_t*)line); break;
        case 7: result = KR_v2_hash(line); break;
        case 8: result = Coffin_hash(line); break;
        case 9: result = x17(line, SEED); break;
        default: break;
    }
#if MODE == 2
    timing_end();
#endif
    return result;
}

int main(int argc, char* argv[])
{
    // Read arguments
    const int hash_choice = atoi(argv[1]);
    char const* const fname = argv[2];

    printf("Algorithm: %d\n", hash_choice);
    printf("File name: %s\n", fname);

    // Open file
    FILE* f = fopen(fname, "r");

#if MODE == 1
    char line[MAXLEN];
    size_t word_count = 0;
    uint32_t hashes[MAXWORDS];

    // Read file line by line
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\n")] = '\0';   // strip newline
        strcpy(words[word_count], line);
        word_count++;
    }
    fclose(f);

    // Calculate hashes
    timing_start();
    for (size_t i = 0; i < word_count; i++) {
        hashes[i] = apply_hash(hash_choice, words[i]);
    }
    timing_end();

    // Output hashes
    for (size_t i = 0; i < word_count; i++) {
        printf("%08x\n", hashes[i]);
    }

#else 
    // Read entire file
    readfile(f, file);
    fclose(f);
    printf("Len: %lu\n", strlen(file)); 

    // Calculate hash
    uint32_t hash = apply_hash(hash_choice, file);
    printf("%08x\n", hash);
#endif

    return 0;
}

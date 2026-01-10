#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h> // for stderr, printf etc
// Define the data structure (must be identical to the writer's definition)
typedef struct {
int counter;
char buffer[256];
} SharedData;
int main(int argc, char* argv[]) {
if (argc != 2) {
fprintf(stderr, "Usage: %s <SHM_NAME>",  argv[0]);
return 1;
}
const char* SHM_NAME = argv[1];
const size_t SHM_SIZE = 1024;
int shm_fd;
SharedData* shm_ptr = NULL;
// 1. Open the existing shared memory object
shm_fd = shm_open(SHM_NAME, O_RDONLY, 0666);
if (shm_fd == -1) {
perror("[READER] shm_open failed");
return 1;
}
// 2. Map the shared memory object into the process's address space (read-only)
shm_ptr = (SharedData*) mmap(NULL, SHM_SIZE, PROT_READ, MAP_SHARED, shm_fd, 0);
if (shm_ptr == MAP_FAILED) {
perror("[READER] mmap failed");
return 1;
}
printf("[READER] Child Process (PID: %d) connected to shared memory.\n", getpid());
// 3. Continuously read until termination signal (-1)
while (shm_ptr->counter != -1) {
if (shm_ptr->counter > 0) {
printf("[READER] Received Counter: %d  | Message: %s \n", shm_ptr->counter,shm_ptr->buffer );
}
usleep(500000); // Wait 0.5 seconds before checking again
}
printf("[READER] Received termination signal. Exiting.\n");
// Cleanup: Unmap the memory (unlink is done by the writer/parent)
munmap(shm_ptr, SHM_SIZE);
close(shm_fd); // Close the file descriptor
return 0;
}
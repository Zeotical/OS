#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h> // for stderr, printf etc
// Define constants for the shared memory
const char* SHM_NAME = "/my_ipc_shm";
const size_t SHM_SIZE = 1024;
const char* MESSAGE = "Hello from the Writer! Data cycle:\n";
// Define the data structure to be shared
typedef struct {
int counter;
char buffer[256];
}SharedData ;
void run_writer(SharedData* shm_ptr, pid_t reader_pid) {
printf("[WRITER] Parent Process (PID: %d) started.\n" , getpid());
printf("[WRITER] Sending 3 messages to Child (PID: %d)...\n", reader_pid);
for (int i = 1; i <= 3; ++i) {
// Write data into the shared memory
shm_ptr->counter = i;
char str[20]; // Buffer to hold the string why we usin 20 here chat?
sprintf(str, "%d", i); // Converts int to string
const char* full_msg = MESSAGE;
strncpy(shm_ptr->buffer, full_msg, sizeof(shm_ptr->buffer) - 1);
shm_ptr->buffer[sizeof(shm_ptr->buffer) - 1] = '\0';
strncpy(shm_ptr->buffer + strlen(shm_ptr->buffer), str, strlen(str)+1);

printf("[WRITER] Wrote message %d. Pausing...\n",i);
sleep(2); // Pause to let the reader read
}
// Set a stop signal for the reader
shm_ptr->counter = -1;
printf("[WRITER] Sent termination signal (-1). Waiting for reader...\n");
}
int main() {
int shm_fd;
SharedData* shm_ptr = NULL;
// 1. Create and open the shared memory object
shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
if (shm_fd == -1) {
perror("shm_open failed");
return 1;
}
// 2. Set the size of the shared memory object
if (ftruncate(shm_fd, SHM_SIZE) == -1) {
perror("ftruncate failed");
shm_unlink(SHM_NAME);
return 1;
}
// 3. Map the shared memory object into the process's address space
shm_ptr = (SharedData*)mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED,
shm_fd, 0);
if (shm_ptr == MAP_FAILED) {
perror("mmap failed");
shm_unlink(SHM_NAME);
return 1;
}
// Initialize the shared data structure
shm_ptr->counter = 0;
// --- Fork the Child (Reader) Process ---
pid_t pid = fork();
if (pid < 0) {
perror("fork failed");
munmap(shm_ptr, SHM_SIZE);
shm_unlink(SHM_NAME);
return 1;
} else if (pid == 0) {
// Child Process: Execute the Reader Program
// Execute the compiled reader program, passing the SHM_NAME as an argument
execl("./shmreader", "shmreader", SHM_NAME, NULL);
// execl only returns if an error occurred
perror("execl failed");
munmap(shm_ptr, SHM_SIZE);
shm_unlink(SHM_NAME);
exit(EXIT_FAILURE);
} else {
// Parent Process: Run the Writer logic
run_writer(shm_ptr, pid);
// Wait for the child to finish
wait(NULL);
// Cleanup: Unmap the memory and remove the shared object
munmap(shm_ptr, SHM_SIZE);
shm_unlink(SHM_NAME);
printf("[WRITER] Parent finished and cleaned up shared memory.\n");
}
return 0;
}
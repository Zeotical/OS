cmake_minimum_required(VERSION 3.0.0)
project (OS_Project VERSION 0.1.0 LANGUAGES C CXX)
add_executable(server server.c) # quivilaence == g++ server.c -o server
add_executable(client client.c) # quivilaence == g++ client.c -o client
set_property(TARGET client PROPERTY C_STANDARD 11)
set_property(TARGET server PROPERTY C_STANDARD 11)

# copying the idioms.txt to the build directory from the source dir
# Source - https://stackoverflow.com/a/65133324
# Posted by tdashroy, modified by community. See post 'Timeline' for change history
#  Retrieved 2026-02-02, License - CC BY-SA 4.0

    configure_file( ${CMAKE_CURRENT_LIST_DIR}/idioms.txt ${CMAKE_CURRENT_BINARY_DIR}/idioms.txt COPYONLY)



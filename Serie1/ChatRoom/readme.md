install-deps:
    sudo apt install uuid-dev

compile-server:
    gcc server.c -o server

compile-client:
    gcc client.c -o client -luuid

run-server:
    ./server <room>

run-client:
    ./client <username> <room>

example:
    ./client saeed room1

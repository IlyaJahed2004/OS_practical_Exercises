Dependencies:
Make sure libcurl is installed:

sudo apt install libcurl4-openssl-dev


Compilation:
gcc downloader.c -o downloader -lcurl -lpthread


If pthreads is missing:
sudo apt install pthreads-dev

▶Usage

Run the program with:
./downloader <url> <num_threads>

Example:
Download using 4 threads:
./downloader https://www.geeksforgeeks.org/c/c-programming-language/ 4


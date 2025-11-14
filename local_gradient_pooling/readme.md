Build
gcc lgp.c -o lgp -lrt

Run
./lgp

Optional Cleanup (remove shared memory)
rm /dev/shm/lgp_shm
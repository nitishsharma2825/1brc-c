gcc -O3 -g -march=native -fno-omit-frame-pointer yourfile.c -o yourfile

-g => adds debug info for profiles
-fno-omit-frame-pointers => helps tools like perf unwind call stacks
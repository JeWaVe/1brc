large:
	gcc -O3 -march=native main.c
	chmod u+x a.out  && ./a.out large.txt

small:
	gcc -O3 -march=native main.c
	chmod u+x a.out && ./a.out small.txt

perf:
	gcc -O3 -march=native -g -fno-omit-frame-pointer -o a.out main.c
	perf record -F 999 -g -- ./a.out small.txt
	perf script > out.perf
	../../FlameGraph/stackcollapse-perf.pl out.perf > out.folded
	../../FlameGraph/flamegraph.pl out.folded > flamegraph.svg

debug: 
	gcc -g -ggdb main.c
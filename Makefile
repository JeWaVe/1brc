large:
	gcc -O3 -march=native main.c
	chmod u+x a.out  && ./a.out large.txt

small:
	gcc -O3 -march=native main.c
	chmod u+x a.out && ./a.out small.txt

perf:
	gcc -O3 -march=native -g -fno-omit-frame-pointer -fno-optimize-sibling-calls -o a.out main.c
		perf record -F 999 -e cycles:u -g --call-graph dwarf,16384 -m 1024 -- ./a.out small.txt
	perf script --inline > out.perf
	../../FlameGraph/stackcollapse-perf.pl out.perf > out.folded
	../../FlameGraph/flamegraph.pl \
		--title "a.out — perf flamegraph" \
		--countname "samples" \
		--minwidth 0.5 \
		--width 1800 \
		out.folded > flamegraph.svg

debug: 
	gcc -g -ggdb main.c
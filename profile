perf stat -e L1-dcache-misses,L1-dcache-loads,cache-references,cache-misses,branches,branch-misses,cycles,instructions,stalled-cycles-frontend ./build/inttest/laplace_harmonic

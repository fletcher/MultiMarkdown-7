FILE_DIR = bench

.PHONY : benchmark
benchmark: bench.c $(FILE_DIR)
	gcc -o benchmark bench.c

.PHONY : files
files: benchmark $(FILE_DIR)
	./benchmark build

.PHONY : run
run: files benchmark
	./benchmark run

.PHONY : clean
clean:
	rm -rf $(FILE_DIR)/*;
	rm benchmark

# Create build directory if it doesn't exist
$(FILE_DIR): 
	-mkdir $(FILE_DIR) 2>/dev/null
	-cd $(FILE_DIR); rm -rf *

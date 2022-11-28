sim_cache: ${src}
	gcc ${src} -o sim_cache -lstdc++ -std=c++11

src_dir=./src
src=${src_dir}/*.cpp

.PHONY: clean
clean:
	rm sim_cache
all: tests/iterable_hash_test-32.test tests/spinner-32.test tests/trie-32.test tests/listset-32.test tests/arrayset-32.test tests/iterable_hash_test.test tests/spinner.test tests/trie.test tests/listset.test tests/arrayset.test lib/fileaccesses bilge git-32.o targets-32.o files-32.o bilge-32.o web/benchmarks.html web/documentation.html temp.html build-32.o web/hierarchy-doing-nothing.png web/hierarchy-doing-nothing.pdf web/hierarchy-touching-c.png web/hierarchy-touching-c.pdf web/hierarchy-touching-header.png web/hierarchy-touching-header.pdf web/hierarchy-rebuilding.png web/hierarchy-rebuilding.pdf web/hierarchy-building.png web/cat-doing-nothing.png web/cat-doing-nothing.pdf web/cat-rebuilding.png web/cat-rebuilding.pdf web/cat-building.png web/cat-building.pdf web/dependent-chain-doing-nothing.png web/dependent-chain-doing-nothing.pdf web/dependent-chain-touching-c.png web/dependent-chain-touching-c.pdf web/dependent-chain-touching-header.png web/dependent-chain-touching-header.pdf web/dependent-chain-rebuilding.png web/dependent-chain-rebuilding.pdf web/dependent-chain-building.png web/dependent-chain-building.pdf web/hierarchy-building.pdf web/style.css web/index.html .bilge

tests/iterable_hash_test-32.o : tests/iterable_hash_test.c lib/iterablehash.h
	cd tests && gcc -m32 -Wall -Werror -O2 -std=c11 -g -o iterable_hash_test-32.o -c iterable_hash_test.c

lib/trie-32.o : lib/trie.c lib/trie.h
	cd lib && gcc -m32 -Wall -Werror -O2 -std=c11 -g -o trie-32.o -c trie.c

lib/listset-32.o : lib/listset.c lib/listset.h
	cd lib && gcc -m32 -Wall -Werror -O2 -std=c11 -g -o listset-32.o -c listset.c

lib/iterablehash-32.o : lib/iterablehash.c lib/iterablehash.h
	cd lib && gcc -m32 -Wall -Werror -O2 -std=c11 -g -o iterablehash-32.o -c iterablehash.c

lib/arrayset-32.o : lib/arrayset.c lib/arrayset.h
	cd lib && gcc -m32 -Wall -Werror -O2 -std=c11 -g -o arrayset-32.o -c arrayset.c

tests/iterable_hash_test-32.test : tests/iterable_hash_test-32.o lib/trie-32.o lib/listset-32.o lib/iterablehash-32.o lib/arrayset-32.o
	gcc -m32 -lpthread -o tests/iterable_hash_test-32.test tests/iterable_hash_test-32.o lib/trie-32.o lib/listset-32.o lib/iterablehash-32.o lib/arrayset-32.o

tests/spinner-32.o : tests/spinner.c
	cd tests && gcc -m32 -Wall -Werror -O2 -std=c11 -g -o spinner-32.o -c spinner.c

tests/trie-32.o : tests/trie.c lib/trie.h
	cd tests && gcc -m32 -Wall -Werror -O2 -std=c11 -g -o trie-32.o -c trie.c

tests/trie-32.test : tests/trie-32.o lib/trie-32.o lib/listset-32.o lib/iterablehash-32.o lib/arrayset-32.o
	gcc -m32 -lpthread -o tests/trie-32.test tests/trie-32.o lib/trie-32.o lib/listset-32.o lib/iterablehash-32.o lib/arrayset-32.o

tests/listset-32.o : tests/listset.c lib/listset.h
	cd tests && gcc -m32 -Wall -Werror -O2 -std=c11 -g -o listset-32.o -c listset.c

tests/listset-32.test : tests/listset-32.o lib/trie-32.o lib/listset-32.o lib/iterablehash-32.o lib/arrayset-32.o
	gcc -m32 -lpthread -o tests/listset-32.test tests/listset-32.o lib/trie-32.o lib/listset-32.o lib/iterablehash-32.o lib/arrayset-32.o

tests/arrayset-32.o : tests/arrayset.c lib/arrayset.h
	cd tests && gcc -m32 -Wall -Werror -O2 -std=c11 -g -o arrayset-32.o -c arrayset.c

tests/iterable_hash_test.o : tests/iterable_hash_test.c lib/iterablehash.h
	cd tests && gcc -Wall -Werror -O2 -std=c11 -g -c iterable_hash_test.c

lib/trie.o : lib/trie.c lib/trie.h
	cd lib && gcc -Wall -Werror -O2 -std=c11 -g -c trie.c

lib/listset.o : lib/listset.c lib/listset.h
	cd lib && gcc -Wall -Werror -O2 -std=c11 -g -c listset.c

lib/iterablehash.o : lib/iterablehash.c lib/iterablehash.h
	cd lib && gcc -Wall -Werror -O2 -std=c11 -g -c iterablehash.c

lib/arrayset.o : lib/arrayset.c lib/arrayset.h
	cd lib && gcc -Wall -Werror -O2 -std=c11 -g -c arrayset.c

lib/syscalls.h : lib/get_syscalls.py
	python lib/get_syscalls.py /usr/src/linux-headers-3.2.0-4-common > lib/syscalls.h

lib/bigbrother.o : lib/syscalls.h lib/bigbrother.c lib/bigbrother.h lib/arrayset.h
	cd lib && gcc -Wall -Werror -O2 -std=c11 -g -c bigbrother.c

tests/iterable_hash_test.test : tests/iterable_hash_test.o lib/trie.o lib/listset.o lib/iterablehash.o lib/arrayset.o lib/bigbrother.o
	gcc -lpthread -o tests/iterable_hash_test.test tests/iterable_hash_test.o lib/trie.o lib/listset.o lib/iterablehash.o lib/arrayset.o lib/bigbrother.o

tests/spinner.o : tests/spinner.c
	cd tests && gcc -Wall -Werror -O2 -std=c11 -g -c spinner.c

tests/spinner.test : tests/spinner.o lib/trie.o lib/listset.o lib/iterablehash.o lib/arrayset.o lib/bigbrother.o
	gcc -lpthread -o tests/spinner.test tests/spinner.o lib/trie.o lib/listset.o lib/iterablehash.o lib/arrayset.o lib/bigbrother.o

tests/trie.o : tests/trie.c lib/trie.h
	cd tests && gcc -Wall -Werror -O2 -std=c11 -g -c trie.c

tests/listset.o : tests/listset.c lib/listset.h
	cd tests && gcc -Wall -Werror -O2 -std=c11 -g -c listset.c

tests/listset.test : tests/listset.o lib/trie.o lib/listset.o lib/iterablehash.o lib/arrayset.o lib/bigbrother.o
	gcc -lpthread -o tests/listset.test tests/listset.o lib/trie.o lib/listset.o lib/iterablehash.o lib/arrayset.o lib/bigbrother.o

tests/arrayset.o : tests/arrayset.c lib/arrayset.h
	cd tests && gcc -Wall -Werror -O2 -std=c11 -g -c arrayset.c

tests/arrayset.test : tests/arrayset.o lib/trie.o lib/listset.o lib/iterablehash.o lib/arrayset.o lib/bigbrother.o
	gcc -lpthread -o tests/arrayset.test tests/arrayset.o lib/trie.o lib/listset.o lib/iterablehash.o lib/arrayset.o lib/bigbrother.o

lib/fileaccesses.o : lib/fileaccesses.c lib/bigbrother.h lib/arrayset.h
	cd lib && gcc -Wall -Werror -O2 -std=c11 -g -c fileaccesses.c

lib/fileaccesses : lib/trie.o lib/listset.o lib/iterablehash.o lib/arrayset.o lib/bigbrother.o lib/fileaccesses.o
	cd lib && gcc -lpopt -lprofiler -o fileaccesses fileaccesses.o trie.o listset.o iterablehash.o arrayset.o bigbrother.o

bilge.o : bilge.c bilge.h lib/trie.h lib/iterablehash.h
	gcc -Wall -Werror -O2 -std=c11 -g -c bilge.c

files.o : files.c bilge.h lib/trie.h lib/iterablehash.h
	gcc -Wall -Werror -O2 -std=c11 -g -c files.c

targets.o : targets.c bilge.h lib/trie.h lib/iterablehash.h
	gcc -Wall -Werror -O2 -std=c11 -g -c targets.c

build.o : build.c bilge.h lib/trie.h lib/iterablehash.h lib/bigbrother.h lib/arrayset.h lib/listset.h
	gcc -Wall -Werror -O2 -std=c11 -g -c build.c

git.o : git.c bilge.h lib/trie.h lib/iterablehash.h
	gcc -Wall -Werror -O2 -std=c11 -g -c git.c

bilge : bilge.o files.o targets.o build.o git.o lib/trie.o lib/listset.o lib/iterablehash.o lib/arrayset.o lib/bigbrother.o
	gcc -lpopt -lprofiler -o bilge bilge.o files.o targets.o build.o git.o lib/trie.o lib/listset.o lib/iterablehash.o lib/arrayset.o lib/bigbrother.o

tests/trie.test : tests/trie.o lib/trie.o lib/listset.o lib/iterablehash.o lib/arrayset.o lib/bigbrother.o
	gcc -lpthread -o tests/trie.test tests/trie.o lib/trie.o lib/listset.o lib/iterablehash.o lib/arrayset.o lib/bigbrother.o

tests/arrayset-32.test : tests/arrayset-32.o lib/trie-32.o lib/listset-32.o lib/iterablehash-32.o lib/arrayset-32.o
	gcc -m32 -lpthread -o tests/arrayset-32.test tests/arrayset-32.o lib/trie-32.o lib/listset-32.o lib/iterablehash-32.o lib/arrayset-32.o

git-32.o : git.c bilge.h lib/trie.h lib/iterablehash.h
	gcc -m32 -Wall -Werror -O2 -std=c11 -g -o git-32.o -c git.c

build-32.o : build.c bilge.h lib/trie.h lib/iterablehash.h lib/bigbrother.h lib/arrayset.h lib/listset.h
	gcc -m32 -Wall -Werror -O2 -std=c11 -g -o build-32.o -c build.c

tests/spinner-32.test : tests/spinner-32.o lib/trie-32.o lib/listset-32.o lib/iterablehash-32.o lib/arrayset-32.o
	gcc -m32 -lpthread -o tests/spinner-32.test tests/spinner-32.o lib/trie-32.o lib/listset-32.o lib/iterablehash-32.o lib/arrayset-32.o

targets-32.o : targets.c bilge.h lib/trie.h lib/iterablehash.h
	gcc -m32 -Wall -Werror -O2 -std=c11 -g -o targets-32.o -c targets.c

files-32.o : files.c bilge.h lib/trie.h lib/iterablehash.h
	gcc -m32 -Wall -Werror -O2 -std=c11 -g -o files-32.o -c files.c

bilge-32.o : bilge.c bilge.h lib/trie.h lib/iterablehash.h
	gcc -m32 -Wall -Werror -O2 -std=c11 -g -o bilge-32.o -c bilge.c

web/hierarchy-building.pdf web/dependent-chain-building.pdf web/dependent-chain-building.png web/dependent-chain-rebuilding.pdf web/dependent-chain-rebuilding.png web/dependent-chain-touching-header.pdf web/dependent-chain-touching-header.png web/dependent-chain-touching-c.pdf web/dependent-chain-touching-c.png web/dependent-chain-doing-nothing.pdf web/dependent-chain-doing-nothing.png web/cat-building.pdf web/cat-building.png web/cat-rebuilding.pdf web/cat-rebuilding.png web/cat-doing-nothing.pdf web/cat-doing-nothing.png web/hierarchy-building.png web/hierarchy-rebuilding.pdf web/hierarchy-rebuilding.png web/hierarchy-touching-header.pdf web/hierarchy-touching-header.png web/hierarchy-touching-c.pdf web/hierarchy-touching-c.png web/hierarchy-doing-nothing.pdf web/hierarchy-doing-nothing.png : bench/generate-plots.py bench/catmod.pyc bench/hiermod.pyc bench/depmod.pyc bench/data/2015-01-23_fed5eab/dependent-chain/bilge\ -j4/ext4/building.txt bench/data/2015-01-23_fed5eab/dependent-chain/bilge\ -j4/tmpfs/building.txt bench/data/2015-01-23_fed5eab/dependent-chain/make\ -j4/ext4/building.txt bench/data/2015-01-23_fed5eab/dependent-chain/make\ -j4/tmpfs/building.txt bench/data/2015-01-23_fed5eab/dependent-chain/scons\ -j4/ext4/building.txt bench/data/2015-01-23_fed5eab/dependent-chain/scons\ -j4/tmpfs/building.txt bench/data/2015-01-23_fed5eab/dependent-chain/tup\ -j4/ext4/building.txt bench/data/2015-01-23_fed5eab/dependent-chain/tup\ -j4/tmpfs/building.txt bench/data/2015-01-23_fed5eab/dependent-chain/bilge\ -j4/ext4/rebuilding.txt bench/data/2015-01-23_fed5eab/dependent-chain/bilge\ -j4/tmpfs/rebuilding.txt bench/data/2015-01-23_fed5eab/dependent-chain/make\ -j4/ext4/rebuilding.txt bench/data/2015-01-23_fed5eab/dependent-chain/make\ -j4/tmpfs/rebuilding.txt bench/data/2015-01-23_fed5eab/dependent-chain/scons\ -j4/ext4/rebuilding.txt bench/data/2015-01-23_fed5eab/dependent-chain/scons\ -j4/tmpfs/rebuilding.txt bench/data/2015-01-23_fed5eab/dependent-chain/tup\ -j4/ext4/rebuilding.txt bench/data/2015-01-23_fed5eab/dependent-chain/tup\ -j4/tmpfs/rebuilding.txt bench/data/2015-01-23_fed5eab/dependent-chain/bilge\ -j4/ext4/touching-header.txt bench/data/2015-01-23_fed5eab/dependent-chain/bilge\ -j4/tmpfs/touching-header.txt bench/data/2015-01-23_fed5eab/dependent-chain/make\ -j4/ext4/touching-header.txt bench/data/2015-01-23_fed5eab/dependent-chain/make\ -j4/tmpfs/touching-header.txt bench/data/2015-01-23_fed5eab/dependent-chain/scons\ -j4/ext4/touching-header.txt bench/data/2015-01-23_fed5eab/dependent-chain/scons\ -j4/tmpfs/touching-header.txt bench/data/2015-01-23_fed5eab/dependent-chain/tup\ -j4/ext4/touching-header.txt bench/data/2015-01-23_fed5eab/dependent-chain/tup\ -j4/tmpfs/touching-header.txt bench/data/2015-01-23_fed5eab/dependent-chain/bilge\ -j4/ext4/touching-c.txt bench/data/2015-01-23_fed5eab/dependent-chain/bilge\ -j4/tmpfs/touching-c.txt bench/data/2015-01-23_fed5eab/dependent-chain/make\ -j4/ext4/touching-c.txt bench/data/2015-01-23_fed5eab/dependent-chain/make\ -j4/tmpfs/touching-c.txt bench/data/2015-01-23_fed5eab/dependent-chain/scons\ -j4/ext4/touching-c.txt bench/data/2015-01-23_fed5eab/dependent-chain/scons\ -j4/tmpfs/touching-c.txt bench/data/2015-01-23_fed5eab/dependent-chain/tup\ -j4/ext4/touching-c.txt bench/data/2015-01-23_fed5eab/dependent-chain/tup\ -j4/tmpfs/touching-c.txt bench/data/2015-01-23_fed5eab/dependent-chain/bilge\ -j4/ext4/doing-nothing.txt bench/data/2015-01-23_fed5eab/dependent-chain/bilge\ -j4/tmpfs/doing-nothing.txt bench/data/2015-01-23_fed5eab/dependent-chain/make\ -j4/ext4/doing-nothing.txt bench/data/2015-01-23_fed5eab/dependent-chain/make\ -j4/tmpfs/doing-nothing.txt bench/data/2015-01-23_fed5eab/dependent-chain/scons\ -j4/ext4/doing-nothing.txt bench/data/2015-01-23_fed5eab/dependent-chain/scons\ -j4/tmpfs/doing-nothing.txt bench/data/2015-01-23_fed5eab/dependent-chain/tup\ -j4/ext4/doing-nothing.txt bench/data/2015-01-23_fed5eab/dependent-chain/tup\ -j4/tmpfs/doing-nothing.txt bench/data/2015-01-23_fed5eab/cat/bilge\ -j4/ext4/building.txt bench/data/2015-01-23_fed5eab/cat/bilge\ -j4/tmpfs/building.txt bench/data/2015-01-23_fed5eab/cat/make\ -j4/ext4/building.txt bench/data/2015-01-23_fed5eab/cat/make\ -j4/tmpfs/building.txt bench/data/2015-01-23_fed5eab/cat/scons\ -j4/ext4/building.txt bench/data/2015-01-23_fed5eab/cat/scons\ -j4/tmpfs/building.txt bench/data/2015-01-23_fed5eab/cat/tup\ -j4/ext4/building.txt bench/data/2015-01-23_fed5eab/cat/tup\ -j4/tmpfs/building.txt bench/data/2015-01-23_fed5eab/cat/bilge\ -j4/ext4/rebuilding.txt bench/data/2015-01-23_fed5eab/cat/bilge\ -j4/tmpfs/rebuilding.txt bench/data/2015-01-23_fed5eab/cat/make\ -j4/ext4/rebuilding.txt bench/data/2015-01-23_fed5eab/cat/make\ -j4/tmpfs/rebuilding.txt bench/data/2015-01-23_fed5eab/cat/scons\ -j4/ext4/rebuilding.txt bench/data/2015-01-23_fed5eab/cat/scons\ -j4/tmpfs/rebuilding.txt bench/data/2015-01-23_fed5eab/cat/tup\ -j4/ext4/rebuilding.txt bench/data/2015-01-23_fed5eab/cat/tup\ -j4/tmpfs/rebuilding.txt bench/data/2015-01-23_fed5eab/cat/bilge\ -j4/ext4/doing-nothing.txt bench/data/2015-01-23_fed5eab/cat/bilge\ -j4/tmpfs/doing-nothing.txt bench/data/2015-01-23_fed5eab/cat/make\ -j4/ext4/doing-nothing.txt bench/data/2015-01-23_fed5eab/cat/make\ -j4/tmpfs/doing-nothing.txt bench/data/2015-01-23_fed5eab/cat/scons\ -j4/ext4/doing-nothing.txt bench/data/2015-01-23_fed5eab/cat/scons\ -j4/tmpfs/doing-nothing.txt bench/data/2015-01-23_fed5eab/cat/tup\ -j4/ext4/doing-nothing.txt bench/data/2015-01-23_fed5eab/cat/tup\ -j4/tmpfs/doing-nothing.txt bench/data/2015-01-23_fed5eab/hierarchy/bilge\ -j4/ext4/building.txt bench/data/2015-01-23_fed5eab/hierarchy/bilge\ -j4/tmpfs/building.txt bench/data/2015-01-23_fed5eab/hierarchy/make\ -j4/ext4/building.txt bench/data/2015-01-23_fed5eab/hierarchy/make\ -j4/tmpfs/building.txt bench/data/2015-01-23_fed5eab/hierarchy/scons\ -j4/ext4/building.txt bench/data/2015-01-23_fed5eab/hierarchy/scons\ -j4/tmpfs/building.txt bench/data/2015-01-23_fed5eab/hierarchy/tup\ -j4/ext4/building.txt bench/data/2015-01-23_fed5eab/hierarchy/tup\ -j4/tmpfs/building.txt bench/data/2015-01-23_fed5eab/hierarchy/bilge\ -j4/ext4/rebuilding.txt bench/data/2015-01-23_fed5eab/hierarchy/bilge\ -j4/tmpfs/rebuilding.txt bench/data/2015-01-23_fed5eab/hierarchy/make\ -j4/ext4/rebuilding.txt bench/data/2015-01-23_fed5eab/hierarchy/make\ -j4/tmpfs/rebuilding.txt bench/data/2015-01-23_fed5eab/hierarchy/scons\ -j4/ext4/rebuilding.txt bench/data/2015-01-23_fed5eab/hierarchy/scons\ -j4/tmpfs/rebuilding.txt bench/data/2015-01-23_fed5eab/hierarchy/tup\ -j4/ext4/rebuilding.txt bench/data/2015-01-23_fed5eab/hierarchy/tup\ -j4/tmpfs/rebuilding.txt bench/data/2015-01-23_fed5eab/hierarchy/bilge\ -j4/ext4/touching-header.txt bench/data/2015-01-23_fed5eab/hierarchy/bilge\ -j4/tmpfs/touching-header.txt bench/data/2015-01-23_fed5eab/hierarchy/make\ -j4/ext4/touching-header.txt bench/data/2015-01-23_fed5eab/hierarchy/make\ -j4/tmpfs/touching-header.txt bench/data/2015-01-23_fed5eab/hierarchy/scons\ -j4/ext4/touching-header.txt bench/data/2015-01-23_fed5eab/hierarchy/scons\ -j4/tmpfs/touching-header.txt bench/data/2015-01-23_fed5eab/hierarchy/tup\ -j4/ext4/touching-header.txt bench/data/2015-01-23_fed5eab/hierarchy/tup\ -j4/tmpfs/touching-header.txt bench/data/2015-01-23_fed5eab/hierarchy/bilge\ -j4/ext4/touching-c.txt bench/data/2015-01-23_fed5eab/hierarchy/bilge\ -j4/tmpfs/touching-c.txt bench/data/2015-01-23_fed5eab/hierarchy/make\ -j4/ext4/touching-c.txt bench/data/2015-01-23_fed5eab/hierarchy/make\ -j4/tmpfs/touching-c.txt bench/data/2015-01-23_fed5eab/hierarchy/scons\ -j4/ext4/touching-c.txt bench/data/2015-01-23_fed5eab/hierarchy/scons\ -j4/tmpfs/touching-c.txt bench/data/2015-01-23_fed5eab/hierarchy/tup\ -j4/ext4/touching-c.txt bench/data/2015-01-23_fed5eab/hierarchy/tup\ -j4/tmpfs/touching-c.txt bench/data/2015-01-23_fed5eab/hierarchy/bilge\ -j4/ext4/doing-nothing.txt bench/data/2015-01-23_fed5eab/hierarchy/bilge\ -j4/tmpfs/doing-nothing.txt bench/data/2015-01-23_fed5eab/hierarchy/make\ -j4/ext4/doing-nothing.txt bench/data/2015-01-23_fed5eab/hierarchy/make\ -j4/tmpfs/doing-nothing.txt bench/data/2015-01-23_fed5eab/hierarchy/scons\ -j4/ext4/doing-nothing.txt bench/data/2015-01-23_fed5eab/hierarchy/scons\ -j4/tmpfs/doing-nothing.txt bench/data/2015-01-23_fed5eab/hierarchy/tup\ -j4/ext4/doing-nothing.txt bench/data/2015-01-23_fed5eab/hierarchy/tup\ -j4/tmpfs/doing-nothing.txt bench/data bench/data/2015-01-23_fed5eab/dependent-chain bench/data/2015-01-23_fed5eab/dependent-chain/bilge\ -j4 bench/data/2015-01-23_fed5eab/dependent-chain/make\ -j4 bench/data/2015-01-23_fed5eab/dependent-chain/scons\ -j4 bench/data/2015-01-23_fed5eab/dependent-chain/tup\ -j4 bench/data/2015-01-23_fed5eab/cat bench/data/2015-01-23_fed5eab/cat/bilge\ -j4 bench/data/2015-01-23_fed5eab/cat/make\ -j4 bench/data/2015-01-23_fed5eab/cat/scons\ -j4 bench/data/2015-01-23_fed5eab/cat/tup\ -j4 bench/data/2015-01-23_fed5eab/hierarchy bench/data/2015-01-23_fed5eab/hierarchy/bilge\ -j4 bench/data/2015-01-23_fed5eab/hierarchy/make\ -j4 bench/data/2015-01-23_fed5eab/hierarchy/scons\ -j4 bench/data/2015-01-23_fed5eab/hierarchy/tup\ -j4
	python2 bench/generate-plots.py

web/style.css : web/style.scss web/normalize.scss
	sass web/style.scss web/style.css

web/index.html temp.html web/documentation.html web/benchmarks.html : web/mkdown.py web/index.md web/sidebar.md web/template.html web/documentation.md web/benchmarks.md
	python web/mkdown.py

.bilge : top.py
	python2 top.py > .bilge


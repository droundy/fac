all: tests/iterable_hash_test-32.test tests/spinner-32.test tests/trie-32.test tests/listset-32.test tests/arrayset-32.test tests/iterable_hash_test.test tests/spinner.test tests/trie.test tests/listset.test tests/arrayset.test bilge git-32.o new-build-32.o clean-32.o targets-32.o files-32.o bilge-32.o web/benchmarks.html web/documentation.html temp.html web/style.css web/index.html .bilge lib/fileaccesses

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

bilge.o : bilge.c bilge.h lib/trie.h lib/listset.h lib/iterablehash.h new-build.h
	gcc -Wall -Werror -O2 -std=c11 -g -c bilge.c

files.o : files.c bilge.h lib/trie.h lib/listset.h lib/iterablehash.h
	gcc -Wall -Werror -O2 -std=c11 -g -c files.c

targets.o : targets.c bilge.h lib/trie.h lib/listset.h lib/iterablehash.h
	gcc -Wall -Werror -O2 -std=c11 -g -c targets.c

clean.o : clean.c bilge.h lib/trie.h lib/listset.h lib/iterablehash.h
	gcc -Wall -Werror -O2 -std=c11 -g -c clean.c

new-build.o : new-build.c bilge.h lib/trie.h lib/listset.h lib/iterablehash.h new-build.h lib/bigbrother.h lib/arrayset.h
	gcc -Wall -Werror -O2 -std=c11 -g -c new-build.c

git.o : git.c bilge.h lib/trie.h lib/listset.h lib/iterablehash.h
	gcc -Wall -Werror -O2 -std=c11 -g -c git.c

bilge : bilge.o files.o targets.o clean.o new-build.o git.o lib/trie.o lib/listset.o lib/iterablehash.o lib/arrayset.o lib/bigbrother.o
	gcc -lpopt -lprofiler -o bilge bilge.o files.o targets.o clean.o new-build.o git.o lib/trie.o lib/listset.o lib/iterablehash.o lib/arrayset.o lib/bigbrother.o

tests/trie.test : tests/trie.o lib/trie.o lib/listset.o lib/iterablehash.o lib/arrayset.o lib/bigbrother.o
	gcc -lpthread -o tests/trie.test tests/trie.o lib/trie.o lib/listset.o lib/iterablehash.o lib/arrayset.o lib/bigbrother.o

tests/arrayset-32.test : tests/arrayset-32.o lib/trie-32.o lib/listset-32.o lib/iterablehash-32.o lib/arrayset-32.o
	gcc -m32 -lpthread -o tests/arrayset-32.test tests/arrayset-32.o lib/trie-32.o lib/listset-32.o lib/iterablehash-32.o lib/arrayset-32.o

tests/spinner-32.test : tests/spinner-32.o lib/trie-32.o lib/listset-32.o lib/iterablehash-32.o lib/arrayset-32.o
	gcc -m32 -lpthread -o tests/spinner-32.test tests/spinner-32.o lib/trie-32.o lib/listset-32.o lib/iterablehash-32.o lib/arrayset-32.o

git-32.o : git.c bilge.h lib/trie.h lib/listset.h lib/iterablehash.h
	gcc -m32 -Wall -Werror -O2 -std=c11 -g -o git-32.o -c git.c

new-build-32.o : new-build.c bilge.h lib/trie.h lib/listset.h lib/iterablehash.h new-build.h lib/bigbrother.h lib/arrayset.h
	gcc -m32 -Wall -Werror -O2 -std=c11 -g -o new-build-32.o -c new-build.c

clean-32.o : clean.c bilge.h lib/trie.h lib/listset.h lib/iterablehash.h
	gcc -m32 -Wall -Werror -O2 -std=c11 -g -o clean-32.o -c clean.c

targets-32.o : targets.c bilge.h lib/trie.h lib/listset.h lib/iterablehash.h
	gcc -m32 -Wall -Werror -O2 -std=c11 -g -o targets-32.o -c targets.c

files-32.o : files.c bilge.h lib/trie.h lib/listset.h lib/iterablehash.h
	gcc -m32 -Wall -Werror -O2 -std=c11 -g -o files-32.o -c files.c

bilge-32.o : bilge.c bilge.h lib/trie.h lib/listset.h lib/iterablehash.h new-build.h
	gcc -m32 -Wall -Werror -O2 -std=c11 -g -o bilge-32.o -c bilge.c

web/style.css : web/style.scss web/normalize.scss
	sass web/style.scss web/style.css

web/index.html temp.html web/documentation.html web/benchmarks.html : web/mkdown.py web/index.md web/sidebar.md web/template.html web/documentation.md web/benchmarks.md
	python web/mkdown.py

.bilge : top.py
	python2 top.py > .bilge


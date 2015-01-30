#!/bin/sh

cd tests && gcc -m32 -Wall -Werror -O2 -std=c11 -g -o iterable_hash_test-32.o -c iterable_hash_test.c

cd lib && gcc -m32 -Wall -Werror -O2 -std=c11 -g -o trie-32.o -c trie.c

cd lib && gcc -m32 -Wall -Werror -O2 -std=c11 -g -o listset-32.o -c listset.c

cd lib && gcc -m32 -Wall -Werror -O2 -std=c11 -g -o iterablehash-32.o -c iterablehash.c

cd lib && gcc -m32 -Wall -Werror -O2 -std=c11 -g -o arrayset-32.o -c arrayset.c

gcc -m32 -lpthread -o tests/iterable_hash_test-32.test tests/iterable_hash_test-32.o lib/trie-32.o lib/listset-32.o lib/iterablehash-32.o lib/arrayset-32.o

cd tests && gcc -m32 -Wall -Werror -O2 -std=c11 -g -o spinner-32.o -c spinner.c

cd tests && gcc -m32 -Wall -Werror -O2 -std=c11 -g -o trie-32.o -c trie.c

gcc -m32 -lpthread -o tests/trie-32.test tests/trie-32.o lib/trie-32.o lib/listset-32.o lib/iterablehash-32.o lib/arrayset-32.o

cd tests && gcc -m32 -Wall -Werror -O2 -std=c11 -g -o listset-32.o -c listset.c

gcc -m32 -lpthread -o tests/listset-32.test tests/listset-32.o lib/trie-32.o lib/listset-32.o lib/iterablehash-32.o lib/arrayset-32.o

cd tests && gcc -m32 -Wall -Werror -O2 -std=c11 -g -o arrayset-32.o -c arrayset.c

cd tests && gcc -Wall -Werror -O2 -std=c11 -g -c iterable_hash_test.c

cd lib && gcc -Wall -Werror -O2 -std=c11 -g -c trie.c

cd lib && gcc -Wall -Werror -O2 -std=c11 -g -c listset.c

cd lib && gcc -Wall -Werror -O2 -std=c11 -g -c iterablehash.c

cd lib && gcc -Wall -Werror -O2 -std=c11 -g -c arrayset.c

python lib/get_syscalls.py /usr/src/linux-headers-3.2.0-4-common > lib/syscalls.h

cd lib && gcc -Wall -Werror -O2 -std=c11 -g -c bigbrother.c

gcc -lpthread -o tests/iterable_hash_test.test tests/iterable_hash_test.o lib/trie.o lib/listset.o lib/iterablehash.o lib/arrayset.o lib/bigbrother.o

cd tests && gcc -Wall -Werror -O2 -std=c11 -g -c spinner.c

gcc -lpthread -o tests/spinner.test tests/spinner.o lib/trie.o lib/listset.o lib/iterablehash.o lib/arrayset.o lib/bigbrother.o

cd tests && gcc -Wall -Werror -O2 -std=c11 -g -c trie.c

cd tests && gcc -Wall -Werror -O2 -std=c11 -g -c listset.c

gcc -lpthread -o tests/listset.test tests/listset.o lib/trie.o lib/listset.o lib/iterablehash.o lib/arrayset.o lib/bigbrother.o

cd tests && gcc -Wall -Werror -O2 -std=c11 -g -c arrayset.c

gcc -lpthread -o tests/arrayset.test tests/arrayset.o lib/trie.o lib/listset.o lib/iterablehash.o lib/arrayset.o lib/bigbrother.o

cd lib && gcc -Wall -Werror -O2 -std=c11 -g -c fileaccesses.c

cd lib && gcc -lpopt -lprofiler -o fileaccesses fileaccesses.o trie.o listset.o iterablehash.o arrayset.o bigbrother.o

gcc -Wall -Werror -O2 -std=c11 -g -c loon.c

gcc -Wall -Werror -O2 -std=c11 -g -c files.c

gcc -Wall -Werror -O2 -std=c11 -g -c targets.c

gcc -Wall -Werror -O2 -std=c11 -g -c clean.c

gcc -Wall -Werror -O2 -std=c11 -g -c new-build.c

gcc -Wall -Werror -O2 -std=c11 -g -c git.c

gcc -lpopt -lprofiler -o loon loon.o files.o targets.o clean.o new-build.o git.o lib/trie.o lib/listset.o lib/iterablehash.o lib/arrayset.o lib/bigbrother.o

gcc -lpthread -o tests/trie.test tests/trie.o lib/trie.o lib/listset.o lib/iterablehash.o lib/arrayset.o lib/bigbrother.o

gcc -m32 -lpthread -o tests/arrayset-32.test tests/arrayset-32.o lib/trie-32.o lib/listset-32.o lib/iterablehash-32.o lib/arrayset-32.o

gcc -m32 -lpthread -o tests/spinner-32.test tests/spinner-32.o lib/trie-32.o lib/listset-32.o lib/iterablehash-32.o lib/arrayset-32.o

gcc -m32 -Wall -Werror -O2 -std=c11 -g -o git-32.o -c git.c

gcc -m32 -Wall -Werror -O2 -std=c11 -g -o new-build-32.o -c new-build.c

gcc -m32 -Wall -Werror -O2 -std=c11 -g -o clean-32.o -c clean.c

gcc -m32 -Wall -Werror -O2 -std=c11 -g -o targets-32.o -c targets.c

gcc -m32 -Wall -Werror -O2 -std=c11 -g -o files-32.o -c files.c

gcc -m32 -Wall -Werror -O2 -std=c11 -g -o loon-32.o -c loon.c

cp loon bilge

sass web/style.scss web/style.css

python web/mkdown.py

python2 top.py > .bilge


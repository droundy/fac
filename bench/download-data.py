import urllib.request, tarfile

response = urllib.request.urlopen('http://physics.oregonstate.edu/~roundyd/fac-bench.tar.bz2')
tarfile.open(fileobj = response, mode = 'r|*').extractall()

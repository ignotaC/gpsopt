
git clone 'https://github.com/ignotaC/smallsoft.git'

cd smallsoft
git pull
make
cp -v bin/readNMEA ./..
cp -v bin/fixedxargs ./..

cd ..

cc -Wall -Wextra -pedantic -lm -DNDEBUG  mapgps.c -o mapgps

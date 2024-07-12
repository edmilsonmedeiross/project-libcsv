#!/bin/sh

# Atualiza o apt e instala o GCC e dependências necessárias
sudo apt update
sudo apt install -y gcc libcunit1-dev make

# Cria o diretório de build se não existir
mkdir -p ./build/Debug

# Compila os arquivos fontes para criar a biblioteca compartilhada
gcc -Wall -fPIC -c libcsv.c -o ./build/Debug/libcsv.o
gcc -shared -o ./build/Debug/libcsv.so ./build/Debug/libcsv.o

# Compila os testes
gcc -Wall -Wextra -Wpedantic -Wshadow -Wformat=2 -Wcast-align -Wconversion -Wsign-conversion -Wnull-dereference -g3 -O0 -c test_libcsv_all.c -o ./build/Debug/test_libcsv_all.o
gcc -Wall -Wextra -Wpedantic -Wshadow -Wformat=2 -Wcast-align -Wconversion -Wsign-conversion -Wnull-dereference -g3 -O0 ./build/Debug/libcsv.o ./build/Debug/test_libcsv_all.o -o ./build/Debug/test_libcsv_all -lcunit -lpthread

# Opcional: Copia a biblioteca compartilhada para /usr/local/lib
sudo cp ./build/Debug/libcsv.so /usr/local/lib/

# Opcional: Copia o arquivo de cabeçalho para /usr/local/include
sudo cp libcsv.h /usr/local/include/

# Atualiza o cache das bibliotecas compartilhadas
sudo ldconfig

# Configura a variável de ambiente LD_LIBRARY_PATH para incluir /usr/local/lib
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
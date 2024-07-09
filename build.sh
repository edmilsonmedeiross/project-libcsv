#!/bin/sh

# Atualiza o apk e instala o GCC e dependências necessárias
apk update
apk add --no-cache gcc musl-dev

# Cria o diretório de build se não existir
mkdir -p ./build/Debug

# Compila os arquivos fontes para criar a biblioteca compartilhada
gcc -Wall -fPIC -c csv.processor.c -o ./build/Debug/csv.processor.o
gcc -shared -o ./build/Debug/libcsv.so ./build/Debug/csv.processor.o

# Copia a biblioteca compartilhada para /usr/lib
cp ./build/Debug/libcsv.so /usr/lib/

# Configura a variável de ambiente LD_LIBRARY_PATH para incluir /usr/lib
export LD_LIBRARY_PATH=/usr/lib


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "libcsv.h"

// Mutex para garantir a segurança entre threads
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Função auxiliar para dividir uma string em partes com base em um delimitador
char **splitString(const char *str, const char *delimiter, int *count) {
    char *copy = strdup(str);
    if (!copy) return NULL;

    size_t capacity = 10;
    char **result = (char **)malloc((size_t)capacity * (size_t)sizeof(char *));
    if (!result) {
        free(copy);
        return NULL;
    }

    int idx = 0;
    char *token = strtok(copy, delimiter);
    while (token) {
        if ((size_t)idx >= capacity) {
            capacity *= 2;
            result = (char **)realloc(result, (size_t)capacity * sizeof(char *));
            if (!result) {
                free(copy);
                return NULL;
            }
        }
        result[idx++] = strdup(token);
        token = strtok(NULL, delimiter);
    }

    *count = idx;
    free(copy);
    return result;
}

// Função para liberar a memória alocada por splitString
void freeSplitString(char **split, int count) {
    for (int i = 0; i < count; i++) {
        free(split[i]);
    }
    free(split);
}

// Função auxiliar para verificar se uma linha atende aos filtros
int rowMatchesFilters(char **headers, char **row, char **filters, int filterCount) {
    for (int i = 0; i < filterCount; i++) {
        char *filter = strdup(filters[i]);
        if (!filter) return 0;

        char *header = strtok(filter, "><=");
        char *value = strtok(NULL, "><=");
        char op = filters[i][strlen(header)];

        int columnIndex = -1;
        for (int j = 0; headers[j] != NULL; j++) {
            if (strcmp(headers[j], header) == 0) {
                columnIndex = j;
                break;
            }
        }
        if (columnIndex == -1) {
            free(filter);
            continue;
        }

        int rowValue = atoi(row[columnIndex]);
        int filterValue = atoi(value);

        int match = 0;
        switch (op) {
            case '>':
                match = rowValue > filterValue;
                break;
            case '<':
                match = rowValue < filterValue;
                break;
            case '=':
                match = rowValue == filterValue;
                break;
        }

        free(filter);
        if (!match) return 0;
    }
    return 1;
}

void processCsv(const char csv[], const char selectedColumns[], const char rowFilterDefinitions[]) {
    pthread_mutex_lock(&mutex);

    int headerCount, rowCount, selectedCount, filterCount;
    char **rows = splitString(csv, "\n", &rowCount);
    if (!rows) {
        pthread_mutex_unlock(&mutex);
        return;
    }

    char **headers = splitString(rows[0], ",", &headerCount);
    char **selectedCols = splitString(selectedColumns, ",", &selectedCount);
    char **filters = splitString(rowFilterDefinitions, "\n", &filterCount);

    if (!headers || !selectedCols || !filters) {
        freeSplitString(rows, rowCount);
        if (headers) freeSplitString(headers, headerCount);
        if (selectedCols) freeSplitString(selectedCols, selectedCount);
        if (filters) freeSplitString(filters, filterCount);
        pthread_mutex_unlock(&mutex);
        return;
    }

    printf("%s\n", selectedColumns); // Imprime os cabeçalhos selecionados

    for (int i = 1; i < rowCount; i++) {
        char **row = splitString(rows[i], ",", &headerCount);
        if (!rowMatchesFilters(headers, row, filters, filterCount)) {
            freeSplitString(row, headerCount);
            continue;
        }

        for (int j = 0; j < selectedCount; j++) {
            for (int k = 0; k < headerCount; k++) {
                if (strcmp(selectedCols[j], headers[k]) == 0) {
                    printf("%s", row[k]);
                    if (j < selectedCount - 1) printf(",");
                    break;
                }
            }
        }
        printf("\n");
        freeSplitString(row, headerCount);
    }

    freeSplitString(rows, rowCount);
    freeSplitString(headers, headerCount);
    freeSplitString(selectedCols, selectedCount);
    freeSplitString(filters, filterCount);
    pthread_mutex_unlock(&mutex);
}

void processCsvFile(const char csvFilePath[], const char selectedColumns[], const char rowFilterDefinitions[]) {
    FILE *file = fopen(csvFilePath, "r");
    if (!file) {
        perror("Unable to open file");
        return;
    }

    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *fileContent = (char *)malloc((size_t)fileSize + 1);
    if (!fileContent) {
        fclose(file);
        return;
    }

    fread(fileContent, 1, (size_t)fileSize, file);
    fileContent[fileSize] = '\0';
    fclose(file);

    processCsv(fileContent, selectedColumns, rowFilterDefinitions);

    free(fileContent);
}

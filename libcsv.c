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
    char **result = (char **)malloc(capacity * sizeof(char *));
    if (!result) {
        free(copy);
        return NULL;
    }

    int idx = 0;
    char *token = strtok(copy, delimiter);
    while (token) {
        if ((size_t)idx >= capacity) {
            capacity *= 2;
            char **new_result = (char **)realloc(result, capacity * sizeof(char *));
            if (!new_result) {
                for (int i = 0; i < idx; i++) {
                    free(result[i]);
                }
                free(result);
                free(copy);
                return NULL;
            }
            result = new_result;
        }
        result[idx] = strdup(token);
        if (!result[idx]) {
            for (int i = 0; i <= idx; i++) {
                free(result[i]);
            }
            free(result);
            free(copy);
            return NULL;
        }
        idx++;
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

// Função auxiliar para verificar se uma coluna é válida
int isValidColumn(char **headers, int headerCount, const char *column) {
    for (int i = 0; i < headerCount; i++) {
        if (strcmp(headers[i], column) == 0) {
            return 1;
        }
    }
    return 0;
}

// Função auxiliar para verificar se um filtro é válido
int isValidFilter(char **headers, int headerCount, const char *filter, char **appliedFilters, int *appliedCount) {
    char *filterCopy = strdup(filter);
    if (!filterCopy) return 0;

    char *column = strtok(filterCopy, "><!=<=>=");
    char *value = strtok(NULL, "><!=<=>=");

    if (!column || !value || (filter[strlen(column)] != '>' && filter[strlen(column)] != '<' && filter[strlen(column)] != '=' && filter[strlen(column)] != '!')) {
        free(filterCopy);
        return 0;
    }

    int valid = isValidColumn(headers, headerCount, column);

    // Verifica se já existe um filtro aplicado para essa coluna
    for (int i = 0; i < *appliedCount; i++) {
        if (strcmp(appliedFilters[i], column) == 0) {
            valid = 0;
            break;
        }
    }

    if (valid) {
        appliedFilters[*appliedCount] = strdup(column);
        (*appliedCount)++;
    }

    free(filterCopy);
    return valid;
}

// Função auxiliar para verificar se uma linha atende aos filtros
int rowMatchesFilters(char **headers, char **row, char **filters, int filterCount) {
    int overallMatch = 1;

    // Arrays para armazenar filtros por header
    char *headerFilters[256][256] = {{NULL}};
    int filterCounts[256] = {0};

    // Distribui os filtros por header
    for (int i = 0; i < filterCount; i++) {
        char *filter = strdup(filters[i]);
        char *column = strtok(filter, "><!=<=>=");
        int columnIndex = -1;

        for (int j = 0; headers[j] != NULL; j++) {
            if (strcmp(headers[j], column) == 0) {
                columnIndex = j;
                break;
            }
        }

        if (columnIndex != -1) {
            headerFilters[columnIndex][filterCounts[columnIndex]++] = strdup(filters[i]);
        }

        free(filter);
    }

    // Verifica cada header
    for (int i = 0; headers[i] != NULL; i++) {
        if (filterCounts[i] == 0) continue;  // Sem filtros para este header

        int headerMatch = 0;
        for (int j = 0; j < filterCounts[i]; j++) {
            char *filter = strdup(headerFilters[i][j]);
            char *column = strtok(filter, "><!=<=>=");
            char *value = strtok(NULL, "><!=<=>=");
            char op1 = headerFilters[i][j][strlen(column)];
            char op2 = headerFilters[i][j][strlen(column) + 1];

            int columnIndex = i;
            char *rowValue = row[columnIndex];
            int match = 0;

            switch (op1) {
                case '>':
                    match = (op2 == '=') ? (strcmp(rowValue, value) >= 0) : (strcmp(rowValue, value) > 0);
                    break;
                case '<':
                    match = (op2 == '=') ? (strcmp(rowValue, value) <= 0) : (strcmp(rowValue, value) < 0);
                    break;
                case '=':
                    match = (strcmp(rowValue, value) == 0);
                    break;
                case '!':
                    match = (strcmp(rowValue, value) != 0);
                    break;
                default:
                    match = 0;
                    break;
            }

            free(filter);

            if (match) {
                headerMatch = 1; // Se qualquer filtro no mesmo header corresponder, o header é considerado correspondido
                break;
            }
        }

        if (!headerMatch) {
            overallMatch = 0;
            break;
        }
    }

    // Limpeza
    for (int i = 0; i < 256; i++) {
        for (int j = 0; j < filterCounts[i]; j++) {
            free(headerFilters[i][j]);
        }
    }

    return overallMatch;
}


// Função para processar o CSV
void processCsv(const char csv[], const char selectedColumns[], const char rowFilterDefinitions[]) {
    pthread_mutex_lock(&mutex);

    int headerCount, rowCount, selectedCount, filterCount;
    char **rows = splitString(csv, "\n", &rowCount);
    if (!rows) {
        pthread_mutex_unlock(&mutex);
        return;
    }

    char **headers = splitString(rows[0], ",", &headerCount);

    // Verifica o número de colunas
    if (headerCount > 256) {
        freeSplitString(rows, rowCount);
        freeSplitString(headers, headerCount);
        pthread_mutex_unlock(&mutex);
        return;
    }

    // Verifica se selectedColumns está vazio
    char **selectedCols;
    if (strlen(selectedColumns) == 0) {
        selectedCols = headers; // Se estiver vazio, seleciona todas as colunas
        selectedCount = headerCount;
    } else {
        selectedCols = splitString(selectedColumns, ",", &selectedCount);
        for (int i = 0; i < selectedCount; i++) {
            if (!isValidColumn(headers, headerCount, selectedCols[i])) {
                freeSplitString(rows, rowCount);
                freeSplitString(headers, headerCount);
                freeSplitString(selectedCols, selectedCount);
                pthread_mutex_unlock(&mutex);
                return;
            }
        }
    }

    char **filters = splitString(rowFilterDefinitions, "\n", &filterCount);
    char *appliedFilters[256] = {0};
    int appliedCount = 0;
    for (int i = 0; i < filterCount; i++) {
        if (!isValidFilter(headers, headerCount, filters[i], appliedFilters, &appliedCount)) {
            freeSplitString(rows, rowCount);
            freeSplitString(headers, headerCount);
            if (selectedCols != headers) freeSplitString(selectedCols, selectedCount);
            freeSplitString(filters, filterCount);
            for (int j = 0; j < appliedCount; j++) {
                free(appliedFilters[j]);
            }
            pthread_mutex_unlock(&mutex);
            return;
        }
    }

    if (!headers || !selectedCols || !filters) {
        freeSplitString(rows, rowCount);
        if (headers) freeSplitString(headers, headerCount);
        if (selectedCols && selectedCols != headers) freeSplitString(selectedCols, selectedCount);
        if (filters) freeSplitString(filters, filterCount);
        pthread_mutex_unlock(&mutex);
        return;
    }

    // Imprime os cabeçalhos selecionados
    for (int j = 0; j < selectedCount; j++) {
        printf("%s", selectedCols[j]);
        if (j < selectedCount - 1) printf(",");
    }
    printf("\n");

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
    if (selectedCols != headers) freeSplitString(selectedCols, selectedCount);
    freeSplitString(filters, filterCount);
    for (int i = 0; i < appliedCount; i++) {
        free(appliedFilters[i]);
    }
    pthread_mutex_unlock(&mutex);
}

// Função para processar um arquivo CSV
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
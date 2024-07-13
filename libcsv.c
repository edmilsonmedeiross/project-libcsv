#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "libcsv.h"

// Mutex para garantir a segurança entre threads
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Define a estrutura Filter
typedef struct {
    int columnIndex;
    char operation[3];
    char *value;
} Filter;

// Declaração das funções auxiliares
int isValidColumn(char **headers, int headerCount, const char *column);
int isValidFilter(char **headers, int headerCount, const char *filter);
char **splitString(const char *str, const char *delimiter, int *count);
void freeSplitString(char **split, int count);
Filter *extractFilters(const char *filterStr, char **headers, int headerCount, int *filterCount);
void freeFilters(Filter *filters, int filterCount);
int rowMatchesFilters(char **headers, char **row, Filter *filters, int filterCount);

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
    fprintf(stderr, "Header '%s' not found in CSV file/string", column);
    return 0;
}



// Função auxiliar para verificar se um filtro é válido
int isValidFilter(char **headers, int headerCount, const char *filter) {
    char *filterCopy = strdup(filter);
    if (!filterCopy) {
        fprintf(stderr, "Memory allocation error\n");
        return 0;
    }

    char *column = strtok(filterCopy, "><!=<=>=");
    char *value = strtok(NULL, "><!=<=>=");
    int valid = column && value && (strchr(filter, '>') || strchr(filter, '<') || strchr(filter, '=') || strchr(filter, '!')) && isValidColumn(headers, headerCount, column);

    if (!valid) {
        fprintf(stderr, "Invalid filter: '%s'\n", filter);
    }

    free(filterCopy);
    return valid;
}

// Função auxiliar para extrair filtros de uma string de definição de filtro
Filter *extractFilters(const char *filterStr, char **headers, int headerCount, int *filterCount) {
    int count;
    char **filterStrings = splitString(filterStr, "\n", &count);
    Filter *filters = (Filter *)malloc(count * sizeof(Filter));

    for (int i = 0; i < count; i++) {
        if (!isValidFilter(headers, headerCount, filterStrings[i])) {
            free(filters);
            *filterCount = 0;
            freeSplitString(filterStrings, count);
            return NULL;
        }

        char *filter = strdup(filterStrings[i]);
        char *column = strtok(filter, "><=!");
        char *value = strtok(NULL, "><=!");
        char operation[3] = {0};

        if (strstr(filterStrings[i], ">=")) {
            strcpy(operation, ">=");
        } else if (strstr(filterStrings[i], "<=")) {
            strcpy(operation, "<=");
        } else {
            operation[0] = filterStrings[i][strlen(column)];
        }

        int columnIndex = -1;
        for (int j = 0; j < headerCount; j++) {
            if (strcmp(headers[j], column) == 0) {
                columnIndex = j;
                break;
            }
        }

        if (columnIndex == -1) {
            fprintf(stderr,"c");
            free(filters);
            *filterCount = 0;
            free(filter);
            freeSplitString(filterStrings, count);
            return NULL;
        }

        filters[i].columnIndex = columnIndex;
        strcpy(filters[i].operation, operation);
        filters[i].value = strdup(value);
        free(filter);
    }

    freeSplitString(filterStrings, count);
    *filterCount = count;
    return filters;
}

// Libera a memória alocada para filtros
void freeFilters(Filter *filters, int filterCount) {
    for (int i = 0; i < filterCount; i++) {
        free(filters[i].value);
    }
    free(filters);
}

// Função auxiliar para verificar se uma linha atende aos filtros
int rowMatchesFilters(char **headers, char **row, Filter *filters, int filterCount) {
    for (int i = 0; i < filterCount; i++) {
        int columnIndex = filters[i].columnIndex;
        char *rowValue = row[columnIndex];
        int match = 0;

        if (strcmp(filters[i].operation, ">=") == 0) {
            match = (strcmp(rowValue, filters[i].value) >= 0);
        } else if (strcmp(filters[i].operation, "<=") == 0) {
            match = (strcmp(rowValue, filters[i].value) <= 0);
        } else if (filters[i].operation[0] == '>') {
            match = (strcmp(rowValue, filters[i].value) > 0);
        } else if (filters[i].operation[0] == '<') {
            match = (strcmp(rowValue, filters[i].value) < 0);
        } else if (filters[i].operation[0] == '=') {
            match = (strcmp(rowValue, filters[i].value) == 0);
        } else if (filters[i].operation[0] == '!') {
            match = (strcmp(rowValue, filters[i].value) != 0);
        } else {
            fprintf(stderr, "Invalid filter: '%s%c%s'\n", headers[columnIndex], filters[i].operation[0], filters[i].value);
            return 0;
        }

        if (!match) {
            return 0;
        }
    }
    return 1;
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
    char **selectedCols;
    if (strlen(selectedColumns) == 0) {
        selectedCols = headers; // Se selectedColumns estiver vazio, seleciona todas as colunas
        selectedCount = headerCount;
    } else {
        selectedCols = splitString(selectedColumns, ",", &selectedCount);
    }
    Filter *filters = extractFilters(rowFilterDefinitions, headers, headerCount, &filterCount);

    if (!headers || !selectedCols || !filters) {
        freeSplitString(rows, rowCount);
        if (headers) freeSplitString(headers, headerCount);
        if (selectedCols && selectedCols != headers) freeSplitString(selectedCols, selectedCount);
        if (filters) freeFilters(filters, filterCount);
        pthread_mutex_unlock(&mutex);
        return;
    }

    // Verifica se todos os selectedCols existem nos headers
    for (int i = 0; i < selectedCount; i++) {
        if (!isValidColumn(headers, headerCount, selectedCols[i])) {
            freeSplitString(rows, rowCount);
            freeSplitString(headers, headerCount);
            if (selectedCols != headers) freeSplitString(selectedCols, selectedCount);
            freeFilters(filters, filterCount);
            pthread_mutex_unlock(&mutex);
            return;
        }
    }

    // Print headers in the order of CSV
    int printedHeaders = 0;
    for (int j = 0; j < headerCount; j++) {
        for (int k = 0; k < selectedCount; k++) {
            if (strcmp(headers[j], selectedCols[k]) == 0) {
                if (printedHeaders > 0) printf(",");
                printf("%s",headers[j]);
                printedHeaders++;
                break;
            }
        }
    }
    printf("\n");

    for (int i = 1; i < rowCount; i++) {
        char **row = splitString(rows[i], ",", &headerCount);
        if (!rowMatchesFilters(headers, row, filters, filterCount)) {
            freeSplitString(row, headerCount);
            continue;
        }

        // Print row values in the order of CSV
        int printedValues = 0;
        for (int j = 0; j < headerCount; j++) {
            for (int k = 0; k < selectedCount; k++) {
                if (strcmp(headers[j], selectedCols[k]) == 0) {
                    if (printedValues > 0) printf(",");
                    printf("%s",row[j]);
                    printedValues++;
                    break;
                }
            }
        }
        printf("\n");
        freeSplitString(row, headerCount);
    }

    freeSplitString(rows, rowCount);
    if (selectedCols != headers) freeSplitString(selectedCols, selectedCount);
    freeSplitString(headers, headerCount);
    freeFilters(filters, filterCount);
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
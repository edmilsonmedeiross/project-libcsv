#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

// Mutex para garantir a segurança entre threads
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Define a estrutura Filter
typedef struct {
    int columnIndex;
    char operation[3];
    char *value;
} Filter;

// Declaração das funções auxiliares
char **splitString(const char *str, const char *delimiter, int *count);
void freeSplitString(char **split, int count);
void validateHeadersAndFilters(const char *selectedColumns, const char *filterStr, char **headers, int headerCount, char *errorBuffer);
int isValidColumn(char **headers, int headerCount, const char *column, char *errorBuffer);
int isValidFilter(char **headers, int headerCount, const char *filter, char *errorBuffer);
Filter *extractFilters(const char *filterStr, char **headers, int headerCount, int *filterCount);
void freeFilters(Filter *filters, int filterCount);
int rowMatchesFilters(char **headers, char **row, Filter *filters, int filterCount,  int headerCount);


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
                for (int i = 0; i <= idx; i++) {
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
int isValidColumn(char **headers, int headerCount, const char *column, char *errorBuffer) {
    for (int i = 0; i < headerCount; i++) {
        if (strcmp(headers[i], column) == 0) {
            return 1;
        }
    }
    sprintf(errorBuffer + strlen(errorBuffer), "Header '%s' not found in CSV file/string\n", column);
    return 0;
}

// Função auxiliar para verificar se um filtro é válido
int isValidFilter(char **headers, int headerCount, const char *filter, char *errorBuffer) {
    char *filterCopy = strdup(filter);
    if (!filterCopy) {
        sprintf(errorBuffer + strlen(errorBuffer), "Memory allocation error\n");
        return 0;
    }

    // Encontrar a posição do operador
    char *operatorPos = strpbrk(filterCopy, "><!=");

    if (!operatorPos) {
        sprintf(errorBuffer + strlen(errorBuffer), "Invalid filter: '%s'\n", filter);
        free(filterCopy);
        return 0;
    }

    // Isolar o operador
    char operator[3] = {0};
    if (operatorPos[1] == '=') {
        strncpy(operator, operatorPos, 2);  // operadores como '>=', '<=', '!='
    } else {
        strncpy(operator, operatorPos, 1);  // operadores como '>', '<', '='
    }

    // Verifique se o operador é um dos válidos
    if (strcmp(operator, ">") != 0 && strcmp(operator, "<") != 0 && strcmp(operator, "=") != 0 &&
        strcmp(operator, "!=") != 0 && strcmp(operator, ">=") != 0 && strcmp(operator, "<=") != 0) {
        sprintf(errorBuffer + strlen(errorBuffer), "Invalid filter: '%s'\n", filter);
        free(filterCopy);
        return 0;
    }

    // Extrair a coluna
    *operatorPos = '\0';  // Terminar a string antes do operador
    char *column = filterCopy;

    // Verificar se a coluna existe nos cabeçalhos
    int headerExists = 0;
    for (int i = 0; i < headerCount; i++) {
        if (strcmp(headers[i], column) == 0) {
            headerExists = 1;
            break;
        }
    }

    if (!headerExists) {
        sprintf(errorBuffer + strlen(errorBuffer), "Header '%s' not found in CSV file/string\n", column);
    }

    free(filterCopy);
    return headerExists;
}

// Função auxiliar para validar cabeçalhos e filtros
void validateHeadersAndFilters(const char *selectedColumns, const char *filterStr, char **headers, int headerCount, char *errorBuffer) {
    int selectedCount, filterCount;
    char **selectedCols = splitString(selectedColumns, ",", &selectedCount);
    char **filterStrings = splitString(filterStr, "\n", &filterCount);

    for (int i = 0; i < selectedCount; i++) {
        if (!isValidColumn(headers, headerCount, selectedCols[i], errorBuffer)) {
            continue;
        }
    }

    for (int i = 0; i < filterCount; i++) {
        if (!isValidFilter(headers, headerCount, filterStrings[i], errorBuffer)) {
            continue;
        }
    }

    freeSplitString(selectedCols, selectedCount);
    freeSplitString(filterStrings, filterCount);
}

// Função auxiliar para extrair filtros de uma string de definição de filtro
Filter *extractFilters(const char *filterStr, char **headers, int headerCount, int *filterCount) {
    int count;
    char **filterStrings = splitString(filterStr, "\n", &count);
    Filter *filters = (Filter *)malloc(count * sizeof(Filter));

    for (int i = 0; i < count; i++) {
        char *filter = strdup(filterStrings[i]);
        char *column = strtok(filter, "><!=<>");
        char *value = strtok(NULL, "><!=<>");
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
int rowMatchesFilters(char **headers, char **row, Filter *filters, int filterCount, int headerCount) {
    // printf("Debugging rowMatchesFilters:\n");

    int *columnMatches = (int *)malloc(headerCount * sizeof(int));
    memset(columnMatches, 0, headerCount * sizeof(int));

    // Agrupar filtros por coluna
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
            free(columnMatches);
            return 0;
        }

        // Se um filtro corresponder, marque a coluna como correspondida
        if (match) {
            columnMatches[columnIndex] = 1;
        }
    }

    // Verifique se todas as colunas têm pelo menos um filtro correspondente
    for (int i = 0; i < filterCount; i++) {
        int columnIndex = filters[i].columnIndex;
        if (columnMatches[columnIndex] == 0) {
            free(columnMatches);
            return 0;
        }
    }

    free(columnMatches);
    return 1;
}

// função para processar string Csv
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
        selectedCols = headers;
        selectedCount = headerCount;
    } else {
        selectedCols = splitString(selectedColumns, ",", &selectedCount);
    }

    char errorBuffer[1024] = {0};
    validateHeadersAndFilters(selectedColumns, rowFilterDefinitions, headers, headerCount, errorBuffer);

    if (strlen(errorBuffer) > 0) {
        fprintf(stderr, "%s", errorBuffer);
        freeSplitString(rows, rowCount);
        freeSplitString(headers, headerCount);
        if (selectedCols != headers) freeSplitString(selectedCols, selectedCount);
        pthread_mutex_unlock(&mutex);
        return;
    }

    Filter *filters = extractFilters(rowFilterDefinitions, headers, headerCount, &filterCount);
    if (!filters) {
        freeSplitString(rows, rowCount);
        freeSplitString(headers, headerCount);
        if (selectedCols != headers) freeSplitString(selectedCols, selectedCount);
        pthread_mutex_unlock(&mutex);
        return;
    }

    // Imprime os headers selecionados na ordem do CSV
    int printedHeaders = 0;
    for (int j = 0; j < headerCount; j++) {
        for (int k = 0; k < selectedCount; k++) {
            if (strcmp(headers[j], selectedCols[k]) == 0) {
                if (printedHeaders > 0) printf(",");
                printf("%s", headers[j]);
                printedHeaders++;
                break;
            }
        }
    }
    printf("\n");

    for (int i = 1; i < rowCount; i++) {
        int rowColumnCount;
        char **row = splitString(rows[i], ",", &rowColumnCount);
        if (!rowMatchesFilters(headers, row, filters, filterCount, headerCount)) {
            freeSplitString(row, rowColumnCount);
            continue;
        }

        // Imprime os valores da linha na ordem dos headers, selecionados
        int printedValues = 0;
        for (int j = 0; j < headerCount; j++) {
            for (int k = 0; k < selectedCount; k++) {
                if (strcmp(headers[j], selectedCols[k]) == 0) {
                    if (printedValues > 0) printf(",");
                    printf("%s", row[j]);
                    printedValues++;
                    break;
                }
            }
        }
        printf("\n");
        freeSplitString(row, rowColumnCount);
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
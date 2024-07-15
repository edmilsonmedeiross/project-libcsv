#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <CUnit/Basic.h>
#include "libcsv.h"

#define TEMP_FILE "temp_stderr.txt"

// Função de setup
int init_suite(void) { return 0; }
// Função de teardown
int clean_suite(void) { return 0; }

// Função auxiliar para redirecionar stdout para um buffer
void redirect_stdout(char *buffer) {
    freopen("/dev/null", "a", stdout); // redireciona stdout para null
    setbuf(stdout, buffer); // redireciona o buffer stdout para buffer
}

// Função auxiliar para restaurar stdout
void restore_stdout(void) {
    freopen("/dev/tty", "a", stdout); // restaura stdout
}

// Função auxiliar para redirecionar stderr para um arquivo temporário
void redirect_stderr(const char *filename) {
    fflush(stderr);
    freopen(filename, "w", stderr);
    setvbuf(stderr, NULL, _IONBF, 0);
}

// Função auxiliar para restaurar stderr
void restore_stderr(int saved_stderr) {
    fflush(stderr);
    dup2(saved_stderr, fileno(stderr));
    close(saved_stderr);
}

// Teste básico para processCsv
void test_processCsv_basic(void) {
    const char csv[] = "header1,header2,header3\n1,2,3\n4,5,6\n7,8,9";
    char output[1024] = {0};
    redirect_stdout(output);
    processCsv(csv, "header1,header3", "header1>1\nheader3<9");
    restore_stdout();
    CU_ASSERT_STRING_EQUAL(output, "header1,header3\n4,6\n");
}

// Teste para colunas com aspas no nome
void test_processCsv_quoted_headers(void) {
    const char csv[] = "hea\"der1,header2,header3\n1,2,3\n4,5,6\n7,8,9";
    char output[1024] = {0};
    redirect_stdout(output);
    processCsv(csv, "hea\"der1,header3", "hea\"der1>1\nheader3<9");
    restore_stdout();
    CU_ASSERT_STRING_EQUAL(output, "hea\"der1,header3\n4,6\n");
}

// Teste para colunas em ordem arbitrária
void test_processCsv_arbitrary_columns(void) {
    const char csv[] = "header1,header2,header3\n1,2,3\n4,5,6\n7,8,9";
    char output[1024] = {0};
    redirect_stdout(output);
    processCsv(csv, "header3,header1", "header1>1\nheader3<9");
    restore_stdout();
    CU_ASSERT_STRING_EQUAL(output, "header1,header3\n4,6\n");
}

// Teste para filtros em ordem arbitrária
void test_processCsv_arbitrary_filters(void) {
    const char csv[] = "header1,header2,header3\n1,2,3\n4,5,6\n7,8,9";
    char output[1024] = {0};
    redirect_stdout(output);
    processCsv(csv, "header1,header3", "header3<9\nheader1>1");
    restore_stdout();
    CU_ASSERT_STRING_EQUAL(output, "header1,header3\n4,6\n");
}

// Teste para colunas inexistentes
void test_processCsv_nonexistent_columns(void) {
    const char csv[] = "header1,header2,header3\n1,2,3\n4,5,6\n7,8,9";
    char error_output[1024] = {0};
    int saved_stderr = dup(fileno(stderr));
    redirect_stderr(TEMP_FILE);

    processCsv(csv, "header4,header1", "header1>1\nheader3<9");

    restore_stderr(saved_stderr);
    FILE *file = fopen(TEMP_FILE, "r");
    fread(error_output, sizeof(char), sizeof(error_output) - 1, file);
    fclose(file);
    remove(TEMP_FILE);

    CU_ASSERT_STRING_EQUAL(error_output, "Header 'header4' not found in CSV file/string\n");
}

// Teste para colunas inexistentes e e cabeçalhos inexistentes no filtro
void test_processCsv_nonexistent_columns_and_headers_filters(void) {
    const char csv[] = "header1,header2,header3\n1,2,3\n4,5,6\n7,8,9";
    char error_output[1024] = {0};
    int saved_stderr = dup(fileno(stderr));
    redirect_stderr(TEMP_FILE);

    processCsv(csv, "header4,header1", "header5>1\nheader3<9");

    restore_stderr(saved_stderr);
    FILE *file = fopen(TEMP_FILE, "r");
    fread(error_output, sizeof(char), sizeof(error_output) - 1, file);
    fclose(file);
    remove(TEMP_FILE);

    CU_ASSERT_STRING_EQUAL(error_output, "Header 'header4' not found in CSV file/string\nHeader 'header5' not found in CSV file/string\n");
}

// Teste para multiplos filtros no mesmo header
void test_processCsv_multiple_filters(void) {
    const char csv[] = "header1,header2,header3\n1,2,3\n4,5,6\n7,8,9";
    char output[1024] = {0};
    redirect_stdout(output);
    processCsv(csv, "header1,header3", "header1>1\nheader1<7\nheader3>3\nheader3<9");
    restore_stdout();

    CU_ASSERT_STRING_EQUAL(output, "header1,header3\n1,3\n4,6\n7,9\n");
}

// teste processCsv multiplos filtros no mesmo header Readme
void test_processCsv_multiple_filters_readme(void) {
    const char csv[] = "header1,header2,header3\n1,2,3\n4,5,6\n7,8,9";
    char output[1024] = {0};
    redirect_stdout(output);
    processCsv(csv, "", "header1=1\nheader1=4\nheader2>3\nheader3>4");
    restore_stdout();

    CU_ASSERT_STRING_EQUAL(output, "header1,header2,header3\n4,5,6\n");
}

// Teste para filtros inexistentes
void test_processCsv_invalid_filters(void) {
    const char csv[] = "header1,header2,header3\n1,2,3\n4,5,6\n7,8,9";
    char error_output[1024] = {0};
    int saved_stderr = dup(fileno(stderr));
    redirect_stderr(TEMP_FILE);

    processCsv(csv, "header1,header2", "header1#2");

    restore_stderr(saved_stderr);
    FILE *file = fopen(TEMP_FILE, "r");
    fread(error_output, sizeof(char), sizeof(error_output) - 1, file);
    fclose(file);
    remove(TEMP_FILE);

    CU_ASSERT_STRING_EQUAL(error_output, "Invalid filter: 'header1#2'\n");
}

// Teste para operadores diferentes, maior ou igual, e menor ou igual
void test_processCsv_operators(void) {
    const char csv[] = "header1,header2,header3\n1,2,3\n4,5,6\n7,8,9";
    char output[1024] = {0};
    redirect_stdout(output);
    processCsv(csv, "header1,header3", "header1!=2\nheader2>=5\nheader3<=6");
    restore_stdout();
    CU_ASSERT_STRING_EQUAL(output, "header1,header3\n4,6\n");
}

// Teste básico para processCsvFile
void test_processCsvFile_basic(void) {
    const char *csvFilePath = "data.csv";
    char output[1024] = {0};
    redirect_stdout(output);
    processCsvFile(csvFilePath, "col1,col3,col4,col7", "col1>l1c1\ncol3>l1c3");
    restore_stdout();
    CU_ASSERT_STRING_EQUAL(output, "col1,col3,col4,col7\nl2c1,l2c3,l2c4,l2c7\nl3c1,l3c3,l3c4,l3c7\n");
}

// Teste para colunas com aspas no nome em processCsvFile
void test_processCsvFile_quoted_headers(void) {
    const char *csvFilePath = "quotedData.csv";
    char output[1024] = {0};
    redirect_stdout(output);
    processCsvFile(csvFilePath, "co\"l1,col3", "co\"l1>l1c1\ncol3<l3c3");
    restore_stdout();
    CU_ASSERT_STRING_EQUAL(output, "co\"l1,col3\nl2c1,l2c3\n");
}

// Teste para colunas em ordem arbitrária em processCsvFile
void test_processCsvFile_arbitrary_columns(void) {
    const char *csvFilePath = "data.csv";
    char output[1024] = {0};
    redirect_stdout(output);
    processCsvFile(csvFilePath, "col4,col1", "col1>l1c1\ncol3>l1c3");
    restore_stdout();
    CU_ASSERT_STRING_EQUAL(output, "col1,col4\nl2c1,l2c4\nl3c1,l3c4\n");
}

// Teste para filtros em ordem arbitrária em processCsvFile
void test_processCsvFile_arbitrary_filters(void) {
    const char *csvFilePath = "data.csv";
    char output[1024] = {0};
    redirect_stdout(output);
    processCsvFile(csvFilePath, "col1,col3,col4,col7", "col3>l1c3\ncol1>l1c1");
    restore_stdout();
    CU_ASSERT_STRING_EQUAL(output, "col1,col3,col4,col7\nl2c1,l2c3,l2c4,l2c7\nl3c1,l3c3,l3c4,l3c7\n");
}

// Teste para seleção de header vazio deve retornar todos
void test_processCsv_empty_selection(void) {
    const char csv[] = "header1,header2,header3\n1,2,3\n4,5,6\n7,8,9";
    char output[1024] = {0};
    redirect_stdout(output);
    processCsv(csv, "", "");
    restore_stdout();
    CU_ASSERT_STRING_EQUAL(output, "header1,header2,header3\n1,2,3\n4,5,6\n7,8,9\n");
}

// Teste para seleção de header vazio deve retornar todos em processCsvFile
void test_processCsvFile_empty_selection(void) {
    const char *csvFilePath = "data.csv";
    char output[1024] = {0};
    redirect_stdout(output);
    processCsvFile(csvFilePath, "", "");
    restore_stdout();
    CU_ASSERT_STRING_EQUAL(output, "col1,col2,col3,col4,col5,col6,col7\nl1c1,l1c2,l1c3,l1c4,l1c5,l1c6,l1c7\nl1c1,l1c2,l1c3,l1c4,l1c5,l1c6,l1c7\nl2c1,l2c2,l2c3,l2c4,l2c5,l2c6,l2c7\nl3c1,l3c2,l3c3,l3c4,l3c5,l3c6,l3c7\n");
}

// Teste para colunas inexistentes em processCsvFile
void test_processCsvFile_nonexistent_columns(void) {
    const char *csvFilePath = "data.csv";
    char error_output[1024] = {0};
    int saved_stderr = dup(fileno(stderr));
    redirect_stderr(TEMP_FILE);

    processCsvFile(csvFilePath, "col8,col1", "col1>l1c1\ncol3>l1c3");

    restore_stderr(saved_stderr);
    FILE *file = fopen(TEMP_FILE, "r");
    fread(error_output, sizeof(char), sizeof(error_output) - 1, file);
    fclose(file);
    remove(TEMP_FILE);

    CU_ASSERT_STRING_EQUAL(error_output, "Header 'col8' not found in CSV file/string\n");
}

// Teste para filtros inexistentes em processCsvFile
void test_processCsvFile_invalid_filters(void) {
    const char *csvFilePath = "data.csv";
    char error_output[1024] = {0};
    int saved_stderr = dup(fileno(stderr));
    redirect_stderr(TEMP_FILE);

    processCsvFile(csvFilePath, "col1,col2", "col1#2");

    restore_stderr(saved_stderr);
    FILE *file = fopen(TEMP_FILE, "r");
    fread(error_output, sizeof(char), sizeof(error_output) - 1, file);
    fclose(file);
    remove(TEMP_FILE);

    CU_ASSERT_STRING_EQUAL(error_output, "Invalid filter: 'col1#2'\n");
}

// Teste para operadores diferentes, maior ou igual, e menor ou igual em processCsvFile
void test_processCsvFile_operators(void) {
    const char *csvFilePath = "data.csv";
    char output[1024] = {0};
    redirect_stdout(output);
    processCsvFile(csvFilePath, "col1,col3", "col1!=l1c1\ncol2>=l2c2\ncol3<=l3c3");
    restore_stdout();
    CU_ASSERT_STRING_EQUAL(output, "col1,col3\nl2c1,l2c3\nl3c1,l3c3\n");
}

int main() {
    CU_initialize_registry();
    CU_pSuite suite = CU_add_suite("Suite_ProcessCSV", init_suite, clean_suite);

    CU_add_test(suite, "test of processCsv_basic", test_processCsv_basic);
    CU_add_test(suite, "test of processCsv_arbitrary_columns", test_processCsv_arbitrary_columns);
    CU_add_test(suite, "test of processCsv_arbitrary_filters", test_processCsv_arbitrary_filters);
    CU_add_test(suite, "test of processCsv_nonexistent_columns", test_processCsv_nonexistent_columns);
    CU_add_test(suite, "test of processCsv_invalid_filters", test_processCsv_invalid_filters);
    CU_add_test(suite, "test of processCsv_multiple_filters", test_processCsv_multiple_filters);
    CU_add_test(suite, "test of processCsv_multiple_filters_readme", test_processCsv_multiple_filters_readme);
    CU_add_test(suite, "test of processCsv_nonexistent_columns_and_headers_filters", test_processCsv_nonexistent_columns_and_headers_filters);
    CU_add_test(suite, "test of processCsv_operators", test_processCsv_operators);
    CU_add_test(suite, "test of processCsv_quoted_headers", test_processCsv_quoted_headers);
    CU_add_test(suite, "test of processCsvFile_basic", test_processCsvFile_basic);
    CU_add_test(suite, "test of processCsvFile_arbitrary_columns", test_processCsvFile_arbitrary_columns);
    CU_add_test(suite, "test of processCsvFile_arbitrary_filters", test_processCsvFile_arbitrary_filters);
    CU_add_test(suite, "test of processCsvFile_nonexistent_columns", test_processCsvFile_nonexistent_columns);
    CU_add_test(suite, "test of processCsvFile_invalid_filters", test_processCsvFile_invalid_filters);
    CU_add_test(suite, "test of processCsvFile_operators", test_processCsvFile_operators);
    CU_add_test(suite, "test of processCsvFile_quoted_headers", test_processCsvFile_quoted_headers);

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();

    return 0;
}

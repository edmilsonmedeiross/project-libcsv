#include <CUnit/Basic.h>
#include "libcsv.h"

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

// Teste básico para processCsv
void test_processCsv_basic(void) {
    const char csv[] = "header1,header2,header3\n1,2,3\n4,5,6\n7,8,9";
    char output[1024] = {0};
    redirect_stdout(output);
    processCsv(csv, "header1,header3", "header1>1\nheader3<9");
    restore_stdout();
    CU_ASSERT_STRING_EQUAL(output, "header1,header3\n4,6\n");
}

// Teste para colunas em ordem arbitrária
void test_processCsv_arbitrary_columns(void) {
    const char csv[] = "header1,header2,header3\n1,2,3\n4,5,6\n7,8,9";
    char output[1024] = {0};
    redirect_stdout(output);
    processCsv(csv, "header3,header1", "header1>1\nheader3<9");
    restore_stdout();
    CU_ASSERT_STRING_EQUAL(output, "header3,header1\n6,4\n");
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
    char output[1024] = {0};
    redirect_stdout(output);
    processCsv(csv, "header4,header1", "header1>1\nheader3<9");
    restore_stdout();
    CU_ASSERT_STRING_EQUAL(output, "");
}

// Teste para filtros inexistentes
void test_processCsv_invalid_filters(void) {
    const char csv[] = "header1,header2,header3\n1,2,3\n4,5,6\n7,8,9";
    char output[1024] = {0};
    redirect_stdout(output);
    processCsv(csv, "header1,header3", "header1#2");
    restore_stdout();
    CU_ASSERT_STRING_EQUAL(output, "");
}

// Teste para múltiplos filtros por header
void test_processCsv_multiple_filters(void) {
    const char csv[] = "header1,header2,header3\n1,2,3\n4,5,6\n7,8,9";
    char output[1024] = {0};
    redirect_stdout(output);
    processCsv(csv, "header1,header2,header3", "header1=1\nheader1=4\nheader2>3\nheader3>4");
    restore_stdout();
    CU_ASSERT_STRING_EQUAL(output, "header1,header2,header3\n4,5,6\n");
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

// Teste para colunas em ordem arbitrária em processCsvFile
void test_processCsvFile_arbitrary_columns(void) {
    const char *csvFilePath = "data.csv";
    char output[1024] = {0};
    redirect_stdout(output);
    processCsvFile(csvFilePath, "col4,col1", "col1>l1c1\ncol3>l1c3");
    restore_stdout();
    CU_ASSERT_STRING_EQUAL(output, "col4,col1\nl2c4,l2c1\nl3c4,l3c1\n");
}

int main() {
    CU_pSuite pSuite = NULL;

    // Inicializa o registro de testes
    if (CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();

    // Adiciona uma suíte ao registro
    pSuite = CU_add_suite("Suite_ProcessCSV", init_suite, clean_suite);
    if (NULL == pSuite) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    // Adiciona os testes à suíte
    if ((NULL == CU_add_test(pSuite, "test of processCsv_basic", test_processCsv_basic)) ||
        (NULL == CU_add_test(pSuite, "test of processCsv_arbitrary_columns", test_processCsv_arbitrary_columns)) ||
        (NULL == CU_add_test(pSuite, "test of processCsv_arbitrary_filters", test_processCsv_arbitrary_filters)) ||
        (NULL == CU_add_test(pSuite, "test of processCsv_nonexistent_columns", test_processCsv_nonexistent_columns)) ||
        (NULL == CU_add_test(pSuite, "test of processCsv_invalid_filters", test_processCsv_invalid_filters)) ||
        (NULL == CU_add_test(pSuite, "test of processCsv_multiple_filters", test_processCsv_multiple_filters)) ||
        (NULL == CU_add_test(pSuite, "test of processCsv_operators", test_processCsv_operators)) ||
        (NULL == CU_add_test(pSuite, "test of processCsvFile_basic", test_processCsvFile_basic)) ||
        (NULL == CU_add_test(pSuite, "test of processCsvFile_arbitrary_columns", test_processCsvFile_arbitrary_columns))) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    // Corre todos os testes usando o básico de CUnit
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();
    return CU_get_error();
}

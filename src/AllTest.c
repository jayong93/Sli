#include <stdio.h>
#include "CuTest.h"

CuSuite* TestModGetSuite() {
	CuSuite* suite = CuSuiteNew();
	return suite;
}

int main() {
	CuString* output = CuStringNew();
	CuSuite* suite = CuSuiteNew();

	CuSuiteAddSuite(suite, TestModGetSuite());
	
	CuSuiteRun(suite);
	CuSuiteSummary(suite, output);
	CuSuiteDetails(suite, output);
	printf("%s\n", output->buffer);
}

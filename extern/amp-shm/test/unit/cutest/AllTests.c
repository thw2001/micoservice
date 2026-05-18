#include <stdio.h>

#include "CuTest.h"

CuSuite* CuGetSuite();
CuSuite* CuStringGetSuite();
CuSuite* ShmCommGetSuite();
CuSuite* ShmQueueGetSuite();
CuSuite* ShmEndpointGetSuite();
CuSuite* PubSubGetSuite();
CuSuite* JsonConfigProcessorGetSuite();

void RunAllTests(void)
{
	CuString *output = CuStringNew();
	CuSuite* suite = CuSuiteNew();

	CuSuiteAddSuite(suite, CuGetSuite());
	CuSuiteAddSuite(suite, CuStringGetSuite());
	CuSuiteAddSuite(suite, ShmCommGetSuite());
	CuSuiteAddSuite(suite, ShmQueueGetSuite());
	CuSuiteAddSuite(suite, ShmEndpointGetSuite());
	CuSuiteAddSuite(suite, PubSubGetSuite());
	CuSuiteAddSuite(suite, JsonConfigProcessorGetSuite());

	CuSuiteRun(suite);
	CuSuiteSummary(suite, output);
	CuSuiteDetails(suite, output);
	printf("%s\n", output->buffer);
}

int main(void)
{
	RunAllTests();
}

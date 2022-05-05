// Landscape Simulator.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "cfgutils.h"
#include "Version.h"
#include "Landscape.h"

Landscape *world = NULL;

typedef void (*SignalHandlerPointer)(int);
SignalHandlerPointer previousHandler;

void SafeHandleSigINT(int signal) {
	if(world != NULL) {
		world->abortSimulation();
	}
}

int main(int argc, char* argv[])
{
	printHeaderText((char*)NULL, vString);
	/*printf("\n+++DEBUG+++\n\n");
	printf("Static Size of a location is %d bytes\n", sizeof(LocationMultiHost));
	printf("Static Size of a Population is %d bytes\n", sizeof(Population));*/

	//Memory allocation error handling
	set_new_handler(&no_storage);

	//Ensure safe handling of ctrl+c shutdown:
	previousHandler = signal(SIGINT, SafeHandleSigINT);

	int runNo = 0;
	//Run numbering:
	if(argc>1) {
		runNo = atoi(argv[1]);
	}

	world = new Landscape(runNo);

	if(!world->init(vString)) {
		printf("Input Data Error!\n");
		return EXIT_FAILURE;
	}

	if(!world->bGiveKeys) {

		printf("+++\n");
		world->simulate();
		printf("+++\n");

		if(!world->bAborted) {
			printf("Simulation Complete\n");
		} else {
			printf("Simulation Aborted\n");
		}
		printf("+++\n");

		delete world;
		world = NULL;

		return EXIT_SUCCESS;

	} else {
		printf("Keyfile generated: %s\n",world->szKeyFileName);
		return EXIT_SUCCESS;
	}
}

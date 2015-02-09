#pragma once

#include <stdio.h>
#include "alljoyn.h"


static const char ServiceName[] = "org.alljoyn.Bus.TestService";
static const char ServicePath[] = "/test";
static const uint16_t ServicePort = 25;


/**
* The interface name followed by the method signatures.
*
* See also .\inc\aj_introspect.h
*/
static const char InterfaceName[] = "org.alljoyn.Bus.TestService";
static const char GetMethodFormat[] = "?Get <sas >v";
static const char SetMethodFormat[] = "?Set <v >v";

static const char* testInterface[] = 
{
	InterfaceName,
	GetMethodFormat,
	SetMethodFormat,
	NULL
};

/**
* A NULL terminated collection of all interfaces.
*/
static const AJ_InterfaceDescription testInterfaces[] = 
{
	testInterface,
	NULL
};

/**
* Objects implemented by the application. The first member in the AJ_Object structure is the path.
* The second is the collection of all interfaces at that path.
*/
static const AJ_Object AppObjects[] = 
{
	{ ServicePath, testInterfaces },
	{ NULL }
};


static char SampleString[] = "This is just a sample test string for unmashalling request %ld";

alljoyn-testing-service
=======================

Testing service for messages marshalling and unmarshalling.

Implementation Notes
--------------------
After cloning this repository, a developer needs to get ajtcl by running these commands in the project root folder:

```
$ git clone https://git.allseenalliance.org/gerrit/core/ajtcl
$ git -C ajtcl checkout RB14.12
```

Increase the size of rxData and txData buffers so that message is big enough to handle complex signatures:

```
\\ ---------------------------------------
\\ In aj_net.c
\\ ---------------------------------------
/*
 * Statically sized buffers for I/O
 */
#define MESSAGE_BUFFER_SIZE 30000
static uint8_t rxData[MESSAGE_BUFFER_SIZE];
static uint8_t txData[MESSAGE_BUFFER_SIZE];
```

Service info
------------
* Service name: "org.alljoyn.Bus.TestService"
* Service path: "/test"
* Service port: 25
* Interface name:
```
static const char* testInterface[] = 
{
	"org.alljoyn.Bus.TestService",
	"?Get <sas >v", // <s: signature, <as: array of variant's signature (optional), >v: returned arguments basing on signature
	"?Set <v >v", // <v: signature and input arguments, >v: returned arguments
	NULL
};
```
* Sample usage for sending a marshalling request (using Set method at index 1):
```
var args =
[
	"(bynqasiuxtaxa{sv}ns)",
	false,
	127,
	-12345,
	54321,
	"String element 0", "String element 1", "String element 2", "String element 3", "String element 4", "",
	-1234567,
	1234567,
	-1234567890,
	1234567890,
	[-5555555555, -4444444444, -3333333333, -2222222222, -1111111111, 0, 1111111111, 2222222222, 3333333333, 4444444444, 5555555555],
	"Element 0: s", "s", "Element 0: s: v => 's'", "Element 1: s", "b", true, "Element 2: s", "i", -10000, "",
	10000,
	"String end"
];

status = AllJoynWinRTComponent.AllJoyn.aj_MarshalArgs(msg, "v", args);
```

How to build
------------
* Windows: Visual Studio 2013
* Linux/Arduino: https://allseenalliance.org/developers/develop/building/thin-linux

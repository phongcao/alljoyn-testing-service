#pragma once

#define CONNECT_ATTEMPTS	10

/* All times are expressed in milliseconds. */
#define CONNECT_TIMEOUT     (1000 * 60)
#define UNMARSHAL_TIMEOUT   (1000 * 5)
#define SLEEP_TIME          (1000 * 2)

#define BUFFER_SIZE			2048

#define INTERFACE_LENGTH	4

#define UNMARSHAL_INDEX		0
#define MARSHAL_INDEX		1

#define VARIANT_LIST_MAX_SIZE	100

#define SAMPLE_ARRAY_LENGTH 10
#define SAMPLE_ARRAY_CONTAINER_LENGTH 3

#define UNMARSHAL_SERVICE			AJ_APP_MESSAGE_ID(0, 0, UNMARSHAL_INDEX)
#define MARSHAL_SERVICE				AJ_APP_MESSAGE_ID(0, 0, MARSHAL_INDEX)

#define AJ_STRUCT_CLOSE          ')'
#define AJ_DICT_ENTRY_CLOSE      '}'

#define AJ_SCALAR    0x10
#define AJ_CONTAINER 0x20
#define AJ_STRING    0x40
#define AJ_VARIANT   0x80

/**
* Characterizes the various argument types
*/
static const uint8_t TypeFlags[] = {
	0x08 | AJ_CONTAINER,  /* AJ_ARG_STRUCT            '('  */
	0,                    /*                          ')'  */
	0x04 | AJ_CONTAINER,  /* AJ_ARG_ARRAY             'a'  */
	0x04 | AJ_SCALAR,     /* AJ_ARG_BOOLEAN           'b'  */
	0,
	0x08 | AJ_SCALAR,     /* AJ_ARG_DOUBLE            'd'  */
	0,
	0,
	0x01 | AJ_STRING,     /* AJ_ARG_SIGNATURE         'g'  */
	0x04 | AJ_SCALAR,     /* AJ_ARG_HANDLE            'h'  */
	0x04 | AJ_SCALAR,     /* AJ_ARG_INT32             'i'  */
	0,
	0,
	0,
	0,
	0x02 | AJ_SCALAR,     /* AJ_ARG_INT16             'n'  */
	0x04 | AJ_STRING,     /* AJ_ARG_OBJ_PATH          'o'  */
	0,
	0x02 | AJ_SCALAR,     /* AJ_ARG_UINT16            'q'  */
	0,
	0x04 | AJ_STRING,     /* AJ_ARG_STRING            's'  */
	0x08 | AJ_SCALAR,     /* AJ_ARG_UINT64            't'  */
	0x04 | AJ_SCALAR,     /* AJ_ARG_UINT32            'u'  */
	0x01 | AJ_VARIANT,    /* AJ_ARG_VARIANT           'v'  */
	0,
	0x08 | AJ_SCALAR,     /* AJ_ARG_INT64             'x'  */
	0x01 | AJ_SCALAR,     /* AJ_ARG_BYTE              'y'  */
	0,
	0x08 | AJ_CONTAINER,  /* AJ_ARG_DICT_ENTRY        '{'  */
	0,
	0                     /*                          '}'  */
};

#define TYPE_FLAG(t) TypeFlags[((t) == '(' || (t) == ')') ? (t) - '(' : (((t) < 'a' || (t) > '}') ? '}' + 2 - 'a' : (t) + 2 - 'a') ]
#define SizeOfType(typeId) (TYPE_FLAG(typeId) & 0xF)

#define IsBasicType(typeId) (TYPE_FLAG(typeId) & (AJ_STRING | AJ_SCALAR))

#define RND(minNumber, maxNumber)	(minNumber + (rand() % (maxNumber - minNumber)))
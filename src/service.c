/**
 * @file
 */
/******************************************************************************
 * Copyright (c) 2012-2014, AllSeen Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for any
 *    purpose with or without fee is hereby granted, provided that the above
 *    copyright notice and this permission notice appear in all copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 ******************************************************************************/

#include "service.h"
#include "stdio.h"
#include "aj_msg_priv.h"


static const char* GetStaticString()
{
	char str[BUFFER_SIZE];

	sprintf(str, SampleString, RND(0, UINT32_MAX));

	return str;
}


static AJ_Status MarshalArgs(AJ_Message* msg, const char** sig, const char*** vList)
{
	AJ_Arg structArg;
	AJ_Arg arg;
	AJ_Status status = AJ_OK;
	int j = 0;
	int k = 0;
	char* vSig = NULL;

	while (**sig)
	{
		uint8_t typeId = (uint8_t)*((*sig)++);
		uint8_t nextTypeId = (uint8_t)*(*sig);

		if (!IsBasicType(typeId))
		{
			if ((typeId == AJ_ARG_STRUCT) || (typeId == AJ_ARG_DICT_ENTRY))
			{
				status = AJ_MarshalContainer(msg, &structArg, typeId);

				if (status != AJ_OK)
				{
					break;
				}

				status = MarshalArgs(msg, sig, vList);

				if (status == AJ_OK)
				{
					uint8_t lastNestedTypeId = (uint8_t)*((*sig) - 1);

					if ((lastNestedTypeId == AJ_STRUCT_CLOSE) || (lastNestedTypeId == AJ_DICT_ENTRY_CLOSE)) 
					{
						status = AJ_MarshalCloseContainer(msg, &structArg);

						if (status != AJ_OK) 
						{
							break;
						}
					}
					else 
					{
						status = AJ_ERR_MARSHAL;
						break;
					}

					continue;
				}
				else
				{
					break;
				}
			}

			if ((typeId == AJ_ARG_ARRAY) && IsBasicType(nextTypeId))
			{
				if (!AJ_IsScalarType(nextTypeId)) // "as"
				{
					AJ_Arg arrayArg;
					status = AJ_MarshalContainer(msg, &arrayArg, AJ_ARG_ARRAY);

					for (j = 0; j < SAMPLE_ARRAY_LENGTH; j++)
					{
						status = AJ_MarshalArgs(msg, "s", GetStaticString());
					}

					status = AJ_MarshalCloseContainer(msg, &arrayArg);
				}
				else
				{
					// Alloc array and init random values
					size_t len = SAMPLE_ARRAY_LENGTH * SizeOfType(nextTypeId);
					uint8_t* aval = (uint8_t*)malloc(len);

					for (j = 0; j < len; j++)
					{
						aval[j] = RND(0, UINT8_MAX);
					}

					// Init args
					AJ_InitArg(&arg, nextTypeId, AJ_ARRAY_FLAG, aval, len);
					status = AJ_MarshalArg(msg, &arg);

					// Free memory
					free(aval);
				}

				(*sig)++;
				continue;
			}

			if ((typeId == AJ_STRUCT_CLOSE) || (typeId == AJ_DICT_ENTRY_CLOSE))
			{
				break;
			}

			if (typeId == AJ_ARG_VARIANT)
			{
				status = AJ_MarshalVariant(msg, **vList);
				vSig = **vList;
				status = MarshalArgs(msg, &vSig, vList);

				if (status != AJ_OK)
				{
					break;
				}

				(*vList)++;
				continue;
			}

			if ((typeId == AJ_ARG_ARRAY) && !IsBasicType(nextTypeId))
			{
				AJ_Arg arrayArg;
				char subSig[BUFFER_SIZE];
				char closeContainer = (nextTypeId == '(') ? ')' : '}';
				memcpy(subSig, *sig, strlen(*sig));
				subSig[strchr(subSig, closeContainer) - subSig + 1] = '\0';
				status = AJ_MarshalContainer(msg, &arrayArg, AJ_ARG_ARRAY);

				for (k = 0; k < SAMPLE_ARRAY_CONTAINER_LENGTH; k++)
				{
					char* inSig = subSig;
					status = MarshalArgs(msg, &inSig, vList);
				}

				status = AJ_MarshalCloseContainer(msg, &arrayArg);

				if (status != AJ_OK)
				{
					break;
				}

				*sig += strlen(subSig);
				continue;
			}

			AJ_ErrPrintf(("AJ_UnmarshalArgs(): AJ_ERR_UNEXPECTED\n"));
			status = AJ_ERR_UNEXPECTED;
			break;
		}
		else
		{
			if (AJ_IsScalarType(typeId))
			{
				void* val = NULL;
				uint8_t u8 = 0;
				uint16_t u16 = 0;
				uint32_t u32 = 0;
				uint64_t u64 = 0;

				switch (SizeOfType(typeId))
				{
				case 1:
					u8 = RND(0, UINT8_MAX);
					val = &u8;
					break;

				case 2:
					u16 = (RND(0, UINT8_MAX)) << 8;
					val = &u16;
					break;

				case 4:
					if (typeId == 'b')
					{
						u32 = RND(0, 2);
					}
					else
					{
						u32 = (RND(0, UINT8_MAX)) << 24;
					}
					val = &u32;
					break;

				case 8:
					u64 = (RND(0, UINT8_MAX)) << 56;
					val = &u64;
					break;
				}

				AJ_InitArg(&arg, typeId, 0, val, 0);
			}
			else
			{
				void* val = GetStaticString();
				AJ_InitArg(&arg, typeId, 0, val, 0);
			}

			status = AJ_MarshalArg(msg, &arg);
		}
	}

	return status;
}


static AJ_Status HandleUnmarshalRequest(AJ_Message* msg)
{
	char* sig;
	char* vList[VARIANT_LIST_MAX_SIZE];
	char** vListIter = vList;
	int vListLen = 0;
	AJ_Message reply;
	AJ_Status status = AJ_OK;

	// Get signature
	status = AJ_UnmarshalArgs(msg, "s", &sig);

	// Get variant arrays
	if (strchr(sig, 'v'))
	{
		AJ_Arg arrayArg;
		status = AJ_UnmarshalContainer(msg, &arrayArg, AJ_ARG_ARRAY);

		do
		{
			status = AJ_UnmarshalArgs(msg, "s", &vList[vListLen++]);
		} while (status == AJ_OK);

		status = AJ_UnmarshalCloseContainer(msg, &arrayArg);
		vListLen--;
	}

	// Initialize reply message
	status = AJ_MarshalReplyMsg(msg, &reply);

	if (AJ_OK != status)
	{
		return status;
	}

	// Marshal variant signature
	status = AJ_MarshalVariant(&reply, sig);

	if (AJ_OK != status)
	{
		return status;
	}

	// Marshal all args recursively
	status = MarshalArgs(&reply, &sig, &vListIter);

	if (AJ_OK != status)
	{
		return status;
	}

	// Send msg
	return AJ_DeliverMsg(&reply);
}


static AJ_Status UnmarshalArgs(AJ_Message* msg, AJ_Message* reply, const char** sig)
{
	AJ_Arg msgStructArg;
	AJ_Arg replyStructArg;
	AJ_Arg arg;
	AJ_Status status = AJ_OK;

	while (**sig)
	{
		uint8_t typeId = (uint8_t)*((*sig)++);
		uint8_t nextTypeId = (uint8_t)*(*sig);

		if (!IsBasicType(typeId))
		{
			if ((typeId == AJ_ARG_STRUCT) || (typeId == AJ_ARG_DICT_ENTRY))
			{
				status = AJ_UnmarshalContainer(msg, &msgStructArg, typeId);

				if (status != AJ_OK)
				{
					break;
				}

				status = AJ_MarshalContainer(reply, &replyStructArg, typeId);
				status = UnmarshalArgs(msg, reply, sig);

				if (status == AJ_OK)
				{
					uint8_t lastNestedTypeId = (uint8_t)*((*sig) - 1);

					if ((lastNestedTypeId == AJ_STRUCT_CLOSE) || (lastNestedTypeId == AJ_DICT_ENTRY_CLOSE))
					{
						status = AJ_UnmarshalCloseContainer(msg, &msgStructArg);
						status = AJ_MarshalCloseContainer(reply, &replyStructArg);

						if (status != AJ_OK)
						{
							break;
						}
					}
					else
					{
						status = AJ_ERR_MARSHAL;
						break;
					}

					continue;
				}
				else
				{
					break;
				}
			}

			if ((typeId == AJ_ARG_ARRAY) && IsBasicType(nextTypeId))
			{
				if (!AJ_IsScalarType(nextTypeId)) // "as"
				{
					AJ_Arg msgArrayArg;
					AJ_Arg replyArrayArg;
					status = AJ_UnmarshalContainer(msg, &msgArrayArg, AJ_ARG_ARRAY);
					status = AJ_MarshalContainer(reply, &replyArrayArg, AJ_ARG_ARRAY);

					do
					{
						char* str;
						status = AJ_UnmarshalArgs(msg, "s", &str);

						if (status != AJ_OK)
						{
							break;
						}

						status = AJ_MarshalArgs(reply, "s", str);
					} 
					while (status == AJ_OK);

					status = AJ_UnmarshalCloseContainer(msg, &msgArrayArg);
					status = AJ_MarshalCloseContainer(reply, &replyArrayArg);
				}
				else
				{
					status = AJ_UnmarshalArg(msg, &arg);
					status = AJ_MarshalArg(reply, &arg);
				}

				(*sig)++;
				continue;
			}

			if ((typeId == AJ_STRUCT_CLOSE) || (typeId == AJ_DICT_ENTRY_CLOSE))
			{
				break;
			}

			if (typeId == AJ_ARG_VARIANT)
			{
				char* inSig;
				status = AJ_UnmarshalVariant(msg, &inSig);
				status = AJ_MarshalVariant(reply, inSig);
				status = UnmarshalArgs(msg, reply, &inSig);

				if (status != AJ_OK)
				{
					break;
				}

				continue;
			}

			if ((typeId == AJ_ARG_ARRAY) && !IsBasicType(nextTypeId))
			{
				AJ_Arg msgArrayArg;
				AJ_Arg replyArrayArg;
				char subSig[BUFFER_SIZE];
				char closeContainer = (nextTypeId == '(') ? ')' : '}';
				memcpy(subSig, *sig, strlen(*sig));
				subSig[strchr(subSig, closeContainer) - subSig + 1] = '\0';
				status = AJ_UnmarshalContainer(msg, &msgArrayArg, AJ_ARG_ARRAY);
				status = AJ_MarshalContainer(reply, &replyArrayArg, AJ_ARG_ARRAY);

				do
				{
					char* inSig = subSig;
					status = UnmarshalArgs(msg, reply, &inSig);
				} 
				while (status == AJ_OK);

				status = AJ_UnmarshalCloseContainer(msg, &msgArrayArg);
				status = AJ_MarshalCloseContainer(reply, &replyArrayArg);

				if (status != AJ_OK)
				{
					break;
				}

				*sig += strlen(subSig);
				continue;
			}

			AJ_ErrPrintf(("AJ_UnmarshalArgs(): AJ_ERR_UNEXPECTED\n"));
			status = AJ_ERR_UNEXPECTED;
			break;
		}
		else
		{
			status = AJ_UnmarshalArg(msg, &arg);
			status = AJ_MarshalArg(reply, &arg);
		}
	}

	return status;
}


static AJ_Status HandleMarshalRequest(AJ_Message* msg)
{
	char* sig;
	AJ_Message reply;
	AJ_Status status = AJ_OK;

	// Initialize reply message
	status = AJ_MarshalReplyMsg(msg, &reply);

	if (AJ_OK != status)
	{
		return status;
	}

	// Get signature
	status = AJ_UnmarshalVariant(msg, &sig);

	if (AJ_OK != status)
	{
		return status;
	}

	// Marshal variant signature
	status = AJ_MarshalVariant(&reply, sig);

	if (AJ_OK != status)
	{
		return status;
	}

	// Marshal all args recursively
	status = UnmarshalArgs(msg, &reply, &sig);

	if (AJ_OK != status)
	{
		return status;
	}

	// Send msg
	return AJ_DeliverMsg(&reply);
}


int AJ_Main(void)
{
    AJ_Status status = AJ_OK;
    AJ_BusAttachment bus;
    uint8_t connected = FALSE;
    uint32_t sessionId = 0;

    /* One time initialization before calling any other AllJoyn APIs. */
    AJ_Initialize();
    AJ_RegisterObjects(AppObjects, NULL);

    while (TRUE) 
	{
        AJ_Message msg;

        if (!connected) 
		{
            status = AJ_StartService(&bus,
                                     NULL,
                                     CONNECT_TIMEOUT,
                                     FALSE,
                                     ServicePort,
                                     ServiceName,
                                     AJ_NAME_REQ_DO_NOT_QUEUE,
                                     NULL);

            if (status != AJ_OK) 
			{
                continue;
            }

            AJ_InfoPrintf(("StartService returned %d, session_id=%u\n", status, sessionId));
            connected = TRUE;
        }

        status = AJ_UnmarshalMsg(&bus, &msg, UNMARSHAL_TIMEOUT);

        if (AJ_ERR_TIMEOUT == status) 
		{
            continue;
        }

        if (AJ_OK == status) 
		{
            switch (msg.msgId) 
			{
				case AJ_METHOD_ACCEPT_SESSION:
					{
						uint16_t port;
						char* joiner;
						AJ_UnmarshalArgs(&msg, "qus", &port, &sessionId, &joiner);
						status = AJ_BusReplyAcceptSession(&msg, TRUE);
						AJ_InfoPrintf(("Accepted session session_id=%u joiner=%s\n", sessionId, joiner));
					}
					break;

				case UNMARSHAL_SERVICE:
					status = HandleUnmarshalRequest(&msg);
					break;

				case MARSHAL_SERVICE:
					status = HandleMarshalRequest(&msg);
					break;

				case AJ_SIGNAL_SESSION_LOST_WITH_REASON:
					/* Session was lost so return error to force a disconnect. */
					{
						uint32_t id, reason;
						AJ_UnmarshalArgs(&msg, "uu", &id, &reason);
						AJ_AlwaysPrintf(("Session lost. ID = %u, reason = %u", id, reason));
					}
					status = AJ_ERR_SESSION_LOST;
					break;

				default:
					/* Pass to the built-in handlers. */
					status = AJ_BusHandleBusMessage(&msg);
					break;
            }
        }

        /* Messages MUST be discarded to free resources. */
        AJ_CloseMsg(&msg);

        if ((status == AJ_ERR_SESSION_LOST || status == AJ_ERR_READ)) 
		{
            AJ_AlwaysPrintf(("AllJoyn disconnect.\n"));
            AJ_Disconnect(&bus);
            connected = FALSE;

            /* Sleep a little while before trying to reconnect. */
            AJ_Sleep(SLEEP_TIME);
        }
    }

    AJ_AlwaysPrintf(("Service exiting with status %d.\n", status));

    return status;
}

#ifdef AJ_MAIN
int main()
{
    return AJ_Main();
}
#endif


#include "rhd_atomwrapper.h"

#define INT32 INT32
#include "CD_Common_Types.h"
#include "CD_Definitions.h"


int
ParseTableWrapper(void *pspace, int index, void *handle, void *BIOSBase,
		  char **msg_return)
{
    DEVICE_DATA deviceData;
    int ret = 0;
    
    /* FILL OUT PARAMETER SPACE */
    deviceData.pParameterSpace = (UINT32*) pspace;
    deviceData.CAIL = handle;
    deviceData.pBIOS_Image = BIOSBase;
    deviceData.format = TABLE_FORMAT_BIOS;

    switch (ParseTable(&deviceData, index)) { /* IndexInMasterTable */
	case CD_SUCCESS:
	    ret = 1;
	    *msg_return = "ParseTable said: CD_SUCCESS";
	    break;
	case CD_CALL_TABLE:
	    ret = 1;
	    *msg_return = "ParseTable said: CD_CALL_TABLE";
	    break;
	case CD_COMPLETED:
	    ret = 1;
	    *msg_return = "ParseTable said: CD_COMPLETED";
	    break;
	case CD_GENERAL_ERROR:
	    ret = 0;
	    *msg_return = " ParseTable said: CD_GENERAL_ERROR";
	    break;
	case CD_INVALID_OPCODE:
	    ret = 0;
	    *msg_return = " ParseTable said: CD_INVALID_OPCODE";
	    break;
	case CD_NOT_IMPLEMENTED:
	    ret = 0;
	    *msg_return = " ParseTable said: CD_NOT_IMPLEMENTED";
	    break;
	case CD_EXEC_TABLE_NOT_FOUND:
	    ret = 0;
	    *msg_return = " ParseTable said: CD_EXEC_TABLE_NOT_FOUND";
	    break;
	case CD_EXEC_PARAMETER_ERROR:
	    ret = 0;
	    *msg_return = " ParseTable said: CD_EXEC_PARAMETER_ERROR";
	    break;
	case CD_EXEC_PARSER_ERROR:
	    ret = 0;
	    *msg_return = " ParseTable said: CD_EXEC_PARSER_ERROR";
	    break;
	case CD_INVALID_DESTINATION_TYPE:
	    ret = 0;
	    *msg_return = " ParseTable said: CD_INVALID_DESTINATION_TYPE";
	    break;
	case CD_UNEXPECTED_BEHAVIOR:
	    ret = 0;
	    *msg_return = " ParseTable said: CD_UNEXPECTED_BEHAVIOR";
	    break;
	case CD_INVALID_SWITCH_OPERAND_SIZE:
	    ret = 0;
	    *msg_return = " ParseTable said: CD_INVALID_SWITCH_OPERAND_SIZE\n";
	    break;
    }
    return ret;
}

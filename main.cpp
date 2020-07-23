#include "communication.h"
#include "dlms/include/GXDLMSCommon.h"
#include "dlms/include/GXBytebuffer.h"

int main(int argc, char *argv[])
{
	CGXDLMSSecureClient *cl;

	cl = new CGXDLMSSecureClient(true,
								16,
								(1 << 30) | (1 << 16),
								DLMS_AUTHENTICATION_HIGH_GMAC,
								"30303030303030303030303030303030",
								DLMS_INTERFACE_TYPE_HDLC);
	delete cl;

    return 0;
}

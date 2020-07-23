#include "communication.h"
#include "dlms/include/GXDLMSCommon.h"
#include "dlms/include/GXBytebuffer.h"

int main(int argc, char *argv[])
{
	CGXDLMSSecureClient *cl;

	cl = new CGXDLMSSecureClient(true,
								this->para.client,
								(1 << 30) | (this->para.logical << 16),
								this->para.level,
								this->para.password.ToString().data(),
								DLMS_INTERFACE_TYPE_HDLC);
	delete cl;

    return 0;
}

#include "communication.h"
#include "dlms/include/GXDLMSCommon.h"
#include "dlms/include/GXBytebuffer.h"

int main(int argc, char *argv[])
{
	CGXDLMSSecureClient *cl;

	cl = new CGXDLMSSecureClient(true, 16,
								((3 << 30) | (1 << 16) | 1530),
								DLMS_AUTHENTICATION_LOW,
								nullptr,
								DLMS_INTERFACE_TYPE_HDLC);

	cl->GetCiphering()->SetSecurity(DLMS_SECURITY_NONE);

	cl->SetProposedConformance(static_cast<DLMS_CONFORMANCE>(\
							   DLMS_CONFORMANCE_GENERAL_PROTECTION | \
							   DLMS_CONFORMANCE_GENERAL_BLOCK_TRANSFER | \
							   DLMS_CONFORMANCE_READ | \
							   DLMS_CONFORMANCE_WRITE | \
							   DLMS_CONFORMANCE_UN_CONFIRMED_WRITE | \
							   DLMS_CONFORMANCE_ATTRIBUTE_0_SUPPORTED_WITH_SET | \
							   DLMS_CONFORMANCE_PRIORITY_MGMT_SUPPORTED | \
							   DLMS_CONFORMANCE_ATTRIBUTE_0_SUPPORTED_WITH_GET | \
							   DLMS_CONFORMANCE_BLOCK_TRANSFER_WITH_GET_OR_READ | \
							   DLMS_CONFORMANCE_BLOCK_TRANSFER_WITH_SET_OR_WRITE | \
							   DLMS_CONFORMANCE_BLOCK_TRANSFER_WITH_ACTION | \
							   DLMS_CONFORMANCE_MULTIPLE_REFERENCES | \
							   DLMS_CONFORMANCE_INFORMATION_REPORT | \
							   DLMS_CONFORMANCE_DATA_NOTIFICATION | \
							   DLMS_CONFORMANCE_ACCESS | \
							   DLMS_CONFORMANCE_PARAMETERIZED_ACCESS | \
							   DLMS_CONFORMANCE_GET | \
							   DLMS_CONFORMANCE_SET | \
							   DLMS_CONFORMANCE_SELECTIVE_ACCESS | \
							   DLMS_CONFORMANCE_EVENT_NOTIFICATION | \
							   DLMS_CONFORMANCE_ACTION\
                               ));

	cl->SetAutoIncreaseInvokeID(false);

	CGXCommunication *comm;

	comm = new CGXCommunication(cl, 6000, GX_TRACE_LEVEL_OFF, nullptr);

	if(comm->Open("/dev/ttyS1:9600:8:Even:0", false, 115200) != 0) {
		delete comm;
		delete cl;
		std::cout << "打开串口出错" << std::endl;
		return -1;
	}

    if(comm->InitializeConnection() != 0) {
        comm->Close();
        delete comm;
        delete cl;
		std::cout << "链路初始化出错" << std::endl;
        return -1;
    }

	CGXDLMSCommon Object(8, "0.0.1.0.0.255");

	std::string value;
	if(comm->Read(&Object, 2, value) != DLMS_ERROR_CODE_OK) {
		std::cout << "读取失败" << std::endl;
	}
	else {
		std::cout << "读取成功： ";
		for (unsigned int n = 0; n < value.size(); n++) {
			std::cout.setf(std::ios::hex);
			std::cout << (static_cast<unsigned char>(value.data()[n]) & 0x000000ff);
			std::cout.unsetf(std::ios::hex);
			std::cout << " ";
		}
		std::cout << std::endl;
	}

	delete comm;
	delete cl;

    return 0;
}

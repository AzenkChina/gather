#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <time.h>
#include "communication.h"
#include "dlms/include/GXDLMSCommon.h"
#include "dlms/include/GXBytebuffer.h"

struct element {
    uint16_t classID = 0;
    std::string obis;
    uint8_t index = 0;
	CGXByteBuffer selects;
};

struct parameter {
	std::string device;

	uint8_t mode = 4;
    uint8_t client = 16;
    uint16_t logical = 1;
    uint16_t physical = 0;
	DLMS_AUTHENTICATION level = DLMS_AUTHENTICATION_NONE;
	bool negotiate = false;

	CGXByteBuffer password;
	CGXByteBuffer ekey;
    CGXByteBuffer akey;

	std::vector<struct element> elements;
};

static void arg_error(char *name) {
	static char *help_string =
	"%s: Valid parameters are:\n"
	"  -d <device> - specify the serial device, like /dev/ttyS1:9600:8Even0 in unix or COM3:9600:8Even0 in windows\n"
	"  -m <mode> - specify the address mode, value is one of 1, 2 or 4\n"
	"  -c <client> - specify the client address, range is 1~127, default is 16\n"
	"  -l <logical> - specify the logical address, range is 1~16383, default is 1\n"
	"  -p <physical> - specify the physical address, range is 0~16383\n"
	"  -s <level> - specify the access level, value is one of 0, 1 or 5, default is 0\n"
	"  -n - Use mode E to negotiate the baudrate\n"
	"  -w <password> - specify the password\n"
	"  -e <ekey> - specify the encryption key\n"
	"  -a <akey> - specify the authentication key\n"
	"  -i <class> - specify the class id\n"
	"  -o <obis> - specify the obis\n"
	"  -t <attribute> - specify the attribute id\n"
	"  -r <from-to> - specify the select parameter, can be entrys(0~65535) or timestep(>=946684800)\n"
	"  -f <file> - specify a config file\n"
	"  -h - get this message\n";

    fprintf(stderr, help_string, name);
    exit(1);
}

static void split(const std::string& s, std::vector<long long>& sv, const char flag = ' ') {
    sv.clear();
    std::istringstream iss(s);
    std::string temp;

    while (std::getline(iss, temp, flag)) {
        sv.push_back(std::stoll(temp));
    }
    return;
}

static void prase_file(char *file, struct parameter& p) {
	/* Read config file. */
	std::ifstream in(file);
	if(!in.is_open()) {
		fprintf(stderr, "Invalid config file\n");
		arg_error(file);
	}
	std::string s = "";
	std::vector<std::string> file_line;
	while(std::getline(in, s)) {
		file_line.push_back(s);
	}
	in.close();
	/* Remove clutter infomations from each line. */
	for(std::vector<std::string>::iterator iter = file_line.begin(); iter != file_line.end(); iter++) {
		if(!iter->empty()) {
			if(iter->find("#") != iter->npos) {
				iter->erase(iter->find("#"));
			}
		}
		if(!iter->empty()) {
			replace(iter->begin(), iter->end(), '\t', ' ');
		}
		if(!iter->empty()) {
			iter->erase(0,iter->find_first_not_of(" "));
		}
		if(!iter->empty()) {
			iter->erase(iter->find_last_not_of(" ") + 1);
		}
	}
	/* Remove blank line. */
	std::vector<std::string> file_trim;
	for(std::vector<std::string>::iterator iter = file_line.begin(); iter != file_line.end(); iter++) {
		if(!iter->empty()) {
			file_trim.push_back(*iter);
		}
	}
	/* Prase infomations. */
	for(std::vector<std::string>::iterator iter = file_trim.begin(); iter != file_trim.end(); iter++) {
		if(iter->empty()) {
			continue;
		}
		else {
			/* Split line with '='. */
			std::vector<std::string> line;
			line.clear();
			std::istringstream iss(*iter);
			std::string temp;
			while (std::getline(iss, temp, '=')) {
				line.push_back(temp);
			}
			if(line.size() != 2) {
				fprintf(stderr, "Invalid config file\n");
				arg_error(file);
			}

			/* Prease tag. */
			std::vector<std::string>::iterator iter = line.begin();
			std::string tag = *iter;
			if(!tag.empty()) {
				tag.erase(0,tag.find_first_not_of(" "));
			}
			if(!tag.empty()) {
				tag.erase(tag.find_last_not_of(" ") + 1);
			}
			/* Prease value. */
			++ iter;
			std::string value = *iter;
			if(!value.empty()) {
				value.erase(0,value.find_first_not_of(" "));
			}
			if(!value.empty()) {
				value.erase(value.find_last_not_of(" ") + 1);
			}

			if(tag == "device") { /* Get the device. */
				if(value.size() < 15) {
					fprintf(stderr, "Invalid config file: '%s'\n", tag.data());
					exit(1);
				}
				p.device = value;
			}
			else if(tag == "mode") { /* Get the address mode. */
				if((std::stoi(value.data()) != 1) && (std::stoi(value.data()) != 2) && (std::stoi(value.data()) != 4)) {
					fprintf(stderr, "Invalid config file: '%s'\n", tag.data());
					exit(1);
				}
				else {
					p.mode = std::stoi(value.data());
				}
			}
			else if(tag == "client") { /* Get the client address. */
				if((std::stoi(value.data()) < 1) || (std::stoi(value.data()) > 127)) {
					fprintf(stderr, "Invalid config file: '%s'\n", tag.data());
					exit(1);
				}
				else {
					p.client = std::stoi(value.data());
				}
			}
			else if(tag == "logical") { /* Get the logical address. */
				if((std::stoi(value.data()) < 1) || (std::stoi(value.data()) > 16383)) {
					fprintf(stderr, "Invalid config file: '%s'\n", tag.data());
					exit(1);
				}
				else {
					p.logical = std::stoi(value.data());
				}
			}
			else if(tag == "physical") { /* Get the physical address. */
				if((std::stoi(value.data()) < 0) || (std::stoi(value.data()) > 16383)) {
					fprintf(stderr, "Invalid config file: '%s'\n", tag.data());
					exit(1);
				}
				else {
					p.physical = std::stoi(value.data());
				}
			}
			else if(tag == "level") { /* Get the access level. */
				if(std::stoi(value.data()) == 0){
					p.level = DLMS_AUTHENTICATION_NONE;
				}
				else if(std::stoi(value.data()) == 1){
					p.level = DLMS_AUTHENTICATION_LOW;
				}
				else if(std::stoi(value.data()) == 5){
					p.level = DLMS_AUTHENTICATION_HIGH_GMAC;
				}
				else{
					fprintf(stderr, "Invalid config file: '%s'\n", tag.data());
					exit(1);
				}
			}
			else if(tag == "negotiate") { /* Get the negotiate state. */
				if(value == "true") {
					p.negotiate = true;
				}
				else if(value == "false") {
					p.negotiate = false;
				}
				else {
					fprintf(stderr, "Invalid config file: '%s'\n", tag.data());
					exit(1);
				}
			}
			else if(tag == "password") { /* Get the password. */
				if((value.size() < 16) || (value.size() % 2)) {
					fprintf(stderr, "Invalid config file: '%s'\n", tag.data());
					exit(1);
				}
				p.password.SetHexString(value.data());
			}
			else if(tag == "ekey") { /* Get the encryption key. */
				if(value.size() != 32) {
					fprintf(stderr, "Invalid config file: '%s'\n", tag.data());
					exit(1);
				}
				p.ekey.SetHexString(value.data());
			}
			else if(tag == "akey") { /* Get the authentication key. */
				if(value.size() != 32) {
					fprintf(stderr, "Invalid config file: '%s'\n", tag.data());
					exit(1);
				}
				p.akey.SetHexString(value.data());
			}
			else if(tag == "element") { /* Get element. */
				/* Split value with ' '. */
				std::vector<std::string> line;
				line.clear();
				std::istringstream iss(value);
				std::string temp;
				while (std::getline(iss, temp, ' ')) {
					line.push_back(temp);
				}
				if((line.size() < 3) || (line.size() > 4)) {
					fprintf(stderr, "Invalid config file: '%s'\n", tag.data());
					exit(1);
				}

				struct element e;
				std::vector<std::string>::iterator iter = line.begin();

				/* Prease class ID. */
				std::string id = *iter;
				if(!id.empty()) {
					id.erase(0,id.find_first_not_of(" "));
				}
				if(!id.empty()) {
					id.erase(id.find_last_not_of(" ") + 1);
				}
				if((std::stoi(id) < 1) || (std::stoi(id) > 16383)) {
					fprintf(stderr, "Invalid config file: '%s'\n", tag.data());
					exit(1);
				}
				else {
					e.classID = std::stoi(id);
				}

				/* Prease OBIS. */
				++ iter;
				std::string obis = *iter;
				if(!obis.empty()) {
					obis.erase(0,obis.find_first_not_of(" "));
				}
				if(!obis.empty()) {
					obis.erase(obis.find_last_not_of(" ") + 1);
				}
				std::vector<long long> sv;
				split(obis, sv, '.');
				if(sv.size() != 6) {
					fprintf(stderr, "Invalid config file: '%s'\n", tag.data());
					exit(1);
				}
				for (const auto& s : sv) {
					if((s < 0) || (s > 255)) {
						fprintf(stderr, "Invalid config file: '%s'\n", tag.data());
						exit(1);
					}
				}
				e.obis = obis;

				/* Prease attribute. */
				++ iter;
				std::string attr = *iter;
				if(!attr.empty()) {
					attr.erase(0,attr.find_first_not_of(" "));
				}
				if(!attr.empty()) {
					attr.erase(attr.find_last_not_of(" ") + 1);
				}
				if((std::stoi(attr) < 1) || (std::stoi(attr) > 32)) {
					fprintf(stderr, "Invalid config file: '%s'\n", tag.data());
					exit(1);
				}
				else {
					e.index = std::stoi(attr);
				}

				/* Prease selects. */
				if(line.size() == 4) {
					++ iter;
					std::string selects = *iter;
					if(!selects.empty()) {
						selects.erase(0,selects.find_first_not_of(" "));
					}
					if(!selects.empty()) {
						selects.erase(selects.find_last_not_of(" ") + 1);
					}
					if(selects.size() < 3) {
						fprintf(stderr, "Invalid config file: '%s'\n", tag.data());
						exit(1);
					}
					std::vector<long long> sv;
					split(selects, sv, '-');
					if(sv.size() != 2) {
						fprintf(stderr, "Invalid config file: '%s'\n", tag.data());
						exit(1);
					}
					if((sv[0] < 0) || (sv[1] < 0)) {
						fprintf(stderr, "Invalid config file: '%s'\n", tag.data());
						exit(1);
					}
					if(sv[1] < sv[0]) {
						fprintf(stderr, "Invalid config file: '%s'\n", tag.data());
						exit(1);
					}
					CGXByteBuffer value;
					value.Clear();
					if((sv[0] < 65536) && (sv[1] < 65536)) {
						value.SetUInt8(2);//by entry
						value.SetUInt8(DLMS_DATA_TYPE_STRUCTURE);
						value.SetUInt8(4);
						//from entry
						value.SetUInt8(DLMS_DATA_TYPE_UINT32);
						value.SetUInt32(sv[0]);
						//to entry
						value.SetUInt8(DLMS_DATA_TYPE_UINT32);
						value.SetUInt32(sv[1]);
						//from selected value
						value.SetUInt8(DLMS_DATA_TYPE_UINT16);
						value.SetUInt16(0);
						//to selected value
						value.SetUInt8(DLMS_DATA_TYPE_UINT16);
						value.SetUInt16(0);
					}
					else if((sv[0] >= 946684800) && (sv[1] >= 946684800)) {
						struct tm t;
						value.SetUInt8(1);//by range
						value.SetUInt8(DLMS_DATA_TYPE_STRUCTURE);
						value.SetUInt8(4);
						//restricting object
						value.SetUInt8(DLMS_DATA_TYPE_STRUCTURE);
						value.SetUInt8(4);
						value.SetUInt8(DLMS_DATA_TYPE_UINT16);
						value.SetUInt16(8);
						value.SetUInt8(DLMS_DATA_TYPE_OCTET_STRING);
						value.SetUInt8(6);
						value.SetHexString("0000010000FF");
						value.SetUInt8(DLMS_DATA_TYPE_INT8);
						value.SetUInt8(2);
						value.SetUInt8(DLMS_DATA_TYPE_UINT16);
						value.SetUInt16(0);
						//from
						struct tm *from = gmtime(static_cast<const time_t *>(&sv[0]));
						value.SetUInt8(DLMS_DATA_TYPE_OCTET_STRING);
						value.SetUInt8(12);
						value.SetUInt16(from->tm_year + 1900);
						value.SetUInt8(from->tm_mon + 1);
						value.SetUInt8(from->tm_mday);
						value.SetUInt8(0xff);
						value.SetUInt8(from->tm_hour);
						value.SetUInt8(from->tm_min);
						value.SetUInt8(from->tm_sec);
						value.SetUInt8(0);
						value.SetUInt16(0x8000);
						value.SetUInt8(0);
						//to
						struct tm *to = gmtime(static_cast<const time_t *>(&sv[1]));
						value.SetUInt8(DLMS_DATA_TYPE_OCTET_STRING);
						value.SetUInt8(12);
						value.SetUInt16(to->tm_year + 1900);
						value.SetUInt8(to->tm_mon + 1);
						value.SetUInt8(to->tm_mday);
						value.SetUInt8(0xff);
						value.SetUInt8(to->tm_hour);
						value.SetUInt8(to->tm_min);
						value.SetUInt8(to->tm_sec);
						value.SetUInt8(0);
						value.SetUInt16(0x8000);
						value.SetUInt8(0);
						//selected values
						value.SetUInt8(DLMS_DATA_TYPE_ARRAY);
						value.SetUInt8(0);
					}
					else {
						fprintf(stderr, "Invalid config file: '%s'\n", tag.data());
						exit(1);
					}

					e.selects = value;
				}

				/* Push the element to vector. */
				p.elements.push_back(e);
			}
			else {
				fprintf(stderr, "Invalid config file: '%s'\n", tag.data());
				exit(1);
			}
		}
	}

    return;
}

static void prase_para(int argc, char *argv[], struct parameter& p) {
	struct element e;

	if(argc <= 1) {
		arg_error(argv[0]);
	}

	for (int i = 1; i < argc; i++) {
		if ((argv[i][0] != '-') || (strlen(argv[i]) != 2)) {
			fprintf(stderr, "Invalid argument: '%s'\n", argv[i]);
			arg_error(argv[0]);
		}

		switch (argv[i][1]) {
			case 'd': { /* Get the device. */
				i++;
				if (i == argc) {
					fprintf(stderr, "Invalid argument: '%s'\n", argv[i]);
					arg_error(argv[0]);
				}
				if(strlen(argv[i]) < 15) {
					fprintf(stderr, "Invalid argument: '%s'\n", argv[i]);
					arg_error(argv[0]);
				}
				p.device = argv[i];
				break;
			}
			case 'm': { /* Get the address mode. */
				i++;
				if (i == argc) {
					fprintf(stderr, "Invalid argument: '%s'\n", argv[i]);
					arg_error(argv[0]);
				}
				if((std::stoi(argv[i]) != 1) && (std::stoi(argv[i]) != 2) && (std::stoi(argv[i]) != 4)) {
					fprintf(stderr, "Invalid argument: '%s'\n", argv[i]);
					arg_error(argv[0]);
				}
				else {
					p.mode = std::stoi(argv[i]);
				}
				break;
			}
			case 'c': { /* Get the client address. */
				i++;
				if (i == argc) {
					fprintf(stderr, "Invalid argument: '%s'\n", argv[i]);
					arg_error(argv[0]);
				}
				if((std::stoi(argv[i]) < 1) || (std::stoi(argv[i]) > 127)) {
					fprintf(stderr, "Invalid argument: '%s'\n", argv[i]);
					arg_error(argv[0]);
				}
				else {
					p.client = std::stoi(argv[i]);
				}
				break;
			}
			case 'l': { /* Get the logical address. */
				i++;
				if (i == argc) {
					fprintf(stderr, "Invalid argument: '%s'\n", argv[i]);
					arg_error(argv[0]);
				}
				if((std::stoi(argv[i]) < 1) || (std::stoi(argv[i]) > 16383)) {
					fprintf(stderr, "Invalid argument: '%s'\n", argv[i]);
					arg_error(argv[0]);
				}
				else {
					p.logical = std::stoi(argv[i]);
				}
				break;
			}
			case 'p': { /* Get the physical address. */
				i++;
				if (i == argc) {
					fprintf(stderr, "Invalid argument: '%s'\n", argv[i]);
					arg_error(argv[0]);
				}
				if((std::stoi(argv[i]) < 0) || (std::stoi(argv[i]) > 16383)) {
					fprintf(stderr, "Invalid argument: '%s'\n", argv[i]);
					arg_error(argv[0]);
				}
				else {
					p.physical = std::stoi(argv[i]);
				}
				break;
			}
			case 's': { /* Get the access level. */
				i++;
				if (i == argc) {
					fprintf(stderr, "Invalid argument: '%s'\n", argv[i]);
					arg_error(argv[0]);
				}
				if(std::stoi(argv[i]) == 0){
					p.level = DLMS_AUTHENTICATION_NONE;
				}
				else if(std::stoi(argv[i]) == 1){
					p.level = DLMS_AUTHENTICATION_LOW;
				}
				else if(std::stoi(argv[i]) == 5){
					p.level = DLMS_AUTHENTICATION_HIGH_GMAC;
				}
				else{
					fprintf(stderr, "Invalid argument: '%s'\n", argv[i]);
					arg_error(argv[0]);
				}
				break;
			}
			case 'n': { /* Get the negotiate state. */
				p.negotiate = true;
				break;
			}
			case 'w': { /* Get the password. */
				i++;
				if (i == argc) {
					fprintf(stderr, "Invalid argument: '%s'\n", argv[i]);
					arg_error(argv[0]);
				}
				if((strlen(argv[i]) < 16) || (strlen(argv[i]) % 2)) {
					fprintf(stderr, "Invalid argument: '%s'\n", argv[i]);
					arg_error(argv[0]);
				}
				p.password.SetHexString(argv[i]);
				break;
			}
			case 'e': { /* Get the encryption key. */
				i++;
				if (i == argc) {
					fprintf(stderr, "Invalid argument: '%s'\n", argv[i]);
					arg_error(argv[0]);
				}
				if(strlen(argv[i]) != 32) {
					fprintf(stderr, "Invalid argument: '%s'\n", argv[i]);
					arg_error(argv[0]);
				}
				p.ekey.SetHexString(argv[i]);
				break;
			}
			case 'a': { /* Get the authentication key. */
				i++;
				if (i == argc) {
					fprintf(stderr, "Invalid argument: '%s'\n", argv[i]);
					arg_error(argv[0]);
				}
				if(strlen(argv[i]) != 32) {
					fprintf(stderr, "Invalid argument: '%s'\n", argv[i]);
					arg_error(argv[0]);
				}
				p.akey.SetHexString(argv[i]);
				break;
			}
			case 'i': { /* Get the class id. */
				i++;
				if (i == argc) {
					fprintf(stderr, "Invalid argument: '%s'\n", argv[i]);
					arg_error(argv[0]);
				}
				if((std::stoi(argv[i]) < 1) || (std::stoi(argv[i]) > 16383)) {
					fprintf(stderr, "Invalid argument: '%s'\n", argv[i]);
					arg_error(argv[0]);
				}
				else {
					e.classID = std::stoi(argv[i]);
				}
				break;
			}
			case 'o': { /* Get the obis. */
				i++;
				if (i == argc) {
					fprintf(stderr, "Invalid argument: '%s'\n", argv[i]);
					arg_error(argv[0]);
				}
				std::vector<long long> sv;
				split(argv[i], sv, '.');
				if(sv.size() != 6) {
					fprintf(stderr, "Invalid argument: '%s'\n", argv[i]);
					arg_error(argv[0]);
				}
				for (const auto& s : sv) {
					if((s < 0) || (s > 255)) {
						fprintf(stderr, "Invalid argument: '%s'\n", argv[i]);
						arg_error(argv[0]);
					}
				}
				e.obis = argv[i];
				break;
			}
			case 't': { /* Get the attribute id. */
				i++;
				if (i == argc) {
					fprintf(stderr, "Invalid argument: '%s'\n", argv[i]);
					arg_error(argv[0]);
				}
				if((std::stoi(argv[i]) < 1) || (std::stoi(argv[i]) > 32)) {
					fprintf(stderr, "Invalid argument: '%s'\n", argv[i]);
					arg_error(argv[0]);
				}
				else {
					e.index = std::stoi(argv[i]);
				}
				break;
			}
			case 'r': { /* Get the select parameter. */
				i++;
				if (i == argc) {
					fprintf(stderr, "Invalid argument: '%s'\n", argv[i]);
					arg_error(argv[0]);
				}
				std::vector<long long> sv;
				split(argv[i], sv, '-');
				if(sv.size() != 2) {
					fprintf(stderr, "Invalid argument: '%s'\n", argv[i]);
					arg_error(argv[0]);
				}
				if((sv[0] < 0) || (sv[1] < 0)) {
					fprintf(stderr, "Invalid argument: '%s'\n", argv[i]);
					arg_error(argv[0]);
				}
				if(sv[1] < sv[0]) {
					fprintf(stderr, "Invalid argument: '%s'\n", argv[i]);
					arg_error(argv[0]);
				}
				CGXByteBuffer value;
				value.Clear();
				if((sv[0] < 65536) && (sv[1] < 65536)) {
					value.SetUInt8(2);//by entry
					value.SetUInt8(DLMS_DATA_TYPE_STRUCTURE);
					value.SetUInt8(4);
					//from entry
					value.SetUInt8(DLMS_DATA_TYPE_UINT32);
					value.SetUInt32(sv[0]);
					//to entry
					value.SetUInt8(DLMS_DATA_TYPE_UINT32);
					value.SetUInt32(sv[1]);
					//from selected value
					value.SetUInt8(DLMS_DATA_TYPE_UINT16);
					value.SetUInt16(0);
					//to selected value
					value.SetUInt8(DLMS_DATA_TYPE_UINT16);
					value.SetUInt16(0);
				}
				else if((sv[0] >= 946684800) && (sv[1] >= 946684800)) {
					struct tm t;
					value.SetUInt8(1);//by range
					value.SetUInt8(DLMS_DATA_TYPE_STRUCTURE);
					value.SetUInt8(4);
					//restricting object
					value.SetUInt8(DLMS_DATA_TYPE_STRUCTURE);
					value.SetUInt8(4);
					value.SetUInt8(DLMS_DATA_TYPE_UINT16);
					value.SetUInt16(8);
					value.SetUInt8(DLMS_DATA_TYPE_OCTET_STRING);
					value.SetUInt8(6);
					value.SetHexString("0000010000FF");
					value.SetUInt8(DLMS_DATA_TYPE_INT8);
					value.SetUInt8(2);
					value.SetUInt8(DLMS_DATA_TYPE_UINT16);
					value.SetUInt16(0);
					//from
					struct tm *from = gmtime(static_cast<const time_t *>(&sv[0]));
					value.SetUInt8(DLMS_DATA_TYPE_OCTET_STRING);
					value.SetUInt8(12);
					value.SetUInt16(from->tm_year + 1900);
					value.SetUInt8(from->tm_mon + 1);
					value.SetUInt8(from->tm_mday);
					value.SetUInt8(0xff);
					value.SetUInt8(from->tm_hour);
					value.SetUInt8(from->tm_min);
					value.SetUInt8(from->tm_sec);
					value.SetUInt8(0);
					value.SetUInt16(0x8000);
					value.SetUInt8(0);
					//to
					struct tm *to = gmtime(static_cast<const time_t *>(&sv[1]));
					value.SetUInt8(DLMS_DATA_TYPE_OCTET_STRING);
					value.SetUInt8(12);
					value.SetUInt16(to->tm_year + 1900);
					value.SetUInt8(to->tm_mon + 1);
					value.SetUInt8(to->tm_mday);
					value.SetUInt8(0xff);
					value.SetUInt8(to->tm_hour);
					value.SetUInt8(to->tm_min);
					value.SetUInt8(to->tm_sec);
					value.SetUInt8(0);
					value.SetUInt16(0x8000);
					value.SetUInt8(0);
					//selected values
					value.SetUInt8(DLMS_DATA_TYPE_ARRAY);
					value.SetUInt8(0);
				}
				else {
					fprintf(stderr, "Invalid argument: '%s'\n", argv[i]);
					arg_error(argv[0]);
				}
				
				e.selects = value;
				break;
			}
			case 'f': { /* Get configs from file. */
				i++;
				if (i == argc) {
					fprintf(stderr, "Invalid argument: '%s'\n", argv[i]);
					arg_error(argv[0]);
				}
				prase_file(argv[i], p);
				break;
			}
			case 'h': { /* Get help. */
				arg_error(argv[0]);
				break;
			}
			default: {
				fprintf(stderr, "Invalid option: '%s'\n", argv[i]);
				arg_error(argv[0]);
			}
		}
	}

	/* Push the element to vector. */
	if((e.classID != 0) && (e.obis.size() > 10) && (e.index != 0)) {
		p.elements.push_back(e);
	}

	/* Check if the device string is valid. */
	if(p.device.size() < 15) {
		fprintf(stderr, "Device should be specified correctly\n");
		exit(1);
	}

	/* Check if the password is valid when the access level is DLMS_AUTHENTICATION_LOW. */
	if(p.level == DLMS_AUTHENTICATION_LOW) {
		if(p.password.GetSize() < 8) {
			fprintf(stderr, "Password should be specified correctly\n");
			exit(1);
		}
	}
	/* Check if the ekey & akey is valid when the access level is DLMS_AUTHENTICATION_HIGH_GMAC. */
	else if(p.level == DLMS_AUTHENTICATION_HIGH_GMAC) {
		if((p.ekey.GetSize() != 16) || (p.akey.GetSize() != 16)) {
			fprintf(stderr, "Invalid ekey or akey\n");
			exit(1);
		}
	}

	/* Check if the elements is empty. */
	if(p.elements.size() < 1) {
		fprintf(stderr, "At least 1 element should be specified\n");
		exit(1);
	}

    return;
}


int main(int argc, char *argv[]) {
	struct parameter param;

	prase_para(argc, argv, param);

	CGXDLMSSecureClient *cl;

    if(param.mode == 1) {
        if(param.level == DLMS_AUTHENTICATION_LOW) {
            cl = new CGXDLMSSecureClient(true,
                                         param.client,
                                         (1 << 30) | (param.logical << 16),
                                         param.level,
                                         param.password.ToString().data(),
                                         DLMS_INTERFACE_TYPE_HDLC);
        }
        else {
            cl = new CGXDLMSSecureClient(true,
                                         param.client,
                                         (1 << 30) | (param.logical << 16),
                                         param.level,
                                         nullptr,
                                         DLMS_INTERFACE_TYPE_HDLC);
        }
    }
    else if (param.mode == 2) {
        if(param.level == DLMS_AUTHENTICATION_LOW) {
            cl = new CGXDLMSSecureClient(true,
                                         param.client,
                                         (2 << 30) | (param.logical << 16) | (param.physical % 100),
                                         param.level,
                                         param.password.ToString().data(),
                                         DLMS_INTERFACE_TYPE_HDLC);
        }
        else {
            cl = new CGXDLMSSecureClient(true,
                                         param.client,
                                         (2 << 30) | (param.logical << 16) | (param.physical % 100),
                                         param.level,
                                         nullptr,
                                         DLMS_INTERFACE_TYPE_HDLC);
        }
    }
    else {
        if(param.level == DLMS_AUTHENTICATION_LOW) {
            cl = new CGXDLMSSecureClient(true,
                                         param.client,
                                         (3 << 30) | (param.logical << 16) | (param.physical % 10000),
                                         param.level,
                                         param.password.ToString().data(),
                                         DLMS_INTERFACE_TYPE_HDLC);
        }
        else {
            cl = new CGXDLMSSecureClient(true, param.client,
                                         (3 << 30) | (param.logical << 16) | (param.physical % 10000),
                                         param.level,
                                         nullptr,
                                         DLMS_INTERFACE_TYPE_HDLC);
        }
    }

    if(param.level == DLMS_AUTHENTICATION_HIGH_GMAC) {
        cl->GetCiphering()->SetSecurity(DLMS_SECURITY_AUTHENTICATION_ENCRYPTION);
    }
    else {
        cl->GetCiphering()->SetSecurity(DLMS_SECURITY_NONE);
    }

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

    CGXByteBuffer bb;

	bb.Clear();
	bb.SetHexString("415A534552564552");
	cl->GetCiphering()->SetSystemTitle(bb);
	bb.Clear();
	bb.SetHexString("31323334353637383132333435363738");
	cl->GetCiphering()->SetDedicatedKey(bb);
	cl->GetCiphering()->SetAuthenticationKey(param.akey);
	cl->GetCiphering()->SetBlockCipherKey(param.ekey);

	CGXCommunication *comm;
	comm = new CGXCommunication(cl, 6000, GX_TRACE_LEVEL_OFF, nullptr);

	if(comm->Open(param.device.data(), param.negotiate, 115200) != 0) {
		delete comm;
		delete cl;
		fprintf(stderr, "Failed to open device\n");
		return -1;
	}

    if(comm->InitializeConnection() != 0) {
        comm->Close();
        delete comm;
        delete cl;
		fprintf(stderr, "Failed to initialize the link layer\n");
        return -1;
    }

	for(std::vector<struct element>::iterator iter = param.elements.begin(); iter != param.elements.end(); iter++) {
		CGXDLMSCommon Object(iter->classID, iter->obis.data());
		std::string result;

		if(comm->Read(&Object, iter->index, &iter->selects, result) != DLMS_ERROR_CODE_OK) {
			fprintf(stdout, "NULL ");
		}
		else {
			for (const auto& c : result) {
				fprintf(stdout, "%02X", static_cast<unsigned char>(c));
			}
			fprintf(stdout, " ");
		}
	}
	fprintf(stdout, "\n");

	comm->Close();
	delete comm;
	delete cl;
    return 0;
}

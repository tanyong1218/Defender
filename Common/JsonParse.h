#pragma once
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include "WarnLogSender.h"
#include "StringHelper.h"
#include "nlohmann/json.hpp"
using json = nlohmann::json;
using namespace std;

#define JSON_DEFAULT_STRING		"-"

//JsonHelperπ¶ƒ‹¿‡
class JsonHelper
{
public:
	JsonHelper();
	~JsonHelper();

public:
	static std::string UserActionLog_GetJsonByVector(__in string ComputerID, __in WORD cmdType , __in WORD cmdID, __in vector<ADMIN_OPERATION_LOG_STRUCT>& vecUserActionLog);
private:

};

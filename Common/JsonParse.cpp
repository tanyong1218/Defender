#include "JsonParse.h"
#include "StringHelper.h"
#include "LogHelper.h"
JsonHelper::JsonHelper()
{

}

JsonHelper::~JsonHelper()
{
}

/*
[
    {
        "CMDContent": 
		[
            {
                "LogContent": "登陆成功",
                "Time": "2021-10-01 16-00-00",
                "UserName": "Admin",
                "dwIsSuccess": 1
            }
        ],
        "CMDID": 101,
        "CMDTYPE": 1,
        "ComputerID": "1111"
    }
]
*/
std::string JsonHelper::UserActionLog_GetJsonByVector(string ComputerID, WORD cmdType, WORD cmdID, vector<ADMIN_OPERATION_LOG_STRUCT>& vecUserActionLog)
{
	std::string sJsonPacket = "";
	std::string sJsonBody = "";

	int nCount = (int)vecUserActionLog.size();
	if (nCount == 0) return sJsonPacket;

	nlohmann::json root1;
	nlohmann::json root2;
	nlohmann::json person;
	nlohmann::json CMDContent;

	for (int i = 0; i < nCount; i++)
	{
		ADMIN_OPERATION_LOG_STRUCT* pUserActionLog = &vecUserActionLog[i];

		std::wstring wsTemp = L"";

		wsTemp = CStrUtil::convertTimeTToStr((DWORD)pUserActionLog->llTime);
		CMDContent["Time"] = CStrUtil::UnicodeToUTF8(wsTemp);

		wsTemp = pUserActionLog->szUserName;
		CMDContent["UserName"] = CStrUtil::UnicodeToUTF8(wsTemp);

		wsTemp = pUserActionLog->szLogContent;

		if (wsTemp.length() > 0)
		{
			CMDContent["LogContent"] = CStrUtil::UnicodeToUTF8(wsTemp);
		}
		else
		{
			CMDContent["LogContent"] = JSON_DEFAULT_STRING;
		}

		CMDContent["dwIsSuccess"] = (int)pUserActionLog->dwIsSuccess;

		root1.push_back(CMDContent);
	}

	person["ComputerID"] = ComputerID;
	person["CMDTYPE"] = (int)cmdType;
	person["CMDID"] = (int)cmdID;
	person["CMDContent"] = root1;

	root2.push_back(person);
	sJsonPacket = root2.dump(); // 4 表示缩进 4 个空格
	
	auto j = json::parse(sJsonPacket);

	return sJsonPacket;
}


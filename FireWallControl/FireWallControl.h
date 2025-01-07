// 下列 ifdef 块是创建使从 DLL 导出更简单的
// 宏的标准方法。此 DLL 中的所有文件都是用命令行上定义的 FIREWALLCONTROL_EXPORTS
// 符号编译的。在使用此 DLL 的
// 任何项目上不应定义此符号。这样，源文件中包含此文件的任何其他项目都会将
// FIREWALLCONTROL_API 函数视为是从 DLL 导入的，而此 DLL 则将用此宏定义的
// 符号视为是被导出的。
#ifdef FIREWALLCONTROL_EXPORTS
#define FIREWALLCONTROL_API __declspec(dllexport)
#else
#define FIREWALLCONTROL_API __declspec(dllimport)
#endif


#include "IComponent.h"
#include <LogHelper.h>
#include <WindowsHelper.h>
#include "FirewallBase.h"

class CFireWallControl : public IComponent {
public:
	CFireWallControl();
	~CFireWallControl();
	CFireWallControl(const CFireWallControl&) = delete;
	CFireWallControl& operator=(const CFireWallControl&) = delete;

	//单例模式
	static CFireWallControl& GetInstance();


public:
	DWORD UnRegister() override;
	IComponent* Register() override;
	BOOL EnableFunction() override;
	BOOL DisableFunction() override;
	BOOL DispatchMessages(IPC_MSG_DATA* pIpcMsg) override;
private:
	CFirewallBase *m_pFirewallBase;
};

//extern "C" 的作用是禁止 C++ 编译器对函数名进行修饰，
//使其按照 C 语言的规则生成函数名。这样，C++ 函数就可以被 C 代码或其他外部程序正确调用。

//在 C++ 中，为了支持函数重载和命名空间等特性，编译器会对函数名进行修饰（mangling），
//生成一个唯一的内部名称。例如，函数 void foo(int) 可能会被修饰为 _foo@4 或类似的名称。
extern "C" __declspec(dllexport) IComponent* GetComInstance();


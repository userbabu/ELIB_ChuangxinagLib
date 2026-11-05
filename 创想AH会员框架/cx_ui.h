// 创想AH会员框架 · 界面层声明
#ifndef CXAH_UI_H
#define CXAH_UI_H

#include <windows.h>
#include <string>

namespace cx {

// 弹出会员向导窗口：page 见 CxAhPage。返回真表示已拿到授权。
bool Ui_ShowMemberWindow(int page);
// 独立小窗口：换机验证（先申请换机再输序列号），返回接口错误码文本。
std::string Ui_ShowChangeWindow(const std::string& email);
// 命令层入口：只弹激活窗口，返回真表示已拿到授权
bool Ui_ShowRenewWindow();
// 用户中心：查看账号/机器码/版本/到期/选配功能，可开购买页、刷新授权、激活序列号
bool Ui_ShowUserCenter(const std::string& email, HWND owner);

}  // namespace cx

#endif

// 会员窗口控件 ID：界面实现与离线 UI 宿主共用同一份，避免编号漂移
// 版面：顶部品牌横幅 + 白色卡片式向导页 + 底部文字链接
#ifndef CXAH_UI_IDS_H
#define CXAH_UI_IDS_H

enum CxAhPage {
    PG_WELCOME = 0,   // 登录或注册
    PG_LOGIN = 1,     // 登录账号
    PG_REGISTER = 2,  // 填写注册信息
    PG_GRANT = 3,     // 开通授权
    PG_DONE = 4,      // 授权成功
    PG_COUNT = 5
};

// 序列号激活 / 换机验证是独立小窗口，不占向导页
enum CxAhKeyGeom { CXKEY_W = 520, CXKEY_H = 320 };
enum CxAhUcGeom { CXUC_W = 620, CXUC_H = 470 };

enum CxAhCtl {
    // 登录或注册
    IDC_TITLE_WELCOME, IDC_INTRO, IDC_BTN_W_LOGIN, IDC_HINT_W_LOGIN, IDC_BTN_W_REG, IDC_HINT_W_REG,

    // 登录账号
    IDC_TITLE_LOGIN, IDC_NOTE_LOGIN, IDC_LBL_LOGIN_EMAIL, IDC_LOGIN_EMAIL,
    IDC_LBL_MAC, IDC_MAC, IDC_BTN_COPYMAC, IDC_BTN_LOGIN,
    IDC_LOGIN_AGREE, IDC_BTN_AGREE, IDC_BTN_FREE, IDC_LOGIN_TIP, IDC_TERMS,

    // 填写注册信息
    IDC_TITLE_REGISTER, IDC_NOTE_REG, IDC_BUSY, IDC_SPIN, IDC_BUSY_TEXT, IDC_GUIDE_REG,
    IDC_LBL_EMAIL, IDC_REG_EMAIL, IDC_LBL_PHONE, IDC_REG_PHONE,
    IDC_LBL_REF, IDC_REG_REF, IDC_LBL_QQ, IDC_REG_QQ,
    IDC_LBL_LEARN, IDC_REG_LEARN, IDC_LBL_CODE, IDC_REG_CODE, IDC_REG_PIC,
    IDC_REG_AGREE, IDC_BTN_AGREE2, IDC_REG_TIP, IDC_BTN_PREV, IDC_BTN_REG,

    // 开通授权
    IDC_TITLE_GRANT, IDC_NOTE_GRANT, IDC_LBL_SHOW_EMAIL, IDC_SHOW_EMAIL,
    IDC_LBL_SHOW_MAC, IDC_SHOW_MAC, IDC_BTN_COPY, IDC_SHOP, IDC_BTN_SHOP,
    IDC_BTN_TRIAL, IDC_BTN_RENEW, IDC_BTN_REFRESH,

    // 授权成功
    IDC_OKICON, IDC_TITLE_DONE, IDC_DUE, IDC_TERMS2, IDC_BTN_OK,

    // 序列号激活 / 换机小窗口（独立窗口）
    IDC_TITLE_RENEW, IDC_RENEW_NOTE, IDC_RENEW_TIP, IDC_BTN_HOME, IDC_LBL_KEY, IDC_RENEW_KEY,
    IDC_LBL_RENEW_CODE, IDC_RENEW_CODE, IDC_RENEW_PIC, IDC_BTN_PIC_REFRESH,
    IDC_BTN_ACT, IDC_BTN_RENEW_BACK,

    // 底部文字链接
    IDC_BTN_TUTORIAL, IDC_BTN_HELP, IDC_BTN_LOGOUT, IDC_BTN_SERVICE,

    // 用户中心（独立窗口）
    IDC_UC_TITLE, IDC_UC_STATE,
    IDC_LBL_ACCOUNT, IDC_UC_ACCOUNT, IDC_LBL_EDITION, IDC_UC_EDITION,
    IDC_LBL_UC_MAC, IDC_UC_MAC, IDC_BTN_UC_COPY,
    IDC_LBL_DUE, IDC_UC_DUE, IDC_LBL_LEFT, IDC_UC_LEFT,
    IDC_LBL_FEATURES, IDC_UC_FEATURES, IDC_UC_TIP,
    IDC_BTN_UC_BUY, IDC_BTN_UC_ACT, IDC_BTN_UC_REFRESH, IDC_BTN_UC_CLOSE
};

// 版面几何：界面实现与宿主点击共用同一套数字
enum CxAhGeom {
    CXAH_W = 656, CXAH_H = 558,           // 客户区
    CXAH_BANNER_H = 104,                  // 横幅高度
    CXAH_PX = 40, CXAH_PY = 132,          // 向导页内容原点
    CXAH_PW = 576, CXAH_PH = 360          // 内容区尺寸
};

static const wchar_t* const CxAhClassName = L"CxAhMemberWindow";   // 窗口类名，宿主按它 FindWindow
static const wchar_t* const CxAhKeyClassName = L"CxAhKeyWindow";    // 序列号激活 / 换机小窗口
static const wchar_t* const CxAhUcClassName = L"CxAhUserCenterWindow";  // 用户中心窗口

#endif

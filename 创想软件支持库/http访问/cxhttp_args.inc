// 由 http访问/gen_http_table.mjs 从 cxhttp_api.json 生成，勿手改。
static ARG_INFO g_argCopyCurlEasy[] = {
    { _T("对象"), _T("要复制配置的源对象：把它的配置复制到本对象。"), 0, 0, MAKELONG(CX_TYPE_CURL_EASY, 0), 0, 0 },
};
static ARG_INFO g_hCurlEasySetHandleArgs[] = {
    { _T("参数_CURL句柄"), _T("外部的 curl_easy 句柄数值；传 0 表示解绑。"), 0, 0, SDT_INT, 0, 0 },
};
static ARG_INFO g_hCurlEasySetoptIntArgs[] = {
    { _T("参数_选项"), _T("“CURLOPT_” 开头常量"), 0, 0, SDT_INT, 0, 0 },
    { _T("参数_值"), _T("根据不同的选项提供值"), 0, 0, SDT_INT, 0, 0 },
};
static ARG_INFO g_hCurlEasySetoptLongArgs[] = {
    { _T("参数_选项"), _T("“CURLOPT_” 开头常量"), 0, 0, SDT_INT, 0, 0 },
    { _T("参数_值"), _T("根据不同的选项提供值"), 0, 0, SDT_INT64, 0, 0 },
};
static ARG_INFO g_hCurlEasySetoptTextArgs[] = {
    { _T("参数_选项"), _T("“CURLOPT_” 开头常量"), 0, 0, SDT_INT, 0, 0 },
    { _T("参数_值"), _T("根据不同的选项提供值"), 0, 0, SDT_TEXT, 0, 0 },
};
static ARG_INFO g_hCurlEasySetoptBinArgs[] = {
    { _T("参数_选项"), _T("“CURLOPT_” 开头常量"), 0, 0, SDT_INT, 0, 0 },
    { _T("参数_值"), _T("根据不同的选项提供值"), 0, 0, SDT_BIN, 0, 0 },
};
static ARG_INFO g_hCurlEasyGetinfoIntArgs[] = {
    { _T("参数_信息类型"), _T("“CURLINFO_”开头常量"), 0, 0, SDT_INT, 0, 0 },
};
static ARG_INFO g_hCurlEasyGetinfoDoubleArgs[] = {
    { _T("参数_信息类型"), _T("“CURLINFO_”开头常量"), 0, 0, SDT_INT, 0, 0 },
};
static ARG_INFO g_hCurlEasyGetinfoTextArgs[] = {
    { _T("参数_信息类型"), _T("“CURLINFO_”开头常量"), 0, 0, SDT_INT, 0, 0 },
};
static ARG_INFO g_hCurlEasyGetinfoTextAryArgs[] = {
    { _T("参数_信息类型"), _T("要取的信息码，取 #CURLINFO_ 开头的常量。"), 0, 0, SDT_INT, 0, 0 },
    { _T("参数_文本数组"), _T("输出参数：按行收到结果数组。"), 0, 0, SDT_TEXT, 0, AS_RECEIVE_VAR_ARRAY },
};
static ARG_INFO g_hCurlEasyGetinfoCertArgs[] = {
    { _T("参数_信息类型"), _T("“CURLINFO_”开头常量"), 0, 0, SDT_INT, 0, 0 },
    { _T("参数_证书信息"), _T("输出参数：收到证书链对象（数量 + 链表数组）。"), 0, 0, MAKELONG(CX_TYPE_CERT_INFO, 0), 0, AS_RECEIVE_VAR },
};
static ARG_INFO g_hCurlEasyEscapeArgs[] = {
    { _T("参数_编码文本"), _T("要作百分号编码的文本。"), 0, 0, SDT_TEXT, 0, 0 },
};
static ARG_INFO g_hCurlEasyUnescapeArgs[] = {
    { _T("参数_URL"), _T("要还原的百分号编码文本。"), 0, 0, SDT_TEXT, 0, 0 },
};
static ARG_INFO g_argCopyCurlHttp[] = {
    { _T("对象"), _T("要复制配置的源对象：把它的配置复制到本对象。"), 0, 0, MAKELONG(CX_TYPE_CURL_HTTP, 0), 0, 0 },
};
static ARG_INFO g_hCurlHttpSetHandleArgs[] = {
    { _T("参数_CURL句柄"), _T("外部的 curl_easy 句柄数值；传 0 表示解绑。"), 0, 0, SDT_INT, 0, 0 },
};
static ARG_INFO g_hCurlHttpSetoptIntArgs[] = {
    { _T("参数_选项"), _T("“CURLOPT_” 开头常量"), 0, 0, SDT_INT, 0, 0 },
    { _T("参数_值"), _T("根据不同的选项提供值"), 0, 0, SDT_INT, 0, 0 },
};
static ARG_INFO g_hCurlHttpSetoptLongArgs[] = {
    { _T("参数_选项"), _T("“CURLOPT_” 开头常量"), 0, 0, SDT_INT, 0, 0 },
    { _T("参数_值"), _T("根据不同的选项提供值"), 0, 0, SDT_INT64, 0, 0 },
};
static ARG_INFO g_hCurlHttpSetoptTextArgs[] = {
    { _T("参数_选项"), _T("“CURLOPT_” 开头常量"), 0, 0, SDT_INT, 0, 0 },
    { _T("参数_值"), _T("根据不同的选项提供值"), 0, 0, SDT_TEXT, 0, 0 },
};
static ARG_INFO g_hCurlHttpSetoptBinArgs[] = {
    { _T("参数_选项"), _T("“CURLOPT_” 开头常量"), 0, 0, SDT_INT, 0, 0 },
    { _T("参数_值"), _T("根据不同的选项提供值"), 0, 0, SDT_BIN, 0, 0 },
};
static ARG_INFO g_hCurlHttpGetinfoIntArgs[] = {
    { _T("参数_信息类型"), _T("“CURLINFO_”开头常量"), 0, 0, SDT_INT, 0, 0 },
};
static ARG_INFO g_hCurlHttpGetinfoDoubleArgs[] = {
    { _T("参数_信息类型"), _T("“CURLINFO_”开头常量"), 0, 0, SDT_INT, 0, 0 },
};
static ARG_INFO g_hCurlHttpGetinfoTextArgs[] = {
    { _T("参数_信息类型"), _T("“CURLINFO_”开头常量"), 0, 0, SDT_INT, 0, 0 },
};
static ARG_INFO g_hCurlHttpGetinfoTextAryArgs[] = {
    { _T("参数_信息类型"), _T("要取的信息码，取 #CURLINFO_ 开头的常量。"), 0, 0, SDT_INT, 0, 0 },
    { _T("参数_文本数组"), _T("输出参数：按行收到结果数组。"), 0, 0, SDT_TEXT, 0, AS_RECEIVE_VAR_ARRAY },
};
static ARG_INFO g_hCurlHttpGetinfoCertArgs[] = {
    { _T("参数_信息类型"), _T("“CURLINFO_”开头常量"), 0, 0, SDT_INT, 0, 0 },
    { _T("参数_证书信息"), _T("输出参数：收到证书链对象（数量 + 链表数组）。"), 0, 0, MAKELONG(CX_TYPE_CERT_INFO, 0), 0, AS_RECEIVE_VAR },
};
static ARG_INFO g_hCurlHttpEscapeArgs[] = {
    { _T("参数_编码文本"), _T("要作百分号编码的文本。"), 0, 0, SDT_TEXT, 0, 0 },
};
static ARG_INFO g_hCurlHttpUnescapeArgs[] = {
    { _T("参数_URL"), _T("要还原的百分号编码文本。"), 0, 0, SDT_TEXT, 0, 0 },
};
static ARG_INFO g_hCurlHttpSetXferinfoArgs[] = {
    { _T("参数_回调函数"), _T("返回值：整数型【1停止，0继续】（整数型 用户数据，长整数型 总下载，长整数型 当前下载，长整数型 总上传，长整数型 当前上传）"), 0, 0, SDT_SUB_PTR, 0, 0 },
    { _T("参数_用户数据"), _T("原样回传给回调的第一个参数（clientp），可省略。"), 0, 0, SDT_INT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_hCurlHttpSetXferinfoAddrArgs[] = {
    { _T("参数_回调函数"), _T("返回值：整数型【1停止，0继续】（整数型 用户数据，长整数型 总下载，长整数型 当前下载，长整数型 总上传，长整数型 当前上传）"), 0, 0, SDT_INT, 0, 0 },
    { _T("参数_用户数据"), _T("原样回传给回调的第一个参数（clientp），可省略。"), 0, 0, SDT_INT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_hCurlHttpSetUrlArgs[] = {
    { _T("参数_url"), _T("完整网址，UTF-8 文本。"), 0, 0, SDT_TEXT, 0, 0 },
};
static ARG_INFO g_hCurlHttpSetProxyArgs[] = {
    { _T("参数_代理"), _T("格式： 代理类型://代理服务器:端口  代理类型有 Http, Socks4, Socks4a, Socks5, Socks5h"), 0, 0, SDT_TEXT, 0, 0 },
    { _T("用户名"), _T("账号，留空表示不带。"), 0, 0, SDT_TEXT, 0, 0 },
    { _T("密码"), _T("对应密码，留空表示不带。"), 0, 0, SDT_TEXT, 0, 0 },
};
static ARG_INFO g_hCurlHttpImpersonateArgs[] = {
    { _T("target"), _T("浏览器指纹型号，如 chrome107；取值见 取指纹型号列表。"), 0, 0, SDT_TEXT, 0, 0 },
    { _T("default_headers"), _T("{“chrome99”,“chrome100”,“chrome101”,“chrome104”,“chrome107”,“chrome99_android”,“edge99”,“edge101”,“safari15_3”,“safari15_5”}"), 0, 0, SDT_BOOL, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_hCurlHttpSetUserPwdArgs[] = {
    { _T("用户名"), _T("账号，留空表示不带。"), 0, 0, SDT_TEXT, 0, 0 },
    { _T("密码"), _T("对应密码，留空表示不带。"), 0, 0, SDT_TEXT, 0, 0 },
};
static ARG_INFO g_hCurlHttpSetCookieFileArgs[] = {
    { _T("参数_文件名"), _T("磁盘文件全路径。"), 0, 0, SDT_TEXT, 0, 0 },
};
static ARG_INFO g_hCurlHttpSetCookieListArgs[] = {
    { _T("参数_文件名"), _T("磁盘文件全路径。"), 0, 0, SDT_TEXT, 0, 0 },
};
static ARG_INFO g_hCurlHttpSetCookieTextArgs[] = {
    { _T("参数_Cookies"), _T("可以是Netscape / Mozilla格式的一行，也可以是 Set-Cookie：...格式。同样启动COOKIE引擎"), 0, 0, SDT_TEXT, 0, 0 },
};
static ARG_INFO g_hCurlHttpSetTimeoutMsArgs[] = {
    { _T("参数_连接超时"), _T("单位：毫秒"), 0, 0, SDT_INT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("参数_请求超时"), _T("单位：毫秒"), 0, 0, SDT_INT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_hCurlHttpSetTimeoutArgs[] = {
    { _T("参数_连接超时"), _T("单位：秒"), 0, 0, SDT_INT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("参数_请求超时"), _T("单位：秒"), 0, 0, SDT_INT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_hCurlHttpSetPostArgs[] = {
    { _T("参数_POST"), _T("真：POST； 假：GET"), 0, 0, SDT_BOOL, 0, 0 },
};
static ARG_INFO g_hCurlHttpSetPostBinArgs[] = {
    { _T("参数_POST数据"), _T("应用程序必须保持这个数据"), 0, 0, SDT_BIN, 0, 0 },
};
static ARG_INFO g_hCurlHttpSetPostTextArgs[] = {
    { _T("参数_POST数据"), _T("应用程序必须保持这个数据"), 0, 0, SDT_TEXT, 0, 0 },
};
static ARG_INFO g_hCurlHttpSetFollowArgs[] = {
    { _T("参数_重定向"), _T("真跟随 3xx 跳转。"), 0, 0, SDT_BOOL, 0, 0 },
    { _T("参数_最大次数"), _T("默认为5"), 0, 0, SDT_INT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_hCurlHttpSetKeepAliveArgs[] = {
    { _T("是否开启"), _T("真开启 TCP 长连接。"), 0, 0, SDT_BOOL, 0, 0 },
    { _T("超时时长"), _T("单位 秒  默认为60"), 0, 0, SDT_INT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_hCurlHttpSetEncodingArgs[] = {
    { _T("编码类型"), _T("留空不开启  否则为编码类型 如gzip"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_hCurlHttpSetHeadersArgs[] = {
    { _T("参数_请求头"), _T("每一个成员代表一个字段"), 0, 0, SDT_TEXT, 0, AS_RECEIVE_ARRAY_DATA },
};
static ARG_INFO g_hCurlHttpSetBufSizeArgs[] = {
    { _T("缓冲区大小"), _T("单次收数据的字节数，0 用默认 512。"), 0, 0, SDT_INT, 0, 0 },
};
static ARG_INFO g_hCurlHttpSetRefererArgs[] = {
    { _T("参数_引用页"), _T("Referer 头的值。"), 0, 0, SDT_TEXT, 0, 0 },
};
static ARG_INFO g_hCurlHttpSetHttpVersionArgs[] = {
    { _T("版本"), _T("#CURL_HTTP_VERSION_"), 0, 0, SDT_INT, 0, 0 },
};
static ARG_INFO g_hCurlHttpSetUserAgentArgs[] = {
    { _T("参数_UserAgent"), _T("文本型参数：参数_UserAgent。"), 0, 0, SDT_TEXT, 0, 0 },
};
static ARG_INFO g_hCurlHttpSetCaInfoArgs[] = {
    { _T("参数_证书文件名"), _T("如“/etc/certs/cabundle.pem”"), 0, 0, SDT_TEXT, 0, 0 },
};
static ARG_INFO g_hCurlHttpSetVerifyArgs[] = {
    { _T("参数_验证SSL证书"), _T("真校验服务端证书，假不校验（调试用）。"), 0, 0, SDT_BOOL, 0, 0 },
    { _T("参数_验证SSL证书服务器"), _T("是否额外校验证书里的域名，可省略。"), 0, 0, SDT_BOOL, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_hCurlHttpRespCookiesArgs[] = {
    { _T("参数_Cookies"), _T("输出参数：逐条收到当前保存的 Cookie。"), 0, 0, SDT_TEXT, 0, AS_RECEIVE_VAR_ARRAY },
};
static ARG_INFO g_argCopyCurlSmtp[] = {
    { _T("对象"), _T("要复制配置的源对象：把它的配置复制到本对象。"), 0, 0, MAKELONG(CX_TYPE_CURL_SMTP, 0), 0, 0 },
};
static ARG_INFO g_hCurlSmtpSetHandleArgs[] = {
    { _T("参数_CURL句柄"), _T("外部的 curl_easy 句柄数值；传 0 表示解绑。"), 0, 0, SDT_INT, 0, 0 },
};
static ARG_INFO g_hCurlSmtpSetoptIntArgs[] = {
    { _T("参数_选项"), _T("“CURLOPT_” 开头常量"), 0, 0, SDT_INT, 0, 0 },
    { _T("参数_值"), _T("根据不同的选项提供值"), 0, 0, SDT_INT, 0, 0 },
};
static ARG_INFO g_hCurlSmtpSetoptLongArgs[] = {
    { _T("参数_选项"), _T("“CURLOPT_” 开头常量"), 0, 0, SDT_INT, 0, 0 },
    { _T("参数_值"), _T("根据不同的选项提供值"), 0, 0, SDT_INT64, 0, 0 },
};
static ARG_INFO g_hCurlSmtpSetoptTextArgs[] = {
    { _T("参数_选项"), _T("“CURLOPT_” 开头常量"), 0, 0, SDT_INT, 0, 0 },
    { _T("参数_值"), _T("根据不同的选项提供值"), 0, 0, SDT_TEXT, 0, 0 },
};
static ARG_INFO g_hCurlSmtpSetoptBinArgs[] = {
    { _T("参数_选项"), _T("“CURLOPT_” 开头常量"), 0, 0, SDT_INT, 0, 0 },
    { _T("参数_值"), _T("根据不同的选项提供值"), 0, 0, SDT_BIN, 0, 0 },
};
static ARG_INFO g_hCurlSmtpGetinfoIntArgs[] = {
    { _T("参数_信息类型"), _T("“CURLINFO_”开头常量"), 0, 0, SDT_INT, 0, 0 },
};
static ARG_INFO g_hCurlSmtpGetinfoDoubleArgs[] = {
    { _T("参数_信息类型"), _T("“CURLINFO_”开头常量"), 0, 0, SDT_INT, 0, 0 },
};
static ARG_INFO g_hCurlSmtpGetinfoTextArgs[] = {
    { _T("参数_信息类型"), _T("“CURLINFO_”开头常量"), 0, 0, SDT_INT, 0, 0 },
};
static ARG_INFO g_hCurlSmtpGetinfoTextAryArgs[] = {
    { _T("参数_信息类型"), _T("要取的信息码，取 #CURLINFO_ 开头的常量。"), 0, 0, SDT_INT, 0, 0 },
    { _T("参数_文本数组"), _T("输出参数：按行收到结果数组。"), 0, 0, SDT_TEXT, 0, AS_RECEIVE_VAR_ARRAY },
};
static ARG_INFO g_hCurlSmtpGetinfoCertArgs[] = {
    { _T("参数_信息类型"), _T("“CURLINFO_”开头常量"), 0, 0, SDT_INT, 0, 0 },
    { _T("参数_证书信息"), _T("输出参数：收到证书链对象（数量 + 链表数组）。"), 0, 0, MAKELONG(CX_TYPE_CERT_INFO, 0), 0, AS_RECEIVE_VAR },
};
static ARG_INFO g_hCurlSmtpEscapeArgs[] = {
    { _T("参数_编码文本"), _T("要作百分号编码的文本。"), 0, 0, SDT_TEXT, 0, 0 },
};
static ARG_INFO g_hCurlSmtpUnescapeArgs[] = {
    { _T("参数_URL"), _T("要还原的百分号编码文本。"), 0, 0, SDT_TEXT, 0, 0 },
};
static ARG_INFO g_hCurlSmtpSetXferinfoArgs[] = {
    { _T("参数_回调函数"), _T("返回值：整数型【1停止，0继续】（整数型 用户数据，长整数型 总下载，长整数型 当前下载，长整数型 总上传，长整数型 当前上传）"), 0, 0, SDT_SUB_PTR, 0, 0 },
    { _T("参数_用户数据"), _T("原样回传给回调的第一个参数（clientp），可省略。"), 0, 0, SDT_INT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_hCurlSmtpSetXferinfoAddrArgs[] = {
    { _T("参数_回调函数"), _T("返回值：整数型【1停止，0继续】（整数型 用户数据，长整数型 总下载，长整数型 当前下载，长整数型 总上传，长整数型 当前上传）"), 0, 0, SDT_INT, 0, 0 },
    { _T("参数_用户数据"), _T("原样回传给回调的第一个参数（clientp），可省略。"), 0, 0, SDT_INT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_hCurlSmtpSetServerArgs[] = {
    { _T("参数_服务器"), _T("如：“smtp.qq.com”"), 0, 0, SDT_TEXT, 0, 0 },
    { _T("参数_端口"), _T("默认：25"), 0, 0, SDT_INT, 0, 0 },
    { _T("参数_用户名"), _T("文本型参数：参数_用户名。"), 0, 0, SDT_TEXT, 0, 0 },
    { _T("参数_密码"), _T("文本型参数：参数_密码。"), 0, 0, SDT_TEXT, 0, 0 },
};
static ARG_INFO g_hCurlSmtpSetCaInfoArgs[] = {
    { _T("参数_证书文件名"), _T("如“/etc/certs/cabundle.pem”"), 0, 0, SDT_TEXT, 0, 0 },
};
static ARG_INFO g_hCurlSmtpSetVerifyArgs[] = {
    { _T("参数_验证SSL证书"), _T("真校验服务端证书，假不校验（调试用）。"), 0, 0, SDT_BOOL, 0, 0 },
    { _T("参数_验证SSL证书服务器"), _T("是否额外校验证书里的域名，可省略。"), 0, 0, SDT_BOOL, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_hCurlSmtpSetMailArgs[] = {
    { _T("参数_主题"), _T("邮件的主题"), 0, 0, SDT_TEXT, 0, 0 },
    { _T("参数_正文"), _T("支持html"), 0, 0, SDT_TEXT, 0, 0 },
    { _T("参数_发件人"), _T("邮件地址，如：“kyozy@xx.com” 或 “酷宝贝<kyozy@xx.com>”"), 0, 0, SDT_TEXT, 0, 0 },
    { _T("参数_收件人列表"), _T("收件地址数组，信封与头部的 To 都由此生成。"), 0, 0, SDT_TEXT, 0, AS_RECEIVE_ARRAY_DATA },
    { _T("参数_附件列表"), _T("附件文件全路径数组；省略或空数组则发纯文本邮件。"), 0, 0, SDT_TEXT, 0, AS_RECEIVE_ARRAY_DATA | AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("参数_编码"), _T("默认：“gb2312”"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_argCopyCurlMulti[] = {
    { _T("对象"), _T("要复制配置的源对象：把它的配置复制到本对象。"), 0, 0, MAKELONG(CX_TYPE_CURL_MULTI, 0), 0, 0 },
};
static ARG_INFO g_hCurlMultiSetHandleArgs[] = {
    { _T("参数_CURL_MULTI句柄"), _T("外部的 curl_multi 句柄数值；传 0 表示解绑。"), 0, 0, SDT_INT, 0, 0 },
};
static ARG_INFO g_hCurlMultiAddHandleArgs[] = {
    { _T("参数_curl"), _T("要加入或移出的 curl_easy 对象，其句柄必须已创建。"), 0, 0, MAKELONG(CX_TYPE_CURL_EASY, 0), 0, AS_RECEIVE_VAR },
};
static ARG_INFO g_hCurlMultiRemoveHandleArgs[] = {
    { _T("参数_curl"), _T("要加入或移出的 curl_easy 对象，其句柄必须已创建。"), 0, 0, MAKELONG(CX_TYPE_CURL_EASY, 0), 0, AS_RECEIVE_VAR },
};
static ARG_INFO g_hCurlMultiPerformArgs[] = {
    { _T("参数_仍在运行数"), _T("这个参数返回，还在处理中的数量"), 0, 0, SDT_INT, 0, AS_RECEIVE_VAR },
};
static ARG_INFO g_hCurlMultiWaitArgs[] = {
    { _T("参数_超时时间"), _T("单位：毫秒"), 0, 0, SDT_INT, 0, 0 },
    { _T("参数_文件描述符数"), _T("numfds"), 0, 0, SDT_INT, 0, AS_RECEIVE_VAR },
};
static ARG_INFO g_hCurlMultiInfoReadArgs[] = {
    { _T("参数_剩余消息数"), _T("输出参数：队列里还剩多少条消息未读。"), 0, 0, SDT_INT, 0, AS_RECEIVE_VAR },
    { _T("参数_消息"), _T("输出参数：收到这条完成消息（msg / easy_handle / result）。"), 0, 0, MAKELONG(CX_TYPE_CURL_MSG, 0), 0, AS_RECEIVE_VAR },
};
static ARG_INFO g_hCurlMultiSetoptArgs[] = {
    { _T("参数_选项"), _T("“CURLMOPT_”开头常量"), 0, 0, SDT_INT, 0, 0 },
    { _T("参数_值"), _T("按选项要求的类型取值：整数型、长整数型、文本型或字节集。"), 0, 0, SDT_INT, 0, 0 },
};
static ARG_INFO g_argCopyUrlParam[] = {
    { _T("对象"), _T("要复制配置的源对象：把它的配置复制到本对象。"), 0, 0, MAKELONG(CX_TYPE_URL_PARAM, 0), 0, 0 },
};
static ARG_INFO g_hUrlParamInitUrlArgs[] = {
    { _T("网址"), _T("完整网址。"), 0, 0, SDT_TEXT, 0, 0 },
};
static ARG_INFO g_hUrlParamAddOrModifyArgs[] = {
    { _T("参数名称"), _T("查询参数名；内含 & 会被剔除。"), 0, 0, SDT_TEXT, 0, 0 },
    { _T("参数值"), _T("参数内容；中文按 UTF-8 百分号编码。"), 0, 0, SDT_TEXT, 0, 0 },
    { _T("参数值是否URL编码"), _T("默认为真"), 0, 0, SDT_BOOL, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("参数值是否自动转换UTF8"), _T("默认为真"), 0, 0, SDT_BOOL, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("参数值是否不编码数字"), _T("保留字母数字与 - . 不编码；省略按真。"), 0, 0, SDT_BOOL, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("参数值为空则不进行任何操作"), _T("真则值为空时直接跳过；省略按假（会写入空值）。"), 0, 0, SDT_BOOL, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_hUrlParamModifyValueArgs[] = {
    { _T("参数名称"), _T("查询参数名；内含 & 会被剔除。"), 0, 0, SDT_TEXT, 0, 0 },
    { _T("修改值"), _T("要换成的新值。"), 0, 0, SDT_TEXT, 0, 0 },
    { _T("参数值是否URL编码"), _T("默认为真"), 0, 0, SDT_BOOL, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("参数值是否自动转换UTF8"), _T("默认为真"), 0, 0, SDT_BOOL, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("参数值是否不编码数字"), _T("保留字母数字与 - . 不编码；省略按真。"), 0, 0, SDT_BOOL, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("参数值为空则不进行任何操作"), _T("真则值为空时直接跳过；省略按假（会写入空值）。"), 0, 0, SDT_BOOL, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_hUrlParamModifyNameArgs[] = {
    { _T("参数名称"), _T("查询参数名；内含 & 会被剔除。"), 0, 0, SDT_TEXT, 0, 0 },
    { _T("修改的参数名"), _T("新的参数名。"), 0, 0, SDT_TEXT, 0, 0 },
};
static ARG_INFO g_hUrlParamRemoveParamArgs[] = {
    { _T("参数名称"), _T("查询参数名；内含 & 会被剔除。"), 0, 0, SDT_TEXT, 0, 0 },
};
static ARG_INFO g_hUrlParamAddParamArgs[] = {
    { _T("参数名称"), _T("查询参数名；内含 & 会被剔除。"), 0, 0, SDT_TEXT, 0, 0 },
    { _T("参数值"), _T("参数内容；中文按 UTF-8 百分号编码。"), 0, 0, SDT_TEXT, 0, 0 },
    { _T("参数值是否URL编码"), _T("默认为真"), 0, 0, SDT_BOOL, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("参数值是否自动转换UTF8"), _T("默认为真"), 0, 0, SDT_BOOL, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("参数值是否不编码数字"), _T("默认为假"), 0, 0, SDT_BOOL, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("参数值为空则不进行任何操作"), _T("真则值为空时直接跳过；省略按假（会写入空值）。"), 0, 0, SDT_BOOL, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_hUrlParamFindParamArgs[] = {
    { _T("网址"), _T("完整网址。"), 0, 0, SDT_TEXT, 0, 0 },
    { _T("参数名称"), _T("查询参数名；内含 & 会被剔除。"), 0, 0, SDT_TEXT, 0, 0 },
};
static ARG_INFO g_argCopyAccessCurl[] = {
    { _T("对象"), _T("要复制配置的源对象：把它的配置复制到本对象。"), 0, 0, MAKELONG(CX_TYPE_ACC_CURL, 0), 0, 0 },
};
static ARG_INFO g_hAccessCurlSetRecvChunkArgs[] = {
    { _T("接收长度"), _T("默认为512字节"), 0, 0, SDT_INT, 0, 0 },
};
static ARG_INFO g_hAccessCurlSetCookieArgs[] = {
    { _T("COOKIE"), _T("设置的COOKIE 若设置 是否使用默认COOKIE 本项无效"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("是否使用默认COOKIE"), _T("默认为假 使用默认后，设置COOKIE参数 将失效，自动使用IE默认COOKIE进行提交"), 0, 0, SDT_BOOL, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("是否自动更新COOKIE_参数"), _T("默认为真 开启后 后续将自动将服务器返回的COOKIE合并到下次提交的COOKIE中"), 0, 0, SDT_BOOL, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_hAccessCurlSetDnsArgs[] = {
    { _T("服务器1"), _T("首选 DNS 服务器，逗号分隔可多台；留空不设置。"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("服务器2"), _T("备用 DNS 服务器；留空不设置。"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_hAccessCurlSetCurlHandleArgs[] = {
    { _T("CURL2"), _T("要使用的 curl_easy 句柄数值。"), 0, 0, SDT_INT, 0, 0 },
};
static ARG_INFO g_hAccessCurlSetShareDnsArgs[] = {
    { _T("SHARE句柄"), _T("curl share 句柄；留空取消共享。"), 0, 0, SDT_INT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_hAccessCurlSetOptionArgs[] = {
    { _T("Option"), _T("属性类型"), 0, 0, SDT_INT, 0, 0 },
    { _T("Value"), _T("属性数值"), 0, 0, _SDT_ALL, 0, 0 },
};
static ARG_INFO g_hAccessCurlSetOptionIntArgs[] = {
    { _T("Option"), _T("属性类型"), 0, 0, SDT_INT, 0, 0 },
    { _T("Value"), _T("属性数值"), 0, 0, SDT_INT, 0, 0 },
};
static ARG_INFO g_hAccessCurlImpersonateArgs[] = {
    { _T("脚本名称"), _T("{“chrome99”,“chrome100”,“chrome101”,“chrome104”,“chrome107”,“chrome99_android”,“edge99”,“edge101”,“safari15_3”,“safari15_5”} 详细查看 https://github.com/lwthiker/curl-impersonate"), 0, 0, SDT_TEXT, 0, 0 },
    { _T("使用默认请求头"), _T("真则同时套用该型号自带的默认请求头；一般传假，自己拼头。"), 0, 0, SDT_BOOL, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_hAccessCurlAccessPageCurlArgs[] = {
    { _T("完整网址"), _T("提供UTF8编码"), 0, 0, SDT_TEXT, 0, 0 },
    { _T("访问方式"), _T("默认GET 也可以POST"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("回调子程序"), _T("参数1整数型 void *clientp 指针,   参数2双精度小数 curl_off_t dltotal 总大小,   参数3双精度小数 curl_off_t dlnow 已下载,   参数4双精度小数 curl_off_t ultotal,  参数5双精度小数 curl_off_t ulnow"), 0, 0, SDT_INT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("提交信息"), _T("POST 正文，省略即无正文。"), 0, 0, SDT_BIN, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("超时"), _T("单位秒"), 0, 0, SDT_INT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("附加协议头"), _T("额外请求头，一行一条；缺失的常用头会自动补全。"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("返回头"), _T("请求返回的头信息"), 0, 0, SDT_TEXT, 0, AS_RECEIVE_VAR | AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("禁止重定向"), _T("禁止重定向 默认为假"), 0, 0, SDT_BOOL, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("网站登录用户名"), _T("目标站的登录用户名，留空不带。"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("网站登录密码"), _T("目标站的登录密码。"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("代理地址"), _T("0.0.0.0:0000格式"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("代理用户名"), _T("代理账号"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("代理密码"), _T("代理密码"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("是否采用Gzip"), _T("是否使用GZIP压缩 设置时确保请求头里不要包含accpent-encoding 否则就会乱套了!"), 0, 0, SDT_BOOL, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("失败日志"), _T("返回错误原因"), 0, 0, SDT_TEXT, 0, AS_RECEIVE_VAR | AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("代理连接失败自动处理"), _T("如果代理服务器连接失败，则自动取消代理重试"), 0, 0, SDT_BOOL, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("模拟浏览器TSL"), _T("默认为chrome107   其他查看 https://github.com/lwthiker/curl-impersonate/blob/main/browsers.json"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("回调用户参数"), _T("回调第一个参数"), 0, 0, SDT_INT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("回调次数"), _T("内部重试计数，首次调用请留空或 0。"), 0, 0, SDT_INT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_argCopyAccessWinHttp[] = {
    { _T("对象"), _T("要复制配置的源对象：把它的配置复制到本对象。"), 0, 0, MAKELONG(CX_TYPE_ACC_WINHTTP, 0), 0, 0 },
};
static ARG_INFO g_hAccessWinHttpSetRecvChunkArgs[] = {
    { _T("接收长度"), _T("默认为512字节"), 0, 0, SDT_INT, 0, 0 },
};
static ARG_INFO g_hAccessWinHttpSetCookieArgs[] = {
    { _T("COOKIE"), _T("设置的COOKIE 若设置 是否使用默认COOKIE 本项无效"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("是否使用默认COOKIE"), _T("默认为假 使用默认后，设置COOKIE参数 将失效，自动使用IE默认COOKIE进行提交"), 0, 0, SDT_BOOL, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("是否自动更新COOKIE_参数"), _T("默认为真 开启后 后续将自动将服务器返回的COOKIE合并到下次提交的COOKIE中"), 0, 0, SDT_BOOL, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_hAccessWinHttpAccessPageWinHttpArgs[] = {
    { _T("完整网址"), _T("要访问的网址（UTF-8）。"), 0, 0, SDT_TEXT, 0, 0 },
    { _T("访问方式"), _T("默认GET 也可以POST"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("回调子程序"), _T("所有参数必须为整数型 子程序需要5个参数 1. HTTP状态_  2为返回状态代码 3为总文件大小 4.保留 5.保留"), 0, 0, SDT_INT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("提交信息"), _T("POST 正文，省略即无正文。"), 0, 0, SDT_BIN, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("超时"), _T("单位秒"), 0, 0, SDT_INT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("附加协议头"), _T("额外请求头，一行一条；缺失的常用头会自动补全。"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("返回头"), _T("请求返回的头信息"), 0, 0, SDT_TEXT, 0, AS_RECEIVE_VAR | AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("禁止重定向"), _T("禁止重定向 默认为假"), 0, 0, SDT_BOOL, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("网站登录用户名"), _T("目标站的登录用户名，留空不带。"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("网站登录密码"), _T("目标站的登录密码。"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("代理地址"), _T("0.0.0.0:0000格式"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("代理用户名"), _T("代理账号"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("代理密码"), _T("代理密码"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("是否采用Gzip"), _T("是否使用GZIP压缩"), 0, 0, SDT_BOOL, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_argCopyAccessWinInet[] = {
    { _T("对象"), _T("要复制配置的源对象：把它的配置复制到本对象。"), 0, 0, MAKELONG(CX_TYPE_ACC_WININET, 0), 0, 0 },
};
static ARG_INFO g_hAccessWinInetSetRecvChunkArgs[] = {
    { _T("接收长度"), _T("默认为512字节"), 0, 0, SDT_INT, 0, 0 },
};
static ARG_INFO g_hAccessWinInetSetCookieArgs[] = {
    { _T("COOKIE"), _T("设置的COOKIE 若设置 是否使用默认COOKIE 本项无效"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("是否使用默认COOKIE"), _T("默认为假 使用默认后，设置COOKIE参数 将失效，自动使用IE默认COOKIE进行提交"), 0, 0, SDT_BOOL, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("是否自动更新COOKIE_参数"), _T("默认为真 开启后 后续将自动将服务器返回的COOKIE合并到下次提交的COOKIE中"), 0, 0, SDT_BOOL, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_hAccessWinInetAccessPageWinInetArgs[] = {
    { _T("完整网址"), _T("完整的网页地址"), 0, 0, SDT_TEXT, 0, 0 },
    { _T("访问方式"), _T("默认GET 也可以POST"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("回调子程序"), _T("返回常量 HTTP状态_ 开头。子程序需要三个参数 第一个为返回状态代码 第二个为总文件大小 第三个为按状态返回数值"), 0, 0, SDT_INT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("提交信息"), _T("POST专用"), 0, 0, SDT_BIN, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("超时"), _T("秒"), 0, 0, SDT_INT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("附加协议头"), _T("一行一个请用换行符隔开"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("返回头"), _T("Location"), 0, 0, SDT_TEXT, 0, AS_RECEIVE_VAR | AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("禁止重定向"), _T("默认不禁止网页重定向"), 0, 0, SDT_BOOL, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("代理地址"), _T("代理地址"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("代理用户名"), _T("代理认证用户名。"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("代理密码"), _T("代理认证密码。"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("是否采用Gzip"), _T("真则在请求头带 Accept-Encoding: gzip 并自动解压；省略按真。"), 0, 0, SDT_BOOL, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_gWebAccessCurlArgs[] = {
    { _T("完整网址"), _T("要访问的网址（UTF-8）。"), 0, 0, SDT_TEXT, 0, 0 },
    { _T("访问方式"), _T("默认GET 也可以POST"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("回调子程序"), _T("子程序内返回0为继续 除了参数1所有参数必须为双精度  1为停止 1参数为CRUL的句柄 2参数为总大小【双精度小数型】 3参数为已下载大小【双精度小数型】 4参数为总上传大小【双精度小数型】 5参数为已上传大小【双精度小数型】"), 0, 0, SDT_INT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("提交信息"), _T("POST 正文，省略即无正文。"), 0, 0, SDT_BIN, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("提交COOKIE"), _T("提交COOKIE 如果留空（空文本不算留空）提交COOKIE则使用系统默认cookie"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("返回COOKIE"), _T("提交后返回的COOKIE 返回更新后的COOKIE"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("超时"), _T("单位秒"), 0, 0, SDT_INT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("附加协议头"), _T("额外请求头，一行一条；缺失的常用头会自动补全。"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("返回头"), _T("请求返回的头信息"), 0, 0, SDT_TEXT, 0, AS_RECEIVE_VAR | AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("禁止重定向"), _T("禁止重定向 默认为假"), 0, 0, SDT_BOOL, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("网站登录用户名"), _T("目标站的登录用户名，留空不带。"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("网站登录密码"), _T("目标站的登录密码。"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("代理地址"), _T("0.0.0.0:0000格式"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("代理用户名"), _T("代理账号"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("代理密码"), _T("代理密码"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("是否采用Gzip"), _T("是否使用GZIP压缩"), 0, 0, SDT_BOOL, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("返回错误日志"), _T("失败返回的文本"), 0, 0, SDT_TEXT, 0, AS_RECEIVE_VAR | AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("代理连接失败自动重试"), _T("如果代理服务器连接失败，则自动取消代理重试 最大重试3次"), 0, 0, SDT_BOOL, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("模拟浏览器TSL"), _T("留空使用chrome107  其他请看https://github.com/lwthiker/curl-impersonate/blob/main/browsers.json"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("取消令牌"), _T("可空。传入令牌当前状态控制本次访问的取消、暂停、恢复；省略不绑定。调用期间内部保留引用，取消返回空字节集。"), 0, 0, MAKELONG(CX_TYPE_CANCEL, 0), 0, AS_RECEIVE_VAR | AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_gWebAccessWinHttpArgs[] = {
    { _T("完整网址"), _T("要访问的网址（UTF-8）。"), 0, 0, SDT_TEXT, 0, 0 },
    { _T("访问方式"), _T("默认GET 也可以POST"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("回调子程序"), _T("返回常量 HTTP状态_ 开头。所有参数必须为整数型  1状态码 2参数为总大小【双精度小数型】 3参数为已下载大小【双精度小数型】 4保留  5保留"), 0, 0, SDT_INT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("提交信息"), _T("POST 正文，省略即无正文。"), 0, 0, SDT_BIN, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("提交COOKIE"), _T("提交COOKIE 如果留空（空文本不算留空）提交COOKIE则使用系统默认cookie"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("返回COOKIE"), _T("提交后返回的COOKIE"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("超时"), _T("单位秒"), 0, 0, SDT_INT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("附加协议头"), _T("额外请求头，一行一条；缺失的常用头会自动补全。"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("返回头"), _T("请求返回的头信息"), 0, 0, SDT_TEXT, 0, AS_RECEIVE_VAR | AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("禁止重定向"), _T("禁止重定向 默认为假"), 0, 0, SDT_BOOL, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("网站登录用户名"), _T("目标站的登录用户名，留空不带。"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("网站登录密码"), _T("目标站的登录密码。"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("代理地址"), _T("为*则使用系统默认代理 0.0.0.0:0000格式"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("代理用户名"), _T("代理账号"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("代理密码"), _T("代理密码"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("是否采用Gzip"), _T("是否使用GZIP压缩"), 0, 0, SDT_BOOL, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("取消令牌"), _T("可空。传入令牌当前状态控制本次访问的取消、暂停、恢复；省略不绑定。调用期间内部保留引用，取消返回空字节集。"), 0, 0, MAKELONG(CX_TYPE_CANCEL, 0), 0, AS_RECEIVE_VAR | AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_gWebAccessWinInetArgs[] = {
    { _T("完整网址"), _T("要访问的网址（UTF-8）。"), 0, 0, SDT_TEXT, 0, 0 },
    { _T("访问方式"), _T("默认GET 也可以POST"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("回调子程序"), _T("返回常量 HTTP状态_ 开头。所有参数必须为整数型   1状态码 2参数为总大小【双精度小数型】 3参数为已下载大小【双精度小数型】 4保留 5保留"), 0, 0, SDT_INT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("提交信息"), _T("POST 正文，省略即无正文。"), 0, 0, SDT_BIN, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("提交COOKIE"), _T("提交COOKIE 如果留空（空文本不算留空）提交COOKIE则使用系统默认cookie"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("返回COOKIE"), _T("提交后返回的COOKIE 返回更新后的COOKIE"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("超时"), _T("单位秒"), 0, 0, SDT_INT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("附加协议头"), _T("额外请求头，一行一条；缺失的常用头会自动补全。"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("返回头"), _T("请求返回的头信息"), 0, 0, SDT_TEXT, 0, AS_RECEIVE_VAR | AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("禁止重定向"), _T("禁止重定向 默认为假"), 0, 0, SDT_BOOL, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("代理地址"), _T("0.0.0.0:0000格式"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("代理用户名"), _T("代理账号"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("代理密码"), _T("代理密码"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("是否采用Gzip"), _T("是否使用GZIP压缩"), 0, 0, SDT_BOOL, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("取消令牌"), _T("可空。传入令牌当前状态控制本次访问的取消、暂停、恢复；省略不绑定。调用期间内部保留引用，取消返回空字节集。"), 0, 0, MAKELONG(CX_TYPE_CANCEL, 0), 0, AS_RECEIVE_VAR | AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_gWebCookieMergeArgs[] = {
    { _T("旧Cookie"), _T("参与合并的旧 Cookie 串，可传文本常量或变量。"), 0, 0, SDT_TEXT, 0, 0 },
    { _T("新Cookie"), _T("服务端新返回的 Cookie 串，同名覆盖旧值。"), 0, 0, SDT_TEXT, 0, 0 },
};
static ARG_INFO g_gWebCookieFromHeadArgs[] = {
    { _T("返回头"), _T("输出参数：收到响应头原文，可省略。"), 0, 0, SDT_TEXT, 0, 0 },
};
static ARG_INFO g_gWebPickHeaderArgs[] = {
    { _T("头信息"), _T("要从中取字段的响应头原文。"), 0, 0, SDT_TEXT, 0, 0 },
    { _T("提取的头信息部分"), _T("例如Content-Encoding"), 0, 0, SDT_TEXT, 0, 0 },
};
static ARG_INFO g_gWebBuildHeadersArgs[] = {
    { _T("附加协议头"), _T("额外请求头，一行一条；缺失的常用头会自动补全。"), 0, 0, SDT_TEXT, 0, 0 },
    { _T("完整网址"), _T("要访问的网址（UTF-8）。"), 0, 0, SDT_TEXT, 0, 0 },
    { _T("提交COOKIE"), _T("本次要提交的 Cookie 串。"), 0, 0, SDT_TEXT, 0, 0 },
    { _T("是否为POST"), _T("逻辑型参数：是否为POST。"), 0, 0, SDT_BOOL, 0, 0 },
    { _T("是否采用GZIP"), _T("逻辑型参数：是否采用GZIP。"), 0, 0, SDT_BOOL, 0, 0 },
    { _T("提交POST字节集长度"), _T("默认不设置头 Content-Length:"), 0, 0, SDT_INT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_gJudgeHeadValidArgs[] = {
    { _T("响应头"), _T("要判断的响应头原文。"), 0, 0, SDT_TEXT, 0, 0 },
};
static ARG_INFO g_gGetHeadCodeArgs[] = {
    { _T("响应头"), _T("要判断的响应头原文。"), 0, 0, SDT_TEXT, 0, 0 },
};
static ARG_INFO g_gWebStatusCodeArgs[] = {
    { _T("响应协议头"), _T("要解析的响应头原文。"), 0, 0, SDT_TEXT, 0, 0 },
    { _T("返回文本码"), _T("返回具体错误内容"), 0, 0, SDT_TEXT, 0, AS_RECEIVE_VAR | AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_gWebBuildFormArgs[] = {
    { _T("formDatas"), _T("表单元素数组，元素为 #formData 类型。"), 0, 0, SDT_INT, 0, AS_RECEIVE_ARRAY_DATA },
    { _T("返回请求头文本"), _T("输出参数：收到 Content-Type: multipart/form-data; boundary=... 整行。"), 0, 0, SDT_TEXT, 0, AS_RECEIVE_VAR | AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_gWebContentTypeArgs[] = {
    { _T("文件名或扩展名"), _T("文件名或扩展名（可带点，如 a.png / .html）。"), 0, 0, SDT_TEXT, 0, 0 },
};
static ARG_INFO g_gAnalyseHeadCookiesArgs[] = {
    { _T("返回头"), _T("输出参数：收到响应头原文，可省略。"), 0, 0, SDT_TEXT, 0, 0 },
};
static ARG_INFO g_gGetMiddleTextArgs[] = {
    { _T("待取内容"), _T("被截取的原文本。"), 0, 0, SDT_TEXT, 0, 0 },
    { _T("起始查找位置"), _T("起始位置（1 基），命令会把游标推进到命中处之后。"), 0, 0, SDT_INT, 0, AS_RECEIVE_VAR },
    { _T("起始文本1"), _T("左标记。"), 0, 0, SDT_TEXT, 0, 0 },
    { _T("起始文本2"), _T("可选的第二层左标记，先定位它再取到结束文本。"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("结束文本"), _T("右标记。"), 0, 0, SDT_TEXT, 0, 0 },
    { _T("不区分大小写"), _T("默认为真"), 0, 0, SDT_BOOL, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("是否包含起始文本"), _T("默认真"), 0, 0, SDT_BOOL, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_gCookieMergeArgs[] = {
    { _T("原COOKIE"), _T("原有 Cookie 串。"), 0, 0, SDT_TEXT, 0, 0 },
    { _T("本次COOKIE"), _T("本次要合并进来的 Cookie 串。"), 0, 0, SDT_TEXT, 0, 0 },
};
static ARG_INFO g_gWebCookieInnerArgs[] = {
    { _T("域名"), _T("只接受域名"), 0, 0, SDT_TEXT, 0, 0 },
    { _T("Cookie名称"), _T("为空返回全部Cookie"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("是否为HTTPS"), _T("真按 https 取，假按 http。"), 0, 0, SDT_BOOL, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_gWebCookieOneArgs[] = {
    { _T("Cookie"), _T("待查找的 Cookie 串。"), 0, 0, SDT_TEXT, 0, 0 },
    { _T("名称"), _T("Cookie 名。"), 0, 0, SDT_TEXT, 0, 0 },
    { _T("不附带名称"), _T("真则只返回值，不返回「名称=」。"), 0, 0, SDT_BOOL, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_gTextCountOccurArgs[] = {
    { _T("被搜索文本"), _T("被查找的原文本。"), 0, 0, SDT_TEXT, 0, 0 },
    { _T("欲搜索文本"), _T("要查找的子串。"), 0, 0, SDT_TEXT, 0, 0 },
    { _T("是否区分大小写"), _T("真区分大小写；省略按真。"), 0, 0, SDT_BOOL, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_gCompressAmazonPageArgs[] = {
    { _T("要压缩的数据"), _T("原始页面字节集，命令返回瘦身后的新字节集。"), 0, 0, SDT_BIN, 0, 0 },
};

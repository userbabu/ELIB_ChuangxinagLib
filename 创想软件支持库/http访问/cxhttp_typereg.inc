// 由 http访问/gen_http_table.mjs 从 cxhttp_api.json 生成，勿手改。
    CX_TYPE("curl_easy", "CurlEasy", "简单接口，手动调用 curl_easy.销毁（）。原模块同名类，参数逐字一致。", g_CurlEasyCmdIndex),
    CX_TYPE("curl_http", "CurlHttp", "封装用于访问HTTP或HTTPS。原模块同名类，参数逐字一致。", g_CurlHttpCmdIndex),
    CX_TYPE("curl_smtp", "CurlSmtp", "封装用于发送邮件，继承自 curl_easy。原模块同名类，参数逐字一致。", g_CurlSmtpCmdIndex),
    CX_TYPE("curl_multi", "CurlMulti", "多接口栈：把若干 curl_easy 对象放进同一轮循环并发执行。原模块同名类，参数逐字一致。", g_CurlMultiCmdIndex),
    CX_TYPE("URL参数类", "UrlParam", "网址查询参数的增删改查（原模块同名类，逐字一致）。原模块同名类，参数逐字一致。", g_UrlParamCmdIndex),
    CX_TYPE("HTTP访问_Curl访问类", "AccessCurl", "原模块同名类：libcurl-impersonate 路线的一次性网页访问。。原模块同名类，参数逐字一致。", g_AccessCurlCmdIndex),
    CX_TYPE("HTTP访问_WinHTTP访问类", "AccessWinHttp", "原模块同名类：WinHTTP 路线的一次性网页访问。。原模块同名类，参数逐字一致。", g_AccessWinHttpCmdIndex),
    CX_TYPE("HTTP访问_WinInet访问类", "AccessWinInet", "原模块同名类：WinInet 路线的一次性网页访问。。原模块同名类，参数逐字一致。", g_AccessWinInetCmdIndex),

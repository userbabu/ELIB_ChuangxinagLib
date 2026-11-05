// 由 http访问/gen_http_table.mjs 从 cxhttp_api.json 生成，勿手改。
// curl_easy：生命周期 3 + 成员 18，行区间 395..415
INT g_CurlEasyCmdIndex[] = { 395, 396, 397, 398, 399, 400, 401, 402, 403, 404, 405, 406, 407, 408, 409, 410, 411, 412, 413, 414, 415 };
static_assert(sizeof(g_CurlEasyCmdIndex) / sizeof(INT) == 21, "curl_easy 成员命令条数变了");
static_assert(CX_TYPE_CURL_EASY == 11, "curl_easy 数据类型序号与登记顺序不一致");
// curl_http：生命周期 3 + 成员 54，行区间 416..472
INT g_CurlHttpCmdIndex[] = { 416, 417, 418, 419, 420, 421, 422, 423, 424, 425, 426, 427, 428, 429, 430, 431, 432, 433, 434, 435, 436, 437, 438, 439, 440, 441, 442, 443, 444, 445, 446, 447, 448, 449, 450, 451, 452, 453, 454, 455, 456, 457, 458, 459, 460, 461, 462, 463, 464, 465, 466, 467, 468, 469, 470, 471, 472 };
static_assert(sizeof(g_CurlHttpCmdIndex) / sizeof(INT) == 57, "curl_http 成员命令条数变了");
static_assert(CX_TYPE_CURL_HTTP == 12, "curl_http 数据类型序号与登记顺序不一致");
// curl_smtp：生命周期 3 + 成员 24，行区间 473..499
INT g_CurlSmtpCmdIndex[] = { 473, 474, 475, 476, 477, 478, 479, 480, 481, 482, 483, 484, 485, 486, 487, 488, 489, 490, 491, 492, 493, 494, 495, 496, 497, 498, 499 };
static_assert(sizeof(g_CurlSmtpCmdIndex) / sizeof(INT) == 27, "curl_smtp 成员命令条数变了");
static_assert(CX_TYPE_CURL_SMTP == 13, "curl_smtp 数据类型序号与登记顺序不一致");
// curl_multi：生命周期 3 + 成员 10，行区间 500..512
INT g_CurlMultiCmdIndex[] = { 500, 501, 502, 503, 504, 505, 506, 507, 508, 509, 510, 511, 512 };
static_assert(sizeof(g_CurlMultiCmdIndex) / sizeof(INT) == 13, "curl_multi 成员命令条数变了");
static_assert(CX_TYPE_CURL_MULTI == 14, "curl_multi 数据类型序号与登记顺序不一致");
// URL参数类：生命周期 3 + 成员 8，行区间 513..523
INT g_UrlParamCmdIndex[] = { 513, 514, 515, 516, 517, 518, 519, 520, 521, 522, 523 };
static_assert(sizeof(g_UrlParamCmdIndex) / sizeof(INT) == 11, "URL参数类 成员命令条数变了");
static_assert(CX_TYPE_URL_PARAM == 15, "URL参数类 数据类型序号与登记顺序不一致");
// HTTP访问_Curl访问类：生命周期 3 + 成员 11，行区间 524..537
INT g_AccessCurlCmdIndex[] = { 524, 525, 526, 527, 528, 529, 530, 531, 532, 533, 534, 535, 536, 537, 613, 614, 615, 651, 652 };
static_assert(sizeof(g_AccessCurlCmdIndex) / sizeof(INT) == 19, "HTTP访问_Curl访问类 成员命令条数变了");
static_assert(CX_TYPE_ACC_CURL == 16, "HTTP访问_Curl访问类 数据类型序号与登记顺序不一致");
// HTTP访问_WinHTTP访问类：生命周期 3 + 成员 6，行区间 538..546
INT g_AccessWinHttpCmdIndex[] = { 538, 539, 540, 541, 542, 543, 544, 545, 546, 616, 617, 618, 653, 654 };
static_assert(sizeof(g_AccessWinHttpCmdIndex) / sizeof(INT) == 14, "HTTP访问_WinHTTP访问类 成员命令条数变了");
static_assert(CX_TYPE_ACC_WINHTTP == 17, "HTTP访问_WinHTTP访问类 数据类型序号与登记顺序不一致");
// HTTP访问_WinInet访问类：生命周期 3 + 成员 5，行区间 547..554
INT g_AccessWinInetCmdIndex[] = { 547, 548, 549, 550, 551, 552, 553, 554, 619, 620, 621, 655, 656 };
static_assert(sizeof(g_AccessWinInetCmdIndex) / sizeof(INT) == 13, "HTTP访问_WinInet访问类 成员命令条数变了");
static_assert(CX_TYPE_ACC_WININET == 18, "HTTP访问_WinInet访问类 数据类型序号与登记顺序不一致");
// 行基址校验：_行基址 必须等于本族之前的命令表行数；本族末尾必须正好是 g_commands 末尾。
// （数组元素不是常量表达式，起始行只能靠这条总长断言间接锁定。）
static_assert(sizeof(g_commands) / sizeof(CMD_INFO) >= 577, "HTTP访问 族的 _行基址 超过了 g_commands 长度");

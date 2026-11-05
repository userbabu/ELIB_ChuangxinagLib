# zyJson2.51 输出 / 类型强转 / 变更操作 行为规格（spec-output）

目标：C++ 替换库达到「相同输入 → 相同输出（字节级一致）」验收线。本文件只覆盖**输出序列化、类型强转、变更（mutate）操作**；值模型/解析器/路径语法另有人负责，仅在输出依赖处引用。

文件缩写（行号 = 导出 txt 的物理行号）：
- `J` = `源码txt\代码\程序集_Json.static.e.txt`
- `Z` = `源码txt\代码\程序集_zyString.static.e.txt`
- `C` = `源码txt\代码\Json2.class.e.txt`
- `A` = `源码txt\代码\程序集_Array.static.e.txt`
- `H` = `源码txt\代码\程序集_HashTableEx.static.e.txt`
- `hp` = `源码txt\代码\程序集_help.static.e.txt`
- `K` = `源码txt\常量.e.txt`
- `D` = `源码txt\DLL.e.txt`
- `M` = `源码txt\模块\编码转换通用模块\代码\__HIDDEN_TEMP_MOD__.static.e.txt`
- `S` = `源码txt\代码\程序集_启动.static.e.txt`

类型常量（K:80-87）：`JVTYPE_UNDEFINED=-1, NULL=0, DOUBLE=1, LONG=2, BOOL=3, STRING=4, ARRAY=5, OBJECT=6`。
所有「W」字节集均为 UTF-16LE 且带 `{0,0}` 结尾；文本型为 ANSI/cp936（见 api-spec.md F 节，此处不重复论证）。

---

## 1. 序列化（到文本 / 到文本W → JsonValue_ToStringA/W + zyString）

### 1.0 入口与参数默认

C:308-322（到文本W）与 C:325-339（到文本）：

```
.如果真 (是否为空 (参数_是否转义))
    参数_是否转义 ＝ 真
.如果真结束
.如果真 (参数_初始缓存大小 ≤ 0)
    参数_初始缓存大小 ＝ 1024
.如果真结束
jv ＝ JsonValue_SelectNoCreateW (_取指针对象 (m_object), 参数_路径)      ' W 版；A 版先 zyA2W (参数_路径)
返回 (JsonValue_ToStringW (jv, 参数_格式化, 参数_是否转义, 参数_初始缓存大小))
```

- 路径用 **NoCreate** 选择：路径不存在 → `jv=0` → `JsonValue_ToStringW/A` 直接返回 `{0,0}` / `“”`（J:2169-2171, J:2187-2189）。**序列化任何路径都不改动树**。
- `参数_格式化`（autoFormat）可空时原样传递（EPL 空逻辑=假）。`参数_是否转义`（isTrans）默认**真**。
- `参数_初始缓存大小` 只影响 zyString 缓冲区容量（J:2172 → Z:37-46），**不影响输出内容**。

J:2160-2194：

```
.子程序 JsonValue_ToStringW, 字节集
    ...
    .如果真 (jsonValue ＝ 0)
        返回 ({ 0, 0 })
    .如果真结束
    zyString ＝ zyString_Create (initBuffSize)
    JsonValue_ToStringProc (jsonValue, zyString, autoFormat, indent, isTrans)
    ret ＝ zyString_GetStrW (zyString)          ' UTF-16LE + {0,0}，长度 = strLen×2+2（Z:236）
    zyThisCall (zyString, #zy_Release)
    返回 (ret)
...
.子程序 JsonValue_ToStringA, 文本型
    ...
    ret ＝ zyString_GetStr (zyString)           ' → zyW2A_Point(pStr, CP_936)（Z:220）
```

### 1.1 核心递归 JsonValue_ToStringProc（J:2196-2294，完整转录）

```
.子程序 JsonValue_ToStringProc
    ...
    type ＝ _Get (jsonValue, #jv_type)
    .判断开始 (type ＝ #JVTYPE_NULL)
        zyString_AddW (zyString, { 110, 0, 117, 0, 108, 0, 108, 0, 0, 0 })          ' null
    .判断 (type ＝ #JVTYPE_BOOL)
        zyString_AddW (zyString, 选择 (JsonValue_GetBool (jsonValue), { 116, 0, 114, 0, 117, 0, 101, 0, 0, 0 }, { 102, 0, 97, 0, 108, 0, 115, 0, 101, 0, 0, 0 }))   ' true / false
    .判断 (type ＝ #JVTYPE_DOUBLE)
        zyString_AddW (zyString, JsonValue_GetRawW (jsonValue))
    .判断 (type ＝ #JVTYPE_LONG)
        zyString_AddW (zyString, JsonValue_GetRawW (jsonValue))
    .判断 (type ＝ #JVTYPE_UNDEFINED)
        zyString_AddW (zyString, JsonValue_GetRawW (jsonValue))
    .判断 (type ＝ #JVTYPE_STRING)
        zyString_AddChar (zyString, 34) ' 引号
        .如果 (isTrans)
            JsonValue_TranslationOut (JsonValue_GetString (jsonValue), value)
        .否则
            value ＝ JsonValue_GetStringW (jsonValue)
        .如果结束
        zyString_AddW (zyString, value)
        zyString_AddChar (zyString, 34) ' 引号
    .判断 (type ＝ #JVTYPE_ARRAY)
        val ＝ _Get (jsonValue, #jv_value)
        .如果真 (val ≠ 0)
            zyString_AddChar (zyString, 91) ' [
            count ＝ Array_GetValues (val, values)
            indent ＝ indent ＋ 1
            .计次循环首 (count, i)
                JsonValue_ToStringProc (values [i], zyString, autoFormat, indent, isTrans)
                .如果真 (i ≠ count)
                    zyString_AddChar (zyString, 44) ' 逗号
                .如果真结束
            .计次循环尾 ()
            indent ＝ indent － 1
            zyString_AddChar (zyString, 93) ' ]
        .如果真结束
    .判断 (type ＝ #JVTYPE_OBJECT)
        val ＝ _Get (jsonValue, #jv_value)
        .如果真 (val ≠ 0)
            zyString_AddChar (zyString, 123) ' {
            .如果真 (autoFormat)
                zyString_AddW (zyString, { 13, 0, 10, 0, 0, 0 }) ' 换行符
            .如果真结束
            count ＝ HashEx_GetKeys (val, names)
            indent ＝ indent ＋ 1
            .计次循环首 (count, i)
                .如果真 (autoFormat 且 indent ＞ 0)
                    zyString_AddW (zyString, 取重复字节集 (indent, { 9, 0 }) ＋ { 0, 0 })
                .如果真结束
                zyString_AddChar (zyString, 34) ' 引号
                .如果 (isTrans)
                    JsonValue_TranslationOut (_取指针字节集 (names [i]), value)
                    zyString_AddW (zyString, value)
                .否则
                    zyString_AddW (zyString, names [i])
                .如果结束
                zyString_AddChar (zyString, 34) ' 引号
                zyString_AddChar (zyString, 58) ' :
                JsonValue_ToStringProc (HashEx_Get (val, names [i]), zyString, autoFormat, indent, isTrans)
                .如果真 (i ≠ count)
                    zyString_AddChar (zyString, 44) ' 逗号
                .如果真结束
                .如果真 (autoFormat)
                    zyString_AddW (zyString, { 13, 0, 10, 0, 0, 0 }) ' 换行符
                .如果真结束
            .计次循环尾 ()
            indent ＝ indent － 1
            .如果真 (autoFormat 且 indent ＞ 0)
                zyString_AddW (zyString, 取重复字节集 (indent, { 9, 0 }) ＋ { 0, 0 })
            .如果真结束
            zyString_AddChar (zyString, 125) ' }
        .如果真结束
    .默认
        
    .判断结束
```

**规则（逐类型）**：

| 存储类型 | 输出 |
|---|---|
| NULL (0) | 固定 4 字符 `null` |
| BOOL (3) | `true`/`false`（经 GetBool，即 `value≠0` 判定） |
| DOUBLE (1) / LONG (2) / UNDEFINED (-1) | **原样输出 jv_raw（UTF-16）**。raw=0（被清空后）→ 输出**空串**（GetRawW 返回 `{0,0}`，AddW 长度 0，J:567-578 + Z:193）。不重新格式化数值！ |
| STRING (4) | `"` + 内容 + `"`。isTrans=真 → 经 TranslationOut 转义；isTrans=假 → **存储的宽字符原样**输出（引号/反斜杠不转义，可能产生非法 JSON） |
| ARRAY (5) | `[elem,elem]`。**数组分支完全无视 autoFormat：数组永远单行紧凑、无换行/缩进、逗号后无空格**。空数组 → `[]`（Array_Create 后 count=0 时 val≠0） |
| OBJECT (6) | 见下格式化规则。空对象紧凑 → `{}`；空对象+autoFormat → `{\r\n}`（换行仍会加，见 1.3） |
| 其它类型值（不可能） | 默认分支：什么都不输出 |

**属性顺序** = HashEx 的 insertion-order 数组 `#htEx_eArray` 的顺序（H:617-622 `HashEx_GetKeys` 按 eArray 下标 0..count-1 输出）。即**插入顺序**；同名键被覆盖时保持原位置（H:243 `Array_ReplaceValue (array, pe, value)`）。这与按字典序或哈希序输出的其它库不同，是对拍第一陷阱。

### 1.2 输出转义（JsonValue_TranslationOut，J:3145-3220）

```
.子程序 JsonValue_TranslationOut, 逻辑型
    ...
    .如果真 (value ＝ 0)
        outValue ＝ { 0, 0 }
        返回 (假)
    .如果真结束
    strLen ＝ lstrlenW_Ptr (value)
    buffLen ＝ strLen × 6 ＋ 2
    outValue ＝ 取空白字节集 (buffLen)
    pBuff ＝ _取指针字节集 (outValue)
    RtlMoveMemory_int (pBuff, value, strLen × 2)
    wchar ＝ _指针到短整数U (pBuff ＋ i × 2)
    transChar ＝ 92 ' \
    .判断循环首 (wchar ≠ 0 或 i ≥ buffLen)
        .判断开始 (wchar ＝ 92 或 wchar ＝ 47 或 wchar ＝ 34) ' \ / "
            pos ＝ pBuff ＋ i × 2
            RtlMoveMemory_int (pos ＋ 2, pos, buffLen － i × 2 － 2)
            RtlMoveMemory_int (pos, _取变量地址 (transChar), 2)
            i ＝ i ＋ 1
        .判断 (wchar ＝ 8)   → 插 `\b`  (str ＝ { 92, 0, 98, 0 })
        .判断 (wchar ＝ 9)   → 插 `\t`
        .判断 (wchar ＝ 10)  → 插 `\n`
        .判断 (wchar ＝ 13)  → 插 `\r`
        .判断 (wchar ＝ 12)  → 插 `\f`
        .默认
        .判断结束
        i ＝ i ＋ 1
        wchar ＝ _指针到短整数U (pBuff ＋ i × 2)
    .判断循环尾 ()
    返回 (真)
```

转义集合**只有 8 个字符**：
- `\`(92) → `\\`；`/`(47) → **`\/`（斜杠总是被转义！）**；`"`(34) → `\"`
- 8→`\b`、9→`\t`、10→`\n`、13→`\r`、12→`\f`
- 其它 <0x20 控制字符：**原样输出宽字符**（不转成 `\u00XX`）。
- 非 ASCII：**永远以 UTF-16 原样输出，绝不转 `\uXXXX`**（全函数无 0x7F 以上分支；`zyToUnicodeString`（hp:691）存在但序列化路径不调用）。
- 孤立代理对、BMP 外字符（代理对）原样透传。
- 属性名与字符串值走同一函数（J:2267 对 names 也 TranslationOut）。
- 注意 value=0（空串被清空后）→ outValue `{0,0}`，即引号之间什么都没有，输出 `""`。

**输入侧对输出的影响（round-trip 关键，J:3014-3106 TranslationIn）**：解析时 `\n` 若前一个字符不是 `\r` 会被替换成 **`\r\n` 两个字符存储**（J:3061-3066）：

```
.判断 (nextChar ＝ 110) ' \n
    .如果真 (pos ＞ value 且 _指针到短整数U (pos － 2) ≠ 13)
        preChar ＝ 13
    .如果真结束
    newChar ＝ 10
    find ＝ 真
```

于是 JSON 文本 `"a\nb"` 解析后再 到文本 → `"a\r\nb"`（`\r` `\n` 都再被 TranslationOut 转义）。`\/`、`\"`、`\\` 解码时吃掉反斜杠（J:3048-3054），输出时 `"` `\\` `/` 再各自加回——`/` 变成 `\/`，**与源文本不同但确定**。`transU=真`（置代码/合并默认，见 C:66/1400/1429）时 `\uXXXX` 解码为宽字符再存储；未知转义（如 `\x41`、`\'`）保持反斜杠原样存储（find=假、无分支命中），输出时反斜杠再被转义 → `\\x41`。

### 1.3 格式化（autoFormat=真）的确切字符

- 换行 = **`\r\n`**（`{ 13, 0, 10, 0 }`，J:2256/2281）。
- 缩进 = **Tab 字符（U+0009），个数=当前深度**（`取重复字节集 (indent, { 9, 0 })`，J:2263/2287）；根对象的键行 1 个 Tab。
- `:` 前后**无空格**；`,` 后**无空格**。
- 对象结构：`{` `\r\n` [每属性：Tab×indent `"key"`（转义后） `:` value `,`? `\r\n`] [关闭行：Tab×(indent-1)（仅当 >0）] `}`。
- 空对象：紧凑 `{}`；格式化 → `{\r\n}`（无缩进关闭行；嵌套在 depth≥1 时空对象为 `{\r\n\t…}`）。
- 空数组：任何模式 `[]`。
- **数组内不插换行**：数组元素是对象时，只有元素自身对象分支产生 `\r\n`+Tab；`[`、`]`、元素间 `,` 全部贴紧。
- **结尾无额外换行**：整个输出以 `}`/`]` 结束，zyString 尾部只是 `{0,0}` 终止符。

**工作示例**（依代码推演）。树：`{"name":"te\"st/行","arr":[1,2],"obj":{"k":"v"}}`，插入顺序即此序；`到文本(, 真, 真)`（格式化+转义，UTF-16 显示为文本）：

```
{
	"name":"te\"st\/行",
	"arr":[1,2],
	"obj":{
		"k":"v"
	}
}
```

即精确字符序列：`{` CR LF TAB `"name":` … `,` CR LF TAB `"arr":[1,2],` CR LF TAB `"obj":{` CR LF TAB TAB `"k":"v"` CR LF TAB `}` CR LF `}`。紧凑模式（格式化=假）：`{"name":"te\"st\/行","arr":[1,2],"obj":{"k":"v"}}`。

### 1.4 wide→ANSI / UTF-8 尾步骤

- `到文本`（A 版）：`zyString_GetStr(zyString)`（Z:204-221，未传 codePage → 默认 **CP_936**）→ `zyW2A_Point(pStr, 936)`（hp:57-72）：`WideCharToMultiByte_ptr(936, 0, p, -1, …, 0, 0, 0)` —— lpDefaultChar=NULL → **不可编码字符落到系统默认字符（cp936 下为 `'?'` 单字节）**，返回文本长度含尾部一个 NUL 字节（EPL 文本按 C 串比较，尾部 NUL 不可见；C++ 侧按不含尾部 NUL 的 GBK 字节实现即可）。
- `取文本UTF8`（C:1108-1127）＝ `W2UTF8_T(取文本W(...))`；`W2UTF8_T` 来自外部易模块「创想编码通用模块」（M:62-63，**无源码可考**，仅签名 `文本型 ← 字节集(末尾带0,0)`）。合理推断为 `WideCharToMultiByte(CP_UTF8)`：合法 UTF-16 → 标准 UTF-8；**孤立代理项**的 fallback 行为未证实（`?` 或 `EF BF BD`），需用现有 EPL 运行库 golden test 钉死（候选解释①Win32 默认 `?`；候选②U+FFFD）。
- `置代码UTF8`/`K_UTF8`/`置文本UTF8` 的 `UTF82W_T`（M:67-68）同理：非法 UTF-8 字节的映射未证实。

---

## 2. 数字的输出格式（raw 的来源决定字节）

序列化对 DOUBLE/LONG **从不重新格式化**，只吐出 `#jv_raw`（J:2215-2218）。字节是否一致完全取决于 raw 是什么时候、由什么调用生成的。raw 的产生点：

1. **解析路径（首选语义：源文本切片原样保留）**。J:1980-2001（jsonValue_parsed_other 尾部）：
   - DOUBLE：`jv ＝ JsonValue_Double (jsonVal, _StrToLongW→(_StrToDoubleW (value), value))` —— 原文 `JsonValue_Double (jsonVal, _StrToDoubleW (value), value)`（J:1988），raw = 源切片（含 `1.50`、`+5`、`-0`、`1E5` 等原形）。
   - **注意陷阱**：LONG 的非根子节点走的是 `JsonValue_Double`（J:2000）：
     ```
     .判断 (childType ＝ #JVTYPE_LONG)
         .如果 (jvParent ＝ 0)
             .如果真 (JsonValue_SetLong (jsonVal, _StrToLongW (value), value))
         .否则
             jv ＝ JsonValue_Double (jsonVal, _StrToLongW (value), value)
     ```
     → **解析出的整数子成员存储类型是 DOUBLE(1)**（int64→double 转换，>2^53 精度丢失），只有根级整数是 LONG(2)；两者 raw 都是源切片所以序列化一致，但 `取类型`/强转行为不同。
2. **程序化 setter / 工厂（EPL `到文本` 格式化，无 rawData 参数时）**：
   - DOUBLE（J:281-286 工厂，J:332-337 `JsonValue_SetDouble`；置双精度/置数值 走后者）：
     ```
     .如果 (rawData ≠ { })
         _Set (this, #jv_raw, _CloneString (rawData, hHeap))
     .否则
         raw ＝ zyA2W (到文本 (double))
         _Set (this, #jv_raw, _CloneString (raw, hHeap))
     ```
   - LONG（J:362-367 / J:413-418）：`raw ＝ zyA2W (到文本 (Long))`；INT（J:463-468）：`raw ＝ zyA2W (到文本 (Int))`（存成 LONG）。
   - BOOL：固定 UTF-16 `true`/`false` 字节（J:489/527）；NULL：固定 `null`（J:239/258）。
3. **克隆会重格式化 DOUBLE/LONG**（`JsonValue_Clone`，J:2949-2952）：
   ```
   .判断 (type ＝ #JVTYPE_DOUBLE)
       dscJV ＝ JsonValue_Double (jsonValue, _GetDouble (jsonValue, #jv_value))
   .判断 (type ＝ #JVTYPE_LONG)
       dscJV ＝ JsonValue_Long (jsonValue, _GetLong (jsonValue, #jv_value))
   ```
   未传 rawData → raw 改由 `到文本(数值)` 重新生成，**源文本格式（如 `1.50`）丢失**。置属性/添加成员/插入成员/合并/复制 全部先 Clone → 这些路径写入的 double/long 输出取决于 EPL 的 double→text 规则。

**`到文本 (双精度小数型)` 的运行时规则**：源码仅调用 krnln 命令 `到文本`（K/D 无涉及，D 节 G 清单列出 krnln `到文本`），CHM 帮助（`文本型 到文本 （ 待转换的数据）`）未说明精度/指数阈值/`-0`/整值是否带尾点。**歧义 —— 必须在 EPL 5.x 运行时用 golden test 钉死**，候选解释：
- ① 15 位有效数字、去尾零、无固定小数点（`1.5→"1.5"`，`3.0→"3"` 或 `"3."`）；
- ② 超阈值转科学计数（`1e21`→`"1E+21"` 样式）；
- ③ 负零、下溢值（`1e-320`）输出形式未知。
`到文本 (长整数型/整数型)` 则是普通十进制带符号（无千分位、无前导零），此点无歧义。
`_StrToDoubleW`（读侧，hp:578-649 手写算法整段被注释，实际生效为）`返回 (到数值 (zyW2A (str)))`（hp:649）；`_StrToLongW` 实际生效 `返回 (_wtoi64 (str))`（hp:684，ntdll `@_wtoi64`，D:253-257）。它们影响 取数值/取长整数 而非输出字节。

**结论（byte-identical 判据）**：
- 纯解析→到文本：数字输出 = 源切片，逐字节等于输入中的原形（含 `+5`、`1.500`、`-0`）。
- 经 setter/clone/merge/置属性 的数字：输出 = `到文本(数值)` 的 ANSI 结果再 UTF-16 化（zyA2W CP936，数字恒 ASCII）→ C++ 必须复刻 EPL 的 double→text（只能实测标定，建议直接测 zyJson2.51 现产物：`置双精度(x); 到文本()`）。

---

## 3. 取源数据 / 取源数据W

C:1052-1066：`SelectNoCreateW` → `JsonValue_GetRawA/W`。

J:567-592：

```
.子程序 JsonValue_GetRawW, 字节集, , 获取元数据，不支持 文本、数组和对象
    .如果真 (jsonValue ＝ 0)
        返回 ({ 0, 0 })
    .如果真结束
    raw ＝ _Get (jsonValue, #jv_raw)
    .如果真 (raw ≠ 0)
        返回 (指针到字节集 (raw, lstrlenW_Ptr (raw) × 2 ＋ 2))
    .如果真结束
    返回 ({ 0, 0 })

.子程序 JsonValue_GetRawA, 文本型
    ' jsonValue＝0 或 raw＝0 → “”；否则 zyW2A_Point (raw)（CP936，不可编码→'?'，尾部含 NUL）
```

按类型返回的 raw：
- NULL → `"null"`；BOOL → `"true"`/`"false"`（固定字面量，即使原 JSON 大小写不同也不保留——解析时经 JsonValue_Bool 重建）。
- DOUBLE/LONG → 解析=源切片原文；程序化 setter/Clone=`到文本`结果（见第 2 节）。
- UNDEFINED → 创建时给定的 raw（通常仅出现在 Clone 链）。
- **STRING/ARRAY/OBJECT → `{0,0}`/`“”`**：解析器从不给这三类写 raw（字符串值以解码后的文本指针存 `#jv_value`，容器存子结构；见 J:594-654 的 JsonValue_String* 不触碰 #jv_raw，以及 jsonValue_parsed_object/array/other 中容器与字符串分支）。排除原因（注释原文 J:567「获取元数据，不支持 文本、数组和对象」）：字符串的源切片含引号与转义未解码形态、容器切片过大且已建模为子树。
- 路径不存在 / 未初始化 → `{0,0}`/`“”`。注意 raw 会被 `JsonValue_Clear`/`JsonValue_Destory` 释放置 0（J:76-80, J:130-134）——清空后 取源数据 变空。

---

## 4. 强转矩阵（getter × 存储类型）

选择均为 `JsonValue_SelectNoCreateW`（J:2487-2537）：路径不存在→`jv=0`；**越界数组下标**→`JsonValue_GetAtReadOnly`（J:950-982）不补齐、返回 0；对象上不存在的键不新建（区别于 K()/I() 的自动创建，见第 5 节）。`不存在`/`越界` 与 `jv=0` 等价。

### 4.1 底层实现转录

GetStringExW（J:702-722；ExA J:725-745 同构，尾部 `zyW2A_Point(pStr)`）：

```
type ＝ _Get (jsonVal, #jv_type)
.判断开始 (type ＝ #JVTYPE_STRING)
    pStr ＝ _Get (jsonVal, #jv_value)
.判断 (type ＝ #JVTYPE_NULL 或 type ＝ #JVTYPE_BOOL 或 type ＝ #JVTYPE_LONG 或 type ＝ #JVTYPE_DOUBLE)
    pStr ＝ _Get (jsonVal, #jv_raw)
.默认
    pStr ＝ 0
.判断结束
.如果真 (pStr ＝ 0)
    返回 ({ 0, 0 })
.如果真结束
返回 (指针到字节集 (pStr, lstrlenW_Ptr (pStr) × 2 ＋ 2))
```

GetBool（J:495-512）：

```
.判断开始 (type ＝ #JVTYPE_LONG)
    返回 (_Get (jsonVal, #jv_value) ≠ 0)          ' _Get 只读低 32 位！
.判断 (type ＝ #JVTYPE_DOUBLE)
    返回 (_GetDouble (jsonVal, #jv_value) ≠ 0)
.判断 (type ＝ #JVTYPE_BOOL)
    返回 (_Get (jsonVal, #jv_value) ≠ 0)
.默认
返回 (假)                                        ' STRING/NULL/ARRAY/OBJECT/UNDEFINED/jv=0 → 假
```

GetDouble（J:291-315）：DOUBLE→`_GetDouble`；LONG→`到数值 (_GetLong(...))`；BOOL→`_Get(value)`（0/1→double）；STRING→`v≠0` 时 `到数值 (zyW2A_Point (v))`（**先转 ANSI 再解析**）；其余/失败→0。
GetLong（J:372-396）：LONG→`_GetLong`；DOUBLE→`到长整数 (_GetDouble(...))`（**舍入模式为 EPL 运行时行为，歧义：截断 or 四舍五入，需 golden test**）；BOOL→`_Get(value)`；STRING→v≠0 时 `_wtoi64_ptr (v)`（跳空白、可带符号，非数字即停，失败 0）；其余→0。
GetInt（J:422-446）：LONG→**`_Get (jsonVal, #jv_value)` = 只取低 32 位**（高位丢弃、无回绕检查，如 2^32 → 0）；DOUBLE→`到整数 (_GetDouble(...))`；BOOL→`_Get(value)`；STRING→v≠0 时 `StrToInt (v)`（J:440；`StrToInt` 实为 `shlwapi!StrToIntW`，D:213-214。**MSDN 定义 StrToInt 返回 BOOL 而非数值** → 候选解释①成功返回 1/失败 0（bug 行为，须实测钉死）；候选解释②旧版直接返回数值。golden test 必做）；其余→0。
GetType（J:1345-1351）：jv=0 → **-1（JVTYPE_UNDEFINED）**；否则 `#jv_type` 原值。

到文本/取文本 的容器回退（C:1069-1105 / C:1129-1167）：

```
' 取文本（A）：
retun ＝ JsonValue_GetStringExA (jv)
.如果真 (retun ＝ “”)                              ' 失败则获取JSON源码
    …参数默认（缓存 1024、转义 真）…
    retun ＝ JsonValue_ToStringA (jv, 参数_格式化, 参数_是否转义, 参数_初始缓存大小)
    .如果真 (retun ＝ #引号 ＋ #引号)  retun ＝ “”   ' "" → 空
    .如果真 (retun ＝ “{}”)          retun ＝ “”
    .如果真 (retun ＝ “[]”)          retun ＝ “”
.如果真结束
.如果真 (retun ＝ “null”)           retun ＝ “”     ' 任何来源的 "null" 一律变空
返回 (retun)
' 取文本W 同构：{ 0,0 }／{ } → ToStringW；`"" {}[] null` 的 UTF-16 字节组比较见 C:1150-1165
```

### 4.2 矩阵（行为值；「序列化」= 第 1 节全量 JSON 文本）

| 存储类型 | 取文本/取文本W(/UTF8) | 取逻辑(W) | 取双精度/取数值(W) | 取长整数(W) | 取整数(W) | 取类型(W) |
|---|---|---|---|---|---|---|
| 不存在/越界 (jv=0) | `“”`/`{0,0}`（ToString(0)） | 假 | 0 | 0 | 0 | **-1** |
| UNDEFINED(-1) | raw（一般空）→ `{0,0}` | 假 | 0 | 0 | 0 | -1 |
| NULL(0) | raw=`null` → **被特判成 “”** | 假 | 0 | 0 | 0 | 0 |
| DOUBLE(1) | raw（源文本原样，含 `1.50`/`-0`） | ≠0 判定 | 原值 | `到长整数(raw值)` 舍入歧义 | `到整数` | 1（**解析出的整数子成员也是 1**，见 §2） |
| LONG(2) | raw | **仅低 32 位 ≠0**（陷阱：2^32 → 假） | 值→double | 原值 | 低 32 位 | 2 |
| BOOL(3) | `true`/`false` | 真/假 | 1/0 | 1/0 | 1/0 | 3 |
| STRING(4) | **文本内容本身（不 JSON 化、不转义）**；空串→`{0,0}`；**内容恰为 `null` 的字符串被返回空**（C:1101/1163 无类型判定） | 假 | `到数值(zyW2A(串))` 按 ANSI 解析 | `_wtoi64` | `StrToIntW`（候选 1/0） | 4 |
| ARRAY(5) | 序列化整棵子树；空数组→`“”`/`{0,0}`；随后第 1 节转义规则 | 假 | 0 | 0 | 0 | 5 |
| OBJECT(6) | 序列化整棵子树；空对象→`“”`/`{0,0}` | 假 | 0 | 0 | 0 | 6 |

- `到文本`/`到文本W`：无「null/空容器→空」特判，NULL 永远 `null`、空容器永远 `null`? 不——空容器输出 `{}`/`[]` 原样保留；只有 取文本 系做特判。
- `取文本UTF8` = UTF-8(W2UTF8_T) of 取文本W（C:1127），路径参数本身就是 字节集。
- ANSI 方向：所有 A 版（取文本/取属性名/取源数据/到文本）输出用 **CP_936** `WideCharToMultiByte`（zyW2A_Point 默认页 hp:66-68），不可编码→`?`；所有 A 版输入（路径/属性名/文本值）用 `zyA2W` CP_936（hp:11-13）。取文本 对字符串值**不做**任何转义/解转义，取的是内存原值。

---

## 5. Setter 语义（置属性/置成员/置文本/置空/置逻辑/置双精度/置数值/置长整数/置整数/置对象/置数组 + W/UTF8 + 数组批量）

### 5.1 公共模式

类侧全部为：`初始化 ()`（懒建根）→ `jv ＝ JsonValue_SelectW/A (_取指针对象 (m_object), 参数_路径)`（**自动创建路径**，见 5.4）→ 调底层 `JsonValue_SetX(jv, …)`。底层标量 setter 一律先 `JsonValue_Destory (jsonValue)` 再覆写 type/value/raw —— **类型不匹配 = 全量清除再替换**：子树释放（数组成员 Release / 对象 child Release + Hash 销毁）、字符串内存释放、**raw 释放**，然后设新值。文本类 setter 不写 raw（J:747-790）→ 置过的字符串 取源数据 为空。

各底层实现（文件行）：SetNull J:245-260、SetBool J:514-529、SetDouble J:318-340（rawData 省略→`到文本`）、SetLong J:399-419、SetInt J:449-469（**存成 JVTYPE_LONG**，尾注释亦称长整数型）、SetString/W/A J:747-790。

### 5.2 逐个说明

- `置文本/置文本W/置文本UTF8`（C:895-930）：目标节点（任何类型）→ STRING。W 版怪癖（C:925-927）：
  ```
  .如果真 (取字节集左边 (参数_文本值, 2) ≠ { 0, 0 })
      参数_文本值 ＝ 参数_文本值 ＋ { 0, 0 }
  ```
  判**左**两字节（应为右）：凡不以宽字符 0 开头的串都在**末尾再追加一个 `{0,0}`**——已带终止符的入参会得到两个宽 0；`_CloneString` 按 `lstrlenW` 截读（hp:244），所以**内容不受影响**，C++ 只要存「第一个 NUL 前的宽字符」即可等价。存值**不做** TranslationIn：反斜杠、`"` 原样存储（输出时才转义）。UTF8 版先 `UTF82W_T`。
- `置空(W)`：→ NULL，raw=`null`。
- `置逻辑(W)`：→ BOOL。
- `置双精度/置数值(W)`：→ DOUBLE，raw=`到文本(d)`（第 2 节）。`置数值` 与 `置双精度` 完全同实现（C:991-1008）。
- `置长整数(W)`：→ LONG；`置整数(W)`：→ LONG（值域即 int32，输出=`到文本(int)`）。
- `置对象(W)/置数组(W)`（C:858-892 → J:847-864 / J:811-826）：**当前已是该类型 → 直接返回真、成员全部保留**；否则 Destory（**清除 children/raw**）→ 新建空容器。即对已有对象调 置对象 是无操作，对标量/数组调 置对象 变成 `{}`（数组元素丢弃）。
  SetArray 已知缺陷（J:813/823）：`hHeap` 局部**从未赋值**（=0）→ `Array_Create (hHeap)` 里 `hHeap＝0` 分支 `HeapCreate` 为该数组**另开私有堆**（A:20-23）。不影响输出字节，影响内存归属。
- `置属性/置属性W`（C:450-501）→ `JsonValue_SetA/W`（J:1209-1238 / J:1180-1207）：
  ```
  .如果真 (_Get (jsonValue, #jv_type) ≠ #JVTYPE_OBJECT)
      JsonValue_SetObject (jsonValue)          ' 目标不是对象 → 强制转空对象（数组/标量的旧值被销毁）
  pMap ＝ _Get (jsonValue, #jv_value)
  old ＝ HashEx_Get (pMap, key)
  _Set (jsonVal, #jv_parent, jsonValue)
  ret ＝ HashEx_Put (pMap, key, jsonVal)        ' 新键→按插入顺序追加；同名→原位置替换（H:233-244）
  .如果真 (old ≠ 0)  zyThisCall (old, #zy_Release)   ' 旧 child 自动 Detch+销毁
  ```
  传入的 参数_值 先 **JsonValue_Clone**（C:464）——double/long 的 raw 会被 `到文本` 重格式化（§2）；若 参数_值 未初始化（取指针=0）→ 返回假。
- `置成员(W)`（C:774-824）→ `JsonValue_SetAt`（J:985-1041），**参数_索引 语义**：
  - 目标非数组 → SetArray 强制转数组（对象/标量旧值销毁；注意对象转数组后属性全丢）。
  - `index ≥ count`：**补 null 扩张**到 index 个 `JsonValue_Null` 后在 index 处放入新值（J:1010-1031）。
  - `index < count` 且旧 child 存在：先 `zyThisCall(jv, #zy_Release)`（引用归零 → OnRelease → **JsonValue_Detch 自动从数组摘除**，J:19-21/137-167）→ 再 `Array_Insert (pArray, index, jsonVal)`（此时该槽已是后续元素前移位，插入不产生重复指针）。
  - `index < 0`：不进扩张分支，`Array_Get` 越界 0 → `Array_Set` 界检查失败 → 假（克隆体被类侧 Release）。
  - 目标数组内路径 `…[k]` 中 `SelectW` 的 GetAt 同样会补 null 扩张（J:936-944）。

### 5.3 数组/对象 setter 落在不同节点上的差异

- 路径末段是**对象属性**：置属性/插入属性 生效；置成员 会先把该属性节点转成数组。
- 路径末段是**数组元素**（`a[2]`）：置成员/添加成员/插入成员 生效；置属性 会把该元素节点转成对象。
- SelectW 首步即 `[n]`（无名字）时强制 `JsonValue_SetArray (jv)`（J:2469-2476）：对**对象根**执行 `置成员(0, “[0]”, …)` 会把整棵根对象转成数组。
- 数组内成员排序/键：数组元素 `#jv_key=0`，不参与 hash。

### 5.4 自动创建路径（读副作用）

`JsonValue_GetW`（J:1076-1104）：非对象节点 → 转对象；缺键 → **插入 NULL 值并返回**。因此 `K("x")`、`选择`、`置属性` 的中间路径、`添加成员/插入成员/置成员/清空/移除*`（全部用 SelectW）都会**真实修改树**：新建的 null 会以 `null` 出现在后续 到文本。而 `取*` 系与 `到文本`、`排序`、`复制` 用 SelectNoCreateW，读不存在 → 0/空。

---

## 6. 插入 / 移除 / 分离 / 清空 / 销毁 / 复制

- **插入成员(W)**（C:655-704 → JsonValue_Insert J:891-911 → Array_Insert A:249-279）：索引 **0..count 闭区间**（`index ＜ 0 或 index ＞ asCount` → -1 → 假）；在 idx 处后移插入（`RtlMoveMemory (table＋(index+1)×4, table＋index×4, (asCount-index)×4)`）。目标非数组 → 先 SetArray 转换。入参 Clone。
- **插入属性(W)**（C:1486-1542 → JsonValue_InsertObjAttrW J:1271-1299 → HashEx_Insert H:269-333）：新键时先查扩容（rehash）再 `Array_Insert (array, idx, value)` → **idx 必须 0..成员数**，越界返回假（与参数注释一致）。**已存在同名键 → 走覆盖分支（H:297-308）：在原位置替换值、忽略 idx、返回真**。目标非对象 → 转对象。入参 Clone。
- **移除属性(W)**（C:399-414 → JsonValue_RemoveW J:2763-2787）：节点**必须已是 OBJECT**（≠ → 假，不转换！与置属性不同）；键不存在 → 假。成功：桶链摘除 + `_decPtr count` + `Array_RemoveFromValue`（顺序数组中该指针起后前移，A:392-417）+ **释放键字符串内存** + `Release(child)`（child 自动 Detch）。路径用 SelectW → 中间节点仍会被创建。
- **移除成员(W)**（C:417-432 → JsonValue_RemoveAt J:2819-2843）：必须 ARRAY；`Array_Get(index)` 越界/0 → 假；成功：`Array_Remove` 前移压缩（A:371-390）+ Release(child)。
- **添加成员(W)**（C:606-653 → JsonValue_Append J:867-886）：目标非数组 → SetArray 转换（对象/标量清空！）；`Array_Add` 尾部追加；Clone 入参；返回 `Array_Add ≠ -1`。
- **分离**（C:395-396 → JsonValue_Detch J:137-167）：无参，作用于自身。父为数组 → `Array_RemoveFromValue`；父为对象 → `HashEx_RemoveFromValue`（H:549-595，**HeapFree 键字符串**，但**不**清 child 的 `#jv_key`——分离后调 取属性名 读悬垂指针，UAF，移植时建议置 0 或在 C++ 保留 key 拷贝）。父=0 → 无操作。节点本体与子树保留、`#jv_parent` 清 0。返回值：无。
- **清空(W)**（C:435-448 → JsonValue_Clear J:83-134）：路径用 SelectW/A（会创建）。按类型：STRING → 释放值、`#jv_value=0`（**类型仍为 STRING** → 输出 `""`）；OBJECT → 逐个 Release child + `HashEx_Clear`（保容器、顺序数组清零 → 输出 `{}`）；ARRAY → Release 全部 + `Array_Clear` → 输出 `[]`；其它 → `#jv_value=0`（DOUBLE 归 0 但输出走 raw）。**所有类型都释放 raw 置 0**（J:130-134）→ 清空后数字再 Clone 会重格式化，取源数据变空。**类型不变**是清空与置X的核心区别。
- **销毁**（C:149-150）：`m_object.清除 ()` → 引用归零 → `JsonValue_OnRelease`（J:15-24）：Detch（若挂在树上则从父中摘除）→ Destory（递归释放子树/raw）→ DeleteObject。已知缺陷：`集_是否已初始化` 不复位（api-spec H-12）。
- **复制(W)**（C:1315-1344 → JsonValue_Clone J:2928-2978）：**深拷贝**（数组逐元素递归 Clone+Append；对象逐 child `JsonValue_Set (dscJV, _Get (items [i], #jv_key), JsonValue_Clone (items [i]))` 重建哈希+顺序）。**拷贝不带父**（新节点 parent=0，独立成根）；**属性名不随 Clone 保留在节点上**（jv_key 只在被放进新对象时由 HashEx_Put 重设；游离的 Clone 结果 取属性名 为空/0）。DOUBLE/LONG 重走 `到文本`（§2 陷阱）。返回的 Json2 `设置是否为顶层 (假)`。路径不存在 → jv=0 → 返回未初始化的空 Json2（取指针 0）。SelectW 会先创建路径（副作用）。

---

## 7. 合并族

### 7.1 合并 / 合并W（JSON 文本并入自身）

C:1391-1443：

```
参数_USC2解码 ＝ 选择 (是否为空 (参数_USC2解码), 真, 参数_USC2解码)     ' 实际默认 真（与尾注释矛盾，H-1）
jv ＝ JsonValue_SelectW/A (根, 参数_路径)
结果 ＝ JsonValue_Combine (jv, ptr(文本), 参数_USC2解码, 参数_是否添加)
.如果真 (结果 ＝ 假)                                     ' 其他方法
    .如果真 (json_.置代码(W) (参数_被合并json文本))
        结果 ＝ 合并对象 (json_)
    .如果真结束
返回 (结果)
```

`置代码`/`置代码W` 在 `集_是否为顶层＝假`（子对象上调用）时：`清空 ()` + `合并(W)(文本, , USC2, 真)` —— 隐藏的末位**真**即 isAdd（C:62、C:130），语义 =「缺失属性要添加」，等价于先清空再全量并入。

JsonValue_Combine（J:3222-3238）：新建临时 OBJECT → `JsonValue_Parsed3` 解析文本 → 失败/内存错误 Release 返回假 → `JsonValue_CombineProc (self, desc, isAdd)`。

**递归算法（由 J:3239-3308 直译的伪码）**：

```
CombineProc(srcNode, descNode, isAdd):
    if type(srcNode)==OBJECT and type(descNode)==OBJECT:
        for child in descNode.按插入顺序的children:
            key = child.jv_key
            if srcNode.含key:
                srcChild = srcNode.Get(key)          # 存在才进
                copy = Clone(child)                  # double/long 会被重格式化!
                if not CombineProc(srcChild, copy, isAdd): return false
            elif isAdd:
                copy = Clone(child)
                if not srcNode.Set(key, copy): return false
        return ret        # 全程无成功操作时 ret=假（初值）！
    else:
        key = srcNode.jv_key; parent = srcNode.parent
        if key != 0 and parent != 0:
            return parent.Set(key, descNode)          # 整体替换：descNode 所有权移交（不 Clone）
        Release(descNode); return false               # 根级/游离节点无法替换
```

要点/边界：
- 只有「双方都是对象」才逐键递归（**任意深度**）；其余类型组合一律**整值替换**（数组不逐元素合并，直接换）。
- 替换分支用的是 **srcNode 的 key+parent**：合并落在某命名属性节点上（路径指向子对象/标量）时能替换成功；路径=根且顶层类型不匹配（如把 `[1,2]` 或 `5` 合并进根对象）→ CombineProc 假 → 类侧兜底 `置代码+合并对象`（对象文本→浅覆盖成功；非标量对象文本→假）。
- **返回假但已改动**：ret 只在最后一次成功操作时被置真；desc 空对象 `{}`、或 isAdd=假且无同名键时 CombineProc 返回假 → 触发兜底 合并对象 → **把 desc 的全部属性无条件 Set 进目标**（覆盖 isAdd=假 的本意，最终返回真）。移植须复刻这条兜底副作用才能字节一致。
- 同键不同标量类型：src=1, desc="s" → 走替换分支 → 覆盖为文本值。
- 合并对目标路径节点非对象且是命名属性时：连父一起换掉整个节点。

### 7.2 合并对象 vs 合并对象Plus vs 合并数组

- **合并对象**（C:1462-1472 → JsonValue_CombineObject J:3317-3369）：源必须 OBJECT（否则假）；目标非 OBJECT → SetObject 强制转换（**目标原内容清空**）。逐源 child（源插入顺序）：`key＝_Get(源child,#jv_key)` → **Clone** → `JsonValue_Set(目标,key,clone)`。**浅一层：不递归合并同名子对象，子对象整体覆盖**；同名键覆盖、新键**追加到顺序末尾**。数组源 → 假（Plus 才转投 合并数组）。
- **合并对象Plus**（C:1445-1460）：`CombineObject` 失败再试 `CombineArray`。即：源对象→按对象覆盖合并；源数组（或目标被转换场景后失败）→数组拼接；两者都非 → 假。
- **合并数组**（C:1475-1484 → JsonValue_CombineArray J:3371-3416）：源必须 ARRAY（否则假）；目标非 ARRAY → SetArray（**清空目标原值/或把原数组丢弃转新堆数组**）；逐源元素 **Clone + Append 到目标尾部**（**尾部拼接**，不做按下标合并/覆盖）。

与 合并（文本版）的差异：合并对象/数组是**节点级、非递归、无 isAdd 概念**；合并/合并W 是**文本解析 + 对象递归**。

---

## 8. 排序

- **排序/排序W**（C:1369-1388 → JsonValue_Sort J:2846-2878）：路径 SelectNoCreateW（不创建；jv=0 直接什么都不做）。

```
.判断开始 (type ＝ #JVTYPE_OBJECT)
    array ＝ _Get (_Get (jsonValue, #jv_value), #htEx_eArray)
    Array_Sort (array, sortProc, userData, 0)      ' 第4参写死 0
.判断 (type ＝ #JVTYPE_ARRAY)
    Array_Sort (_Get (jsonValue, #jv_value), sortProc, userData, 0)
```

  **参数_算法 被彻底忽略**：`JsonValue_Sort` 形参 `algorithm` 收下后两处调用都传字面 `0` → 永远 `_Array_FastSort`（快速排序，**不稳定**；Array_Sort A:452-456 的 `algorithm=1 → ShellSort` 分支不可达）。对象排的是**内部顺序数组 eArray**（桶结构不动）→ **直接改变 到文本 的属性输出顺序**。数组排成员指针。
  回调契约：`整数型 比较函数 (整数型 JsonValue1, 整数型 JsonValue2, 整数型 userData)`，两参数是 **JsonValue 结构体指针**（不是值）；返回 `>0 / =0 / <0` 决定交换（FastSort 内以 `zyCall3(proc, a, b, lParam)` 的 `>0/≥0/≤0` 比较，A:480-512；stdcall）。示例回调（S:105-121）用 `v1.K(,“年龄”).取长整数()` 比较。混合类型比较完全由回调决定，库本身不做类型比较。FastSort 细节须复刻：三数取中型（left/mid=(left+right)/2 整除/left,right/m，两次 `>0` 交换）+ 以 left 元素为基准 + Hoare 式相遇分区 + 递归 `(left,i-1)`、`(i+1,right)`；比较次数与相等元素落点都依赖此实现，逐行照抄 A:460-513 才能保证「排序后数组」与「排序后序列化」同时字节一致。

- **排序对象/排序对象W**（C:1346-1367 → JsonValue_SortObj J:2881-2911）：`JsonValue_SortObj (jv, 取反 (参数_降序))` → 升/降序写死比较函数：

```
.如果 (asc)  Array_Sort (array, &JsonValue_SortProcAsc, 0, 0)
.否则       Array_Sort (array, &JsonValue_SortProcDesc, 0, 0)

.子程序 JsonValue_SortProcAsc, 整数型                       ' J:2913-2918
    返回 (lstrcmpW (_Get (value1, #jv_key), _Get (value2, #jv_key)))
.子程序 JsonValue_SortProcDesc, 整数型                      ' J:2921-2926
    返回 (lstrcmpW (_Get (value2, #jv_key), _Get (value1, #jv_key)))
```

  仅对 **OBJECT** 有效（数组返回假）；按**属性名 UTF-16 码元** `lstrcmpW` 排序（非不敏感、非区域排序；升序返回负/0/正，降序=参数互换即反向）。参数_降序 省略（EPL 假）→ 取反 → 升序。同为不稳定快排。C++ 复刻时 `lstrcmpW` 返回值须按 Windows 语义（逐 WORD 无符号比较，负/0/正）参与快排分支。

---

## 9. 取所有键 / 取所有值（+W）

- **取所有键(W)**（C:1251-1277）双重语义：
  ```
  .如果真 (是否为空 (参数_属性名))
      返回 (取键数量(W) (参数_路径))              ' GetLength：OBJECT→HashEx_GetSize, ARRAY→Array_GetCount, 其它/0→0；NoCreate 选择
  .如果真结束
  jv ＝ JsonValue_SelectNoCreateW (…)
  返回 (JsonValue_GetKeysW/A (jv, 参数_属性名))
  ```
  GetKeysW（J:2687-2704）：**仅 OBJECT**，否则返回 0（数组节点也是 0，且不触碰出参）。成功 → `HashEx_GetKeys`（H:597-623）：`重定义数组 (keys, 假, count)` ——**出参数组总是被重定义**（count=0 时清空），元素 = `指针到字节集 (key, lstrlenW×2＋2)`，即 **UTF-16LE 含 `{0,0}` 尾**。A 版（J:2707-2732）先取 W 数组再逐条 `zyW2A`（CP936，不可编码→`?`，含尾 NUL）。返回数量 = `ht_count`。元素顺序 = 插入顺序（eArray）。
- **取所有值(W)**（C:1279-1313）：SelectNoCreateW → `JsonValue_GetValues`（J:2735-2760）：OBJECT→`HashEx_GetValues`（H:626-651，`重定义数组(childValues,假,count)` 后**直接 memcpy eArray 原始指针，不 AddRef**）；ARRAY→`Array_GetValues`（A:419-436 同构）；其它/0 → 0。类侧：
  ```
  .如果真 (count ≠ 0)
      重定义数组 (参数_值, 假, count)
      .计次循环首 (count, i)
          参数_值 [i].置指针 (values [i])        ' 置指针内 Addref 平衡裸指针
  ```
  **count=0 时 参数_值 数组不被重定义**（保留调用方旧内容）。返回 count 即元素数。得到的 Json2 元素**共享树内节点**（非拷贝）——经它们 set 会改到原树；且未调 `设置是否为顶层(假)`（api-spec H-2/H-3），后续对元素调 置代码 会走「清空+合并(…,真)」分支。

---

## 10. 错误与边界 / 全局状态

- **未初始化对象（m_object 指针 0）**：`_取指针对象 (m_object)` 返回 0。读系（到文本/取*/取类型/排序/复制）→ 空结果：`“”`/`{0,0}`/0/假/**-1（取类型）**、复制返回空 Json2；无副作用。变更系分两类：调了 `初始化 ()` 的（置文本/置空/置逻辑/置双精度/置数值/置长整数/置整数/置对象/置数组/置成员/添加成员/插入成员/合并/合并W/合并对象/合并对象Plus/K*/I*）先懒建 OBJECT 根再操作；**没调** 的（置属性(W)、合并数组、清空*、移除*、取源数据*、取所有值*、取所有键*、取父*、取键数量*、排序*、复制*）→ `JsonValue_SelectW (0, path)` 返回 0（J:2448-2450 只处理 path=0；jv=0 逐步传导）→ 底层 setter 见 `jsonValue＝0` 一律**返回假**（各函数首行 `.如果真 (jsonValue ＝ 0) 返回 (假)`），Clear/Destory 见 0 直接 return（无返回值）。
- **路径不匹配**：NoCreate 选择 → 0 → 同上空结果；Create 选择 → **沿途创建**：缺失对象键 → 追加 `null` 子节点（改变输出顺序！）；越界数组下标 → 补 `null` 到该下标；中间标量 → 被强转为其容器类型（对象或数组，旧值销毁）。移除/插入成员/添加成员 在类型不符时分别「假 / 强转 / 强转」（§5、§6）。
- **内存不足 `#JSON_MEMORY`（-3，K:92）**：工厂 `JsonValue_X` 返回 0（zyNewObjectEmpty/HeapAlloc 失败）；`JsonValue_Parsed3` 各分配点返回 -3 并 `JsonValue_Clear(jsonVal)`（J:1387 注释：失败也会清除源数据）；Set 系 `HashEx_Put/Array_Insert` 返回 -1 → 假。类方法 置代码 把错误码写进 `参数_错误代码` 参考参数并返回假。运行期 OOM 分支对「输入→输出等价」的影响仅是失败返回值，不产生随机差异。
- **可复现性相关的全局/隐式状态**：
  - 堆：根对象 `GetProcessHeap()`（J:192-196 等，`json＝0` 时）；子节点继承父 `#zyobj_hHeap`；**置数组/数组强转路径**（JsonValue_SetArray 的 hHeap=0 bug）导致每个被转换数组自带 `HeapCreate` 私有堆（A:20-23）——仅内存归属，不改变输出。
  - `JsonValue_PathParsed` 用 `Array_Create (GetProcessHeap ())` 临时路径数组（J:2355），用完即毁。
  - 属性顺序数组（eArray）是输出顺序唯一依据；`HashEx` 扩容 rehash（H:106-141）**不动** eArray → 序列化顺序对哈希表容量变化免疫。
  - 引用计数 `InterlockedIncrement/Decrement`（Object J:19-25）与 OnRelease→Detch 的「释放即自动摘除」语义必须复刻，否则 置成员/移除 的中间态指针复用行为不同。
  - 无任何进程级缓存/静态表；比较/解析依赖 Win32 `lstrcmpW/lstrlenW/WideCharToMultiByte` 语义 → 在 Windows 上恒定。
- **其它已确认行为差异（与直觉相反但必须照抄）**：`取文本` 把字符串 `"null"` 内容吞成空（§4）；`取整数` 对 LONG 只读低 32 位、`取逻辑` 对 LONG 只测低 32 位（J:432/504）；解析整数子成员类型是 DOUBLE（J:2000）；`\/` 恒被输出转义（J:3177）；`a\nb` 解析后重输出变 `a\r\nb`（J:3062）；合并零命中时走 合并对象 全量兜底（§7.1）；置属性 传未初始化 Json2 → 假；`置文本W` 判左补右的双 NUL（§5.2）；`到文本` 对未创建路径返回 `“”`（而非 `null`）。

---

## 附：歧义清单（必须 golden-test 钉死的候选解释）

1. `到文本 (双精度)` 的 krnln 运行时格式（§2）：整值是否带尾点/`-0`/指数阈值 —— 无源码依据。
2. `JsonValue_GetInt` 对 STRING：`StrToInt`（shlwapi StrToIntW，D:213）MSDN 返回 BOOL → 恒 1/0？还是旧版返回数值？候选① 1/0（倾向，实现即 API 语义）；候选② 数值。
3. `到长整数/到整数 (双精度)`（取长整数/取整数 的 DOUBLE 分支，J:384/434）：四舍五入 vs 截断、`.5` 规则 —— krnln 转换命令语义，实测钉死。
4. `到数值 (文本)`（GetDouble 的 STRING 分支 / _StrToDoubleW）：是否接受 `1e5`、`+5`、`&H`、区域小数点 —— 实测。
5. `W2UTF8_T`/`UTF82W_T`（M:62-68）对孤立代理/非法 UTF-8 的 fallback —— 外部 ec 模块无源码。
6. 文本型返回中尾随 NUL 是否被上层 `＝` 比较吞掉（§1.4，倾向是，因 C:1089 比较成立）。
7. `JsonValue_CombineProc` 全对象分支返回 ret 初值假引发的 合并对象 兜底（§7.1）：语义已按源码确定，但 合并对象 失败再叠加时 `合并` 返回值 = 兜底结果 —— 与注释「属性名相同的值替换」矛盾，照码实现。

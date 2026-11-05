# zyJson2.51 行为规格（spec-core）— 值模型 / 类型打标 / 存储 / 成员顺序 / 解析器 / 路径 / 引用计数

> 目的：为 **C++ 原地替换（drop-in）** 提供「相同输入 → 相同输出」的**行为**规格。
> 与 `api-spec.md` 的分工：`api-spec.md` 管**签名**（参数名/可空/顺序/返回类型）；本文管**语义**（内存布局、状态机、顺序、错误码、边界行为）。两份冲突时以本文为准（本文直接从 EPL 源码逐行核对）。
> 所有结论均来自下列文件；每条规则附**原文片段 + 文件:行号**，可逐条复核。

## 0. 约定

### 0.1 文件别名（均在 `libs\zyJson支持库\源码txt\` 下）

| 别名 | 实际路径 |
| --- | --- |
| `[Json]` | `代码\程序集_Json.static.e.txt`（3418 行） |
| `[Hash]` | `代码\程序集_HashTableEx.static.e.txt`（713 行） |
| `[Arr]` | `代码\程序集_Array.static.e.txt`（469 行） |
| `[Stack]` | `代码\程序集_Stack.static.e.txt`（186 行） |
| `[Str]` | `代码\程序集_zyString.static.e.txt`（272 行） |
| `[Obj]` | `代码\程序集_Object.static.e.txt`（94 行） |
| `[Help]` | `代码\程序集_help.static.e.txt`（714 行） |
| `[Cls]` | `代码\Json2.class.e.txt`（1597 行） |
| `[Boot]` | `代码\程序集_启动.static.e.txt` |
| `[Const]` | `常量.e.txt`（127 行） |
| `[Type]` | `自定义类型.e.txt` |
| `[DLL]` | `DLL.e.txt`（58 条 `.DLL命令`） |

行号是导出 txt 的行号（含 `.版本 2` 等伪代码行），与 `api-spec.md` 的 `L<n>`（针对 `[Cls]`）一致。

### 0.2 术语与编码

* **W 文本** ＝ UTF-16LE、以单个宽字符 `{0,0}` 结尾、**无 BOM**（详见 `api-spec.md` F 节）。EPL `字节集` 参数在带 `W` 后缀的入口里就是这种串。
* **A 文本** ＝ 易语言 `文本型`，实际是 **ANSI/cp936(GBK)** 字节串。所有 `xxx`（无后缀）与 `xxxA` 入口的文本参数默认 cp936。
* 长度语义：`lstrlenW` 返回**宽字符个数**（不含终止符）。凡 `× 2` 即「字符→字节」，凡 `× 2 ＋ 2` 即「含结尾 0,0 的字节数」。
* 本文里 **char** 一律指 16-bit UTF-16 代码单元；**位置/索引**一律 0-based（除非明说「1-based 的成员序号」）。

### 0.3 三条「必读」结论（最容易在 C++ 里写错的地方）

1. **对象成员顺序 ＝ 插入顺序**（不是哈希顺序、不是字典序）。哈希表额外维护一条 `eArray` 有序数组，`取所有键`/`到文本`/`取所有值`/`Clone`/`合并对象` 全部走它（§4）。
2. **容器内的整数被存成 `JVTYPE_DOUBLE`（类型 1），只有根节点的裸整数才是 `JVTYPE_LONG`（类型 2）**；但两种情况 `jv_raw` 都保存原始 token，所以**序列化输出与输入逐字符相同**，而 `取类型`/`K().取类型()` 会报 1（§2.5、`[Json]:2000`）。
3. **解析器不做严格 JSON 校验**：尾逗号、无引号 key、单引号、`\/`、非标准 `\xNN` 等大量语法被接受；`\uXXXX` 在 `参数_Unicode解码 ＝ 假` 时**不解码**，字面量 `\uXXXX`（含反斜杠）被存下来，输出时反斜杠被再转义成 `\\uXXXX`（§6、§3.4）。

---

# 1. 值模型（JsonValue 内存布局 + zyObject/COM 包装）

## 1.1 JsonValue 节点：48 字节，全部字段用偏移访问

偏移常量来自 `[Const]:38-41`（zyObject 头）与 `[Const]:97-104`（JsonValue 扩展）：

```
.常量 zyobj_pFunctions, "0", , zyObjectStruct
.常量 zyobj_refCount, "4"
.常量 zyobj_pParentThis, "8"
.常量 zyobj_hHeap, "12"
...
.常量 jv_cbSize, "48"
.常量 jv_hashCode, "16"
.常量 jv_key, "20"
.常量 jv_value, "24", , 8字节
.常量 jv_next, "32"
.常量 jv_type, "36"
.常量 jv_parent, "40"
.常量 jv_raw, "44", , 元数据
```
（`[Const]:38-41`、`[Const]:97-104`）

### 完整布局表（32 位进程，所有字段 4 字节对齐）

| 偏移 | 名称 | 类型/含义 | 谁写 | 谁释放 |
| --- | --- | --- | --- | --- |
| 0 | `pFunctions` | 虚表指针，指向 **4 个函数指针** 的数组（`QueryInterface/AddRef/Release/Destory`） | `JsonValue_InitFunctons` `[Json]:4-13` | `zyDeleteObject` `[Obj]:48` |
| 4 | `refCount` | 引用计数（`InterlockedIncrement/Decrement`，**初始 1**） | 构造函数 `_Set (this, #zyobj_refCount, 1)` | — |
| 8 | `pParentThis` | **未被 JsonValue 使用**（保留自 zyObject 结构，只为给 `zyNewObject` 的「类模板拷贝」留位）。真正的父指针是 `jv_parent` | 仅 `zyNewObject` 的 `RtlMoveMemory (this, Class, size)` 路径 `[Obj:13]` | — |
| 12 | `hHeap` | 本节点及其**所有子对象/容器/字符串**使用的堆。根节点＝`GetProcessHeap()`；子节点＝从父（构造时传入的 `json` 参数）继承 | `[Json]:193-198` 等所有构造函数 | — |
| 16 | `hashCode` | 属性名的 FNV-ish 哈希（见 §4.3）。仅当节点作为对象成员时有效 | `HashEx_Put/PutPtr/Insert` `[Hash]:197,260,327` | — |
| 20 | `key` | **属性名**指针（UTF-16，含 `{0,0}`）。属于「该节点在其父对象里的那把钥匙」。数组成员此字段＝0 | `HashEx_Put/PutPtr/Insert` 用 `_CloneString(Ptr)` 分配 `[Hash]:198,261,328` | `HashEx_Remove/RemoveFromValue/Clear/Destory` 用 `HeapFree` `[Hash]:540,588,704,77` |
| 24 | `value` | **8 字节联合体**（24-27 低 4 字节，28-31 高 4 字节），按 `type` 解释（见下表） | 各构造函数/Setter | `JsonValue_Destory`/`JsonValue_Clear` `[Json]:43-74,98-128` |
| 32 | `next` | **哈希桶内链**（单链，头插）。仅当节点在某个对象的桶链里时有效 | `[Hash]:199,262,329` | — |
| 36 | `type` | `JVTYPE_*`（§2.1） | 构造函数/Setter | — |
| 40 | `parent` | 父 `JsonValue` 指针（**裸指针，不持有引用**） | `JsonValue_Append/Insert/SetAt/GetAt/Set*/Get*` | — |
| 44 | `raw` | **元数据**指针：源 token 的 UTF-16 文本（含 `{0,0}`），序列化数字/null/bool/undefined 时**原样吐出** | `_CloneString` | `JsonValue_Destory/Clear/SetRaw*` `[Json]:76-80,130-134,541-545` |

### `value` 联合体按 `type` 的解释

| `type` | `value` 内容 | 分配者 | 释放者 |
| --- | --- | --- | --- |
| `JVTYPE_UNDEFINED`(-1) | 0 | — | — |
| `JVTYPE_NULL`(0) | 0 | — | — |
| `JVTYPE_DOUBLE`(1) | IEEE-754 double（`_SetDouble/_GetDouble`，低 4 字节在前）`[Json]:279,301` | 立即数写入 | 无需 |
| `JVTYPE_LONG`(2) | 64-bit 整数（`_SetLong/_GetLong`）`[Json]:360,382` | 同上 | 无需 |
| `JVTYPE_BOOL`(3) | 低 4 字节＝0/1（`到整数 (bool)`）`[Json]:487` | — | — |
| `JVTYPE_STRING`(4) | `PWSTR`（UTF-16，`strLen*2+2` 字节）`[Json]:610,630,652` | `_CloneStringPtr`/`_CloneString` → `HeapAlloc(hHeap, HEAP_ZERO_MEMORY, …)` `[Help]:235-247,249-265` | `_Free (v, hHeap)` `[Json]:46` |
| `JVTYPE_ARRAY`(5) | **Array 句柄**（`hArray`，见 §5）`[Json]:806` | `Array_Create (hHeap)` | 先逐个 `Release` 成员，再 `Array_Destroy` `[Json]:61-70` |
| `JVTYPE_OBJECT`(6) | **HashTableEx 句柄**（`hHashTable`，见 §4）`[Json]:842` | `HashEx_Create (hHeap)` | 先逐个 `Release` 成员，再 `HashEx_Destory` `[Json]:50-59` |

注意 `JVTYPE_STRING` 时 `jv_raw`＝0（字符串构造函数从不写 raw），所以 `取源数据` 对文本型无效（与 `[Cls]:1052` 的尾注释一致，也和 `JsonValue_GetRawW` 的「不支持 文本、数组和对象」一致 `[Json]:567`）。

### 构造函数的统一骨架

每个构造函数体都是这几句（以 `JsonValue_Object` 为例，`[Json]:828-844`）：

```
.如果 (json ＝ 0)
    hHeap ＝ GetProcessHeap ()
.否则
    hHeap ＝ _Get (json, #zyobj_hHeap)
.如果结束
this ＝ zyNewObjectEmpty (hHeap, #jv_cbSize)
_Set (this, #zyobj_hHeap, hHeap)
_Set (this, #zyobj_pFunctions, JsonValue_InitFunctons (hHeap))
_Set (this, #zyobj_refCount, 1)
_Set (this, #jv_value, HashEx_Create (hHeap))
_Set (this, #jv_type, #JVTYPE_OBJECT)
```

* 第一个参数 `json` 的语义**不是父节点**，只是「借用它的堆」；`parent`/`key` 都不在这里设置（由调用者 `JsonValue_Set*/Append` 补写）。所以「`创建对象` 可以看成静态方法」——传 0 就走 `GetProcessHeap()`（`[Cls]:172-182`）。
* `pFunctions` 是**每个对象各自一份** 16 字节虚表数组（`_Alloc (16, hHeap)` `[Json:8]`），不是共享的类虚表 → C++ 里可以共享 vtable，但**不要**假设可以按 vtable 指针识别类型；识别类型请用 `jv_type`。
* `zyNewObjectEmpty` 只 `HeapAlloc(HEAP_ZERO_MEMORY)`，**不拷贝任何模板**；`zyNewObject` 才做 `RtlMoveMemory (this, Class, size)` 模板拷贝（`[Obj]:12-13`）。JsonValue 全部走 `zyNewObjectEmpty`。

## 1.2 COM 式包装：虚表槽与调用方式

```
.常量 zy_QueryInterface, "0"
.常量 zy_AddRef, "1"
.常量 zy_Release, "2"
.常量 zy_Destory, "3"
```
（`[Const]:33-36`）

JsonValue 的虚表内容（`[Json]:8-12`）：

```
_Set (pFunctions, 0, _子程序指针到整数 (&zyObject_QueryInterface))
_Set (pFunctions, 4, _子程序指针到整数 (&zyObject_AddRef))
_Set (pFunctions, 8, _子程序指针到整数 (&JsonValue_OnRelease))
_Set (pFunctions, 12, _子程序指针到整数 (&JsonValue_Destory))
```

即：**只有 `Release` 被改写**（多了一步 `JsonValue_Detch`），`Destory` 是 JsonValue 自己的析构；`QueryInterface` 恒返回 1（不做任何接口查询校验，`[Obj]:65-70`）。

zyObject 通用实现（`[Obj]:72-93`）：

```
.子程序 zyObject_AddRef, 整数型
    cRef ＝ InterlockedIncrement (this ＋ 4)
    返回 (cRef)

.子程序 zyObject_Release, 整数型
    cRef ＝ InterlockedDecrement (this ＋ 4)
    .如果真 (cRef ＝ 0)
        zyThisCall (this, #zy_Destory)
        zyDeleteObject (this)
    .如果真结束
    返回 (cRef)
```

JsonValue 版 Release（`[Json]:15-25`）：

```
.子程序 JsonValue_OnRelease, 整数型
    cRef ＝ InterlockedDecrement (this ＋ #zyobj_refCount)
    .如果真 (cRef ＝ 0)
        JsonValue_Detch (this)          ' ← 关键：先从父容器摘自己
        zyThisCall (this, #zy_Destory)  ' ← 再递归释放子节点 / 释放字符串 / 销毁容器
        zyDeleteObject (this)           ' ← 最后 free 虚表数组 + 节点本身
    .如果真结束
    返回 (cRef)
```

**`cRef` 减到 0 之后的顺序必须是 Detch → Destory → free 节点**，且 Detch 早于 Destory：这保证子节点在析构过程中**已经不在父容器里**，父容器不会被重复摘除（见 §1.4 的替换流程依赖这一点）。

`zyThisCall` 的实际调用约定（`[Help]:192-214`）：取 `*(int*)this`（虚表）再取 `[index*4]`，然后用 `zyCall7 (pFunc, this, 参数1..参数6)` 以 **C 调用约定、this 作第一个入参** 调用。C++ 等价：`((int(*)(void*, int, int, int, int, int, int)) vt[index])(this, …)`。非虚直接调用也一样（`JsonValue_Release (jsonValue)` ＝ `zyThisCall (jsonValue, #zy_Release)`，`[Json]:178-184`）。

`zyDeleteObject`（`[Obj]:38-49`）：

```
hHeap ＝ _Get (this, #zyobj_hHeap)
pFunctions ＝ _Get (this, #zyobj_pFunctions)
_Free (pFunctions, hHeap)
_Free (this, hHeap)
```
注意 `this ＝ 0` 直接返回；**不检查 refCount**（被 Release 调到时已经是 0）。

## 1.3 易语言 `对象` 变量（m_object）怎么装这个指针

类里唯一的状态是 `.程序集变量 m_object, 对象`（`[Cls]:4`）。读写它靠两个内联汇编（`[Help]:440-459`）：

```
.子程序 _取指针对象, 整数型           ' mov eax,[ebp+08] / mov eax,[eax] / mov eax,[eax]
.子程序 _取指针对象地址, 整数型        ' mov eax,[ebp+08] / mov eax,[eax]
```

* `_取指针对象地址 (m_object)` ＝ EPL 对象变量内部数据块的首地址；`_Set (_取指针对象地址 (m_object), 0, this)` ＝ **把 `this` 直接写进对象变量的指针字段**，绕过 `QueryInterface`/`AddRef`（`[Cls]:33,54,78,123,142,169,284,301`）。
* `_取指针对象 (m_object)` ＝ 取出该指针字段值（多一层解引用）。
* `m_object.清除 ()` ＝ EPL 运行时对持有的指针调虚表 `Release`（索引 2）并置空。因此：
  * `置指针`（`[Cls]:161-170`）必须先 `JsonValue_Addref (参数_指针)` 再 `清除` 再写入 → **两个 Json2 变量指向同一节点时，各自都会 Release**。
  * `销毁`（`[Cls]:149-150`）只做 `m_object.清除 ()`。
  * `取指针`（`[Cls]:158-159`）只读，不 AddRef → 调用方（如 `置属性`）拿到的是** borrowed 指针**，`置属性` 立即 `JsonValue_Clone`（`[Cls]:462-472`）。
* 因为 `_Set` 写指针不做引用计数，`初始化 ()`/`置代码` 里「新建对象 → 清除 → 写入」的顺序是必须的：先建 `this`（refCount=1），`m_object.清除 ()`（此时旧指针 Release），再写入新指针（`[Cls]:68-78`）。写入后**refCount 的那 1 就归 m_object 持有**。
* `创建*` 系列把引用「转移」的写法（`[Cls]:172-182`）：`this`（ref 1）→ `jv.置指针(this)`（+1＝2）→ `JsonValue_Release(this)`（－1＝1，由 jv 持有）。C++ 里必须保持这个平衡，否则早释放或泄漏（`api-spec.md` H-11）。

## 1.4 谁释放子节点：析构、清空、替换、摘出

### JsonValue_Destory（真正析构，`[Json]:28-80`）

```
.判断开始 (type ＝ #JVTYPE_STRING)
    v ＝ _Get (this, #jv_value)
    .如果真 (v ≠ 0)
        _Free (v, hHeap)
.判断 (type ＝ #JVTYPE_OBJECT)
    count ＝ HashEx_GetValues (v, items)
    .计次循环首 (count, i)
        zyThisCall (items [i], #zy_Release)
    .计次循环尾 ()
    HashEx_Destory (v)
.判断 (type ＝ #JVTYPE_ARRAY)
    count ＝ Array_GetValues (v, items)
    .计次循环首 (count, i)
        zyThisCall (items [i], #zy_Release)
    .计次循环尾 ()
    Array_Destroy (v)
...
raw ＝ _Get (this, #jv_raw)
.如果真 (raw ≠ 0)
    _Free (raw, hHeap)
```

* **子节点由父节点释放**：`Release` 每个子（递归析构），再销毁容器句柄。
* `HashEx_GetValues`/`Array_GetValues` 先把指针**拷进 EPL 数组**（`items`）再遍历，因此释放过程中容器被并发改动（子节点 Release→Detch→`RemoveFromValue`）是**安全的**（`[Hash]:648-650`、`[Arr]:432-435`）。
* **不**清 `jv_key`/`jv_parent`/`jv_next`/`jv_hashCode`（key 由哈希表负责，见 §1.1 表）。
* `JsonValue_Destory` 也被当作「类型转换前的清空」直接调用（不引用计数）：所有 `JsonValue_SetXxx` 与 `JsonValue_SetArray/SetObject` 都调它（`[Json]:216,254,329,410,460,524,757,771,786,822,859`）。

### JsonValue_Clear（`清空`，`[Json]:83-134`）与 Destory 的差别

| | OBJECT | ARRAY | STRING | 标量（NULL/BOOL/LONG/DOUBLE/UNDEFINED） |
| --- | --- | --- | --- | --- |
| 释放子节点/字符串 | 是（逐个 Release） | 是 | 是 | — |
| 容器本身 | `HashEx_Clear`（**保留句柄**、清桶链、`Array_Clear` eArray、count=0，`[Hash]:683-711`） | `Array_Clear`（保留句柄，count=0，`[Arr]:358-368`） | — | — |
| `jv_value` | 不变（仍非 0） | 不变 | 置 0 | `_SetLong (jsonValue, #jv_value, 0)`（`[Json]:126`，**仅在默认分支**） |
| `jv_type` | **不变** | **不变** | **不变（仍是 STRING，但指针＝0 → 输出 `""`）** | **不变** |
| `jv_raw` | 释放并置 0 | 同 | 同 | 同 |

**要点：`清空` 不改类型**。`JsonValue_Clear` 后：对象→`{}`，数组→`[]`，文本→`""`（因为 `ToStringProc` 对 STRING 走 `JsonValue_GetStringW`，pStr=0 时返回 `{0,0}`，只写出两个引号），数字→值为 0 但 raw 被释放 → 序列化输出**空**（`JsonValue_GetRawW` 返回 `{0,0}`，`zyString_AddW` 什么都不加）。最后一项是很容易被漏掉的输出差异。

`JsonValue_Clear` 还被 `JsonValue_Parsed3` 在解析前后各调一次（`[Json]:1387`、失败路径 `[Json]:1424,1432`）→ **解析失败会把原内容清空**（见 §6.9）。

### JsonValue_Detch（`分离`，`[Json]:137-167`）

```
jvParent ＝ _Get (jsonValue, #jv_parent)
.如果真 (jvParent ≠ 0)
    type ＝ _Get (jvParent, #jv_type)
    .判断开始 (type ＝ #JVTYPE_ARRAY)
        ret ＝ Array_RemoveFromValue (pArray, jsonValue)
    .判断 (type ＝ #JVTYPE_OBJECT)
        ret ＝ HashEx_RemoveFromValue (pHash, jsonValue)
    .默认
    .判断结束
    _Set (jsonValue, #jv_parent, 0)
.如果真结束
```

* 只处理「父是数组/对象」；父是标量时什么都不做（但仍会清 `jv_parent`）。
* `HashEx_RemoveFromValue` 会 `HeapFree` 该成员的 **key 字符串**并 `count--`、从 eArray 移除（`[Hash]:549-595`）；`Array_RemoveFromValue` 只做移位＋`count--`（`[Arr]:392-417`）。
* **`Detch` 不释放节点本身**，`分离`（`[Cls]:395-396`）之后该 Json2 变量仍指向原节点（引用数没变），只是脱离了树 → 之后 `到文本` 只输出自己子树、`取父` 返回 0。

### 替换成员的标准流程（`JsonValue_SetW` 系列，`[Json]:1180-1207`）

```
old ＝ HashEx_Get (pMap, key)
_Set (jsonVal, #jv_parent, jsonValue)
ret ＝ HashEx_Put (pMap, key, jsonVal)
.如果真 (old ≠ 0)
    zyThisCall (old, #zy_Release)
.如果真结束
返回 (ret ≠ -1)
```

顺序很重要：**先记录 old，再挂 parent，再 Put（把 new 接进桶链＋eArray 并接管 old 的 key），最后 Release(old)**。
old 在 Release→Detch 时尝试 `HashEx_RemoveFromValue`，但它**已经不在桶链上**（Put 时把 prev.next 直接指向了 new，见 `[Hash]:234-237`），所以找不到 → 不会二次释放 key。这是源码里成文的「隐式协议」，C++ 版必须保持同一顺序，否则会 double-free key 字符串。

* `JsonValue_Append`/`Insert`/`SetAt`/`GetAt` 也都是「先 `_Set (jsonVal, #jv_parent, jsonValue)`」（`[Json]:885,910,1026,1035,1040,941`）。
* **把同一个节点 Set 进两个父节点会双挂**：`_Set (jsonVal, #jv_parent, …)` 只覆盖 parent，不会从旧父摘除 → 旧父容器仍指向它 → 双方各 Release 一次 → double free。C++ 里建议改成「转移所有权前先 Detch」。
* 数组替换（`JsonValue_SetAt`，`[Json]:1032-1041`）用的是 `Release(old)`（→ 自动 `Array_RemoveFromValue` 把 old 从槽位摘掉）**＋ `Array_Insert`**（右移后插入），效果＝原地替换。若 old 为 0 才用 `Array_Set`。这段看起来像 bug（Insert 会右移），但因为 Release 已经摘除了 old，实际语义正确——C++ 版**不要**照字面写成「先插入后释放」。

## 1.5 堆（hHeap）继承规则

* 根：`GetProcessHeap()`（`_Alloc` 默认走它，`[Help]:216-223`）。
* 子节点：构造时传父节点指针 → `_Get (json, #zyobj_hHeap)` → 与父同堆。
* 容器：`HashEx_Create (hHeap)` / `Array_Create (hHeap)` 用节点的 hHeap；两者都只在**参数为 0**时才 `HeapCreate` 自己的堆并记 `isNewHeap=1`（`[Hash]:27-30`、`[Arr]:20-23`）。`isNewHeap` 决定销毁时是 `HeapDestroy` 还是两次 `HeapFree`（`[Hash]:67-84`、`[Arr]:49-54`、`[Stack]:53-58`）。
* **例外（源码缺陷）**：`JsonValue_SetArray` 用了一个**从未赋值**的局部 `hHeap`（`[Json]:811-825`）：
  ```
  .局部变量 hHeap, 整数型
  ...
  JsonValue_Destory (jsonValue)
  _Set (jsonValue, #jv_value, Array_Create (hHeap))
  ```
  → hHeap＝0 → `Array_Create` 自己 `HeapCreate` → 每次「把节点变成数组」都新建一个私有堆。C++ 里若统一用节点堆，行为差异仅体现在内存/句柄数量，不影响输出；若要 100% 对齐内存占用则照抄。
* 解析用的 `Stack_Create (hHeap)` 也用节点堆（`[Json]:1382-1383`）。

---

# 2. 类型系统与打标

## 2.1 JVTYPE 取值（`[Const]:3-10` ＝ `[Const]:80-87`，两套同值，前者公开）

```
.常量 JV类型_未定义, "-1", 公开, JVTYPE_UNDEFINED
.常量 JV类型_空, "0", 公开, JVTYPE_NULL
.常量 JV类型_双精度型, "1", 公开, JVTYPE_DOUBLE
.常量 JV类型_长整数型, "2", 公开, JVTYPE_LONG
.常量 JV类型_逻辑型, "3", 公开, JVTYPE_BOOL
.常量 JV类型_文本型, "4", 公开, JVTYPE_STRING
.常量 JV类型_数组, "5", 公开, JVTYPE_ARRAY
.常量 JV类型_对象, "6", 公开, JVTYPE_OBJECT
```

`取类型` 直接读 `jv_type`；节点指针为 0 时 `JsonValue_GetType` 返回 `#JVTYPE_UNDEFINED`（-1）（`[Json]:1345-1351`）。**C++ 里对空句柄要返回 -1，不是 0。**

## 2.2 打标入口全览（谁把 type 设成几）

| 动作 | type 结果 | 备注 |
| --- | --- | --- |
| `JsonValue_Object` / `JsonValue_SetObject` | 6 | `SetObject` 仅在 `type ≠ 6` 时 Destory+建新哈希（`[Json]:847-864`） |
| `JsonValue_Array` / `JsonValue_SetArray` | 5 | 同上（`[Json]:811-826`） |
| `JsonValue_String/W/A`、`SetString*` | 4 | raw 不设置（`[Json]:594-654,747-790`） |
| `JsonValue_Bool/Null/Double/Long/Undefined` + `Set*` | 3/0/1/2/-1 | 都写 raw（`[Json]:186-260,262-419,471-529`） |
| `JsonValue_SetInt` | **2（LONG）** | `_SetLong` 存 int32 到 8 字节槽（`[Json]:449-469`） |
| 解析：容器内整数 | **1（DOUBLE）** | 见 §2.5 |
| 解析：根节点裸整数 | 2（LONG） | `JsonValue_SetLong`（`[Json]:1994`） |
| 解析：容器内「看起来是数字但有小数点/e/符号」 | 1（DOUBLE） | |
| 解析：`true/false` | 3 | `bool ＝ lstrcmpW (value, "true") ＝ 0`（`[Json]:1967-1977`）—— 任何非 `true` 的 BOOL 文本（即 `false`）→ 假 |
| 解析：`null` | 0 | |
| 解析：引号包住 | 4 | |

## 2.3 数值读取（跨类型转换，决定 `取双精度/取长整数/取整数` 的结果）

`JsonValue_GetDouble`（`[Json]:291-315`）：

```
.判断开始 (type ＝ #JVTYPE_DOUBLE)   → 返回 (_GetDouble (jsonVal, #jv_value))
.判断 (type ＝ #JVTYPE_LONG)        → 返回 (到数值 (_GetLong (jsonVal, #jv_value)))
.判断 (type ＝ #JVTYPE_BOOL)        → 返回 (_Get (jsonVal, #jv_value))          ' 0/1
.判断 (type ＝ #JVTYPE_STRING)      → 返回 (到数值 (zyW2A_Point (v)))           ' GBK 后 到数值
.默认                               → 返回 (0)
```

`JsonValue_GetLong`（`[Json]:372-396`）：LONG→原值；DOUBLE→`到长整数 (double)`（向零截断）；BOOL→0/1；STRING→`_wtoi64_ptr (v)`（ntdll，遇非数字停止，不报溢出）；其它→0。
`JsonValue_GetInt`（`[Json]:422-446`）：同上，但 STRING 走 `StrToInt (v)`（**注意：`[DLL]:213` 声明的是 `shlwapi!StrToIntW`，返回 BOOL 而非数值**，见 §9 待实测项 T-3）。
`JsonValue_GetBool`（`[Json]:495-512`）：只有 LONG/DOUBLE/BOOL 参与，判 `≠ 0`；**STRING/NULL/OBJECT/ARRAY 一律假**（`"false"` 文本取逻辑值＝假，`"1"` 文本取逻辑值也＝假）。

## 2.4 `JsonValue_Parsed_ValueIsNumber` ＋ `JsonValue_Parsed_GetValueType` 判定链

`GetValueType`（`[Json]:2056-2082`）**判定顺序**（顺序即优先级，不可调换）：

```
nullStr ＝ { 110, 0, 117, 0, 108, 0, 108, 0, 0, 0 }         ' "null"
trueStr ＝ { 116, 0, 114, 0, 117, 0, 101, 0, 0, 0 }         ' "true"
falseStr ＝ { 102, 0, 97, 0, 108, 0, 115, 0, 101, 0, 0, 0 }  ' "false"
.判断开始 (lstrcmpW (pVal, nullStr) ＝ 0) → #JVTYPE_NULL
.判断 (lstrcmpW (pVal, trueStr) ＝ 0 或 lstrcmpW (pVal, falseStr) ＝ 0) → #JVTYPE_BOOL
.判断 (JsonValue_Parsed_ValueIsNumber (val, isDouble)) → isDouble ? #JVTYPE_DOUBLE : #JVTYPE_LONG
.默认 → #JVTYPE_UNDEFINED
```

* 字面量比较是 **区分大小写的精确 UTF-16 串比较**（`Null`/`TRUE`/`True` 全部**不**识别 → 落到 UNDEFINED → `#JSON_INVALIDVALUE`）。
* `引号` 的判断**在 GetValueType 之前**由调用方完成：值 token 首字符是 `"` 或 `'` 就当作 STRING，不再进 GetValueType（`[Json]:1943-1954`）。

## 2.5 数字打标的精确条件（逐行）

`[Json]:2085-2157`（完整原文，注意 `isDouble` 是 `参考` 输出）：

```
pVal ＝ _取指针字节集 (val)
len ＝ lstrlenW (val)
.如果真 (len ＝ 0)
    返回 (假)
.如果真结束
.判断循环首 (i ＜ len)
    wchar ＝ _指针到短整数U (pVal ＋ i × 2)
    .如果真 (wchar ≤ 32)              ' 跳过空白
        i ＝ i ＋ 1
        到循环尾 ()
    .如果真结束
    .如果 (i ＝ 0)
        .如果真 (wchar ≠ 45 且 wchar ≠ 43 且 (wchar ＜ 48 或 wchar ＞ 57)) ' 不是 + - 0~9
            .如果真 (wchar ＝ 46) ' 点.
                done ＝ done ＋ 1
                .如果真 (done ＞ 1) ' 有两个点以上返回假
                    返回 (假)
                .如果真结束
                i ＝ i ＋ 1
                到循环尾 ()
            .如果真结束
            返回 (假)
        .如果真结束
    .否则
        .如果真 (wchar ＜ 48 或 wchar ＞ 57) ' 0~9
            .判断开始 (wchar ＝ 46) ' 点.
                done ＝ done ＋ 1
                .如果真 (done ＞ 1) ' 有两个点以上返回假
                    返回 (假)
                .如果真结束
                i ＝ i ＋ 1
                到循环尾 ()
            .判断 (wchar ＝ 101 或 wchar ＝ 69) ' E 或 e
                e ＝ e ＋ 1
                .如果真 (e ＞ 1)
                    返回 (假)
                .如果真结束
                i ＝ i ＋ 1
                到循环尾 ()
            .判断 (wchar ＝ 43 或 wchar ＝ 45) ' + -
                f ＝ f ＋ 1
                .如果真 (f ＞ 1) ' 有两个点以上返回假
                    返回 (假)
                .如果真结束
                i ＝ i ＋ 1
                到循环尾 ()
            .默认
            .判断结束
            返回 (假)
        .如果真结束
    .如果结束
    i ＝ i ＋ 1
.判断循环尾 ()
.如果真 (done ＝ 1 或 e ＝ 1 或 f ＝ 1)
    isDouble ＝ 真
.如果真结束
返回 (真)
```

规则整理成 C++ 可实现的判定：

1. **允许字符集**：数字 `0-9`、最多 1 个 `.`、最多 1 个 `e/E`、最多 1 个 `+/-`、以及任意个 `wchar ≤ 32` 的空白（含 `\0`）。其它字符（如 `x`、`f`、`,`、`-` 出现 2 次）立即 `返回 (假)` → UNDEFINED → `#JSON_INVALIDVALUE`。
2. **「首字符」特殊处理只在整个 token 的第一个字符就是数字/`+`/`-`/`.` 时生效**（判据是 `i ＝ 0`，而**空白跳过会推进 `i`**）。所以：
   * `"-5"`（无前置空白）→ i=0 处 `-` 被放行且不计数 → `f` 保持 0 → **`isDouble ＝ 假` → LONG**。
   * `" -5"`（token 里有前置空白，例如被外层保留）→ 空白使 `i` 变成 1 → 随后 `-` 走 `.否则` 分支 → `f ＝ 1` → **DOUBLE**。
     外层解析器通常已剥掉值前的空白（`valueStart` 会随空白前移，见 §6.5），所以实际输入里几乎总是前者。C++ 版必须保留这个「`i` 不重置」的行为。
3. `isDouble` 为真 **当且仅当** `done＝1 或 e＝1 或 f＝1`（注意是 `＝1`，`done>1` 已提前返回假；`done/e/f` 为 0 则保持 `isDouble` 的原值＝假，因为它是 `参考` 参数、被调方传入的是新局部变量 `isDouble`（`[Json]:1656,2072`）→ 初值假）。
4. **不接受**：`NaN`、`Infinity`、`-Infinity`、`0x1F`、`1_000`、`1.2.3`、`1e`（`e` 后没数字仍**接受** → DOUBLE）、`.`（单独一个点仍**接受** → DOUBLE，`_StrToDoubleW(".")` → 0）、`""`（len=0 → 假）、前导零（`007` → 接受 → LONG，值 7，raw `"007"`）。
5. **没有任何范围检查**（不做 int64/double 溢出判断），也没有 `到整数`/`到小数` 的中间换算；类型只由「有没有小数点/e/符号」决定。

### 关键的容器→DOUBLE 提升（本库最大的一条「反直觉」）

`[Json]:1992-2001`：

```
.判断 (childType ＝ #JVTYPE_LONG)
    .如果 (jvParent ＝ 0)
        .如果真 (JsonValue_SetLong (jsonVal, _StrToLongW (value), value))
            返回 (#JSON_OK)
        .如果真结束
        pos ＝ valueStart
        返回 (#JSON_MEMORY)
    .否则
        jv ＝ JsonValue_Double (jsonVal, _StrToLongW (value), value)   ' ← 注意是 Double
    .如果结束
```

* 容器里的整数走 `JsonValue_Double`：`_StrToLongW` 的 `长整数型` 被隐式转成 `双精度小数型` 存进 `value`，`type ＝ #JVTYPE_DOUBLE`。
* 后果（全部是可观测差异）：
  * `取类型` 对 `{"a":1}` 的 `a` 返回 **1（双精度型）**，不是 2。
  * `取长整数` 得到 `到长整数 (double)`：|x| > 2^53 的整数**丢精度**；`123456789012345678` 会失真。
  * `到文本` 仍输出 **原始 token**（`jv_raw ＝ value`），所以字符串层面看不出差别。
  * `JsonValue_Clone` 一个这样的节点会走 `JsonValue_Double (jsonValue, _GetDouble(...))` **不带 raw**（`[Json]:2950`）→ raw 变成 `到文本 (double)` → **数字文本被重新格式化**（见 §3.5、§9 T-4）。
* DOUBLE 分支（`[Json]:1980-1990`）则一致：根用 `SetDouble`、子用 `JsonValue_Double`，两者都带 `value` 作 raw。

## 2.6 number → double / number → int64 的实际解析函数（必须照抄）

`[Help]:578-649`（`_StrToDoubleW`）—— **整段手写解析已被注释掉**，现役实现只有一行：

```
    返回 (到数值 (zyW2A (str)))
```
（`[Help]:649`，其余 60 行注释掉的代码见 `[Help]:589-647`）

* 语义：UTF-16 → cp936 ANSI 文本 → 易语言核心库 `到数值`（`Val`）→ double。
* 因此 double 的解析细节由 **krnln `到数值`** 决定：它会跳过前导空白、接受 `+/-`、遇第一个非法字符停止（部分解析也算成功，返回已读到的值）、失败返回 0。它**不是** `strtod`：十六进制/`NaN`/`Infinity` 之类行为需实测对齐（§9 T-1）。

`[Help]:651-684`（`_StrToLongW`）—— 同样，手写循环被注释掉，现役：

```
    返回 (_wtoi64 (str))
```
（`[Help]:684`；`_wtoi64` ＝ `ntdll.dll!@_wtoi64`，`[DLL]:253-255`）

* `_wtoi64` 语义：跳过前导空白，可选 `+/-`，读十进制数字直到非数字；**不检测溢出**（溢出为未定义/回绕）；不识别 `0x`。
* 注意 `_StrToLongW` 只用在 LONG 分支（`[Json]:1994,2000`）。DOUBLE 分支一律用 `_StrToDoubleW`。所以 `"1e5"` 的值由 `到数值` 给出。

`取整数` 对 STRING 用 `StrToInt`（shlwapi，返回 BOOL 的疑点见 §9 T-3），对 DOUBLE 用 `到整数`（向零截断）。

---

# 3. 字符串存储（UTF-16、分配者、`_CloneString`、ANSI 转换）

## 3.1 内部一律 UTF-16LE

* `JVTYPE_STRING` 的 `jv_value` ＝ `PWSTR`（UTF-16LE，以单个宽字符 0 结尾），长度 `字符数 × 2 ＋ 2` 字节。
* 对象成员名 `jv_key`、元数据 `jv_raw`、路径解析出的 step name 也都是 UTF-16LE。
* **没有任何 UTF-8/ANSI 的内部存储**：`A` 版入口在进门时一次性 `zyA2W` 转成 UTF-16，出口一次性 `zyW2A_Point` 转回去；带 `UTF8` 后缀的入口（`置代码UTF8`/`K_UTF8`/`置文本UTF8`/`取文本UTF8`）先经外部模块 `UTF82W_T`/`W2UTF8_T` 转 UTF-16 再走同一内部路径（`[Cls]:90-103,520-532,905-917,1108-1127`）。
  → **UTF-8 支持不是本库内部实现的**，C++ 版可以把内部表示定为 `std::u16string`/`char16_t*`，只在边界做转换。

## 3.2 分配者：全部走 `HeapAlloc(hHeap, HEAP_ZERO_MEMORY, …)`

```
.子程序 _Alloc, 整数型
    .参数 Size, 整数型
    .参数 hHeap, 整数型, 可空
    .如果真 (hHeap ＝ 0)
        hHeap ＝ GetProcessHeap ()
    .如果真结束
    返回 (HeapAlloc (hHeap, #HEAP_ZERO_MEMORY, Size))
```
（`[Help]:216-223`；`_Free` 同理走 `HeapFree(hHeap, 0, addr)`，`[Help]:225-233`）

所以：**字符串的堆 ＝ 持有它的节点的 `hHeap`**（子节点继承父节点的堆），除非显式传 0（则 `GetProcessHeap()`）。`HEAP_ZERO_MEMORY` 保证末尾 `{0,0}` 天然存在，这是很多地方只拷 `len*2` 字节却仍能当 C 字符串用的原因。

## 3.3 `_CloneString`（按 `lstrlenW` 截断）与 `_CloneStringPtr`（按显式长度）

```
.子程序 _CloneString, 整数型
    .参数 str, 字节集
    .参数 hHeap, 整数型, 可空
    .局部变量 len, 整数型
    .局部变量 p, 整数型
    .如果真 (hHeap ＝ 0)
        hHeap ＝ GetProcessHeap ()
    .如果真结束
    len ＝ lstrlenW (str) × 2
    p ＝ HeapAlloc (hHeap, #HEAP_ZERO_MEMORY, len ＋ 2)
    RtlMoveMemory (p, _取指针字节集 (str), len)
    返回 (p)
```
（`[Help]:235-247`）

```
.子程序 _CloneStringPtr, 整数型
    .参数 str, 整数型
    .参数 strLen, 整数型
    .参数 hHeap, 整数型, 可空
    ...
    .如果真 (strLen ＜ 0) ' 此处修复一个BUG  创想软件  部分情况下strlen为-1 会导致内存分配失败导致崩溃
        strLen ＝ 0
    .如果真结束
    p ＝ HeapAlloc (hHeap, #HEAP_ZERO_MEMORY, strLen × 2 ＋ 2)
    RtlMoveMemory (p, str, strLen × 2)
    返回 (p)
```
（`[Help]:249-265`）

| 调用场景 | 用哪个 | 后果 |
| --- | --- | --- |
| 对象成员名 `jv_key` | `_CloneString`（`[Hash]:261`）／`_CloneStringPtr (key, lstrlenW_Ptr (key))`（`[Hash]:198`，Ptr 版 key 已是指针） | 名字遇到内嵌 0 就截断 |
| 解析出来的字符串值 | **`_CloneStringPtr`**（`JsonValue_String` `[Json]:610`、`JsonValue_SetString` `[Json]:758`） | 按**显式字符数**拷贝 → 内嵌的 `\u0000` 之后的内容**仍被存进内存**，但所有读取端（`lstrlenW`）会在 0 处截断 → 输出、`取文本` 只看到前半段 |
| `jv_raw`（null/true/false/数字/undefined） | `_CloneString`（`[Json]:203,241,282,285,363,366,490,528,545,564`） | raw 里没有 0，安全 |
| `创建文本W`/`置文本W`（`JsonValue_StringW/SetStringW`） | `_CloneString` → **入参 `字节集` 必须以 `{0,0}` 结尾**，否则长度由内存里的第一个 0 决定（可能越界读） |

**读回**（`JsonValue_GetStringW` 等）统一是 `指针到字节集 (pStr, lstrlenW_Ptr (pStr) × 2 ＋ 2)`（`[Json]:682,722,576`）→ 返回值**带 `{0,0}`**，长度＝字符数×2+2。`JsonValue_GetString`（不带 W）返回裸指针（`[Json]:656-666`），**类型不是 STRING 时返回 0**（不做类型转换）。

## 3.4 文本（ANSI/GBK）→ UTF-16 的转换与「非法字节」处理

```
.子程序 zyA2W, 字节集
    .参数 参数_Ansi文本, 文本型
    .参数 参数_代码页, 整数型, 可空, 默认为：936(简体中文)
    ...
    .如果真 (是否为空 (参数_代码页))
        参数_代码页 ＝ 936
    .如果真结束
    局部_Ansi长度 ＝ 取文本长度 (参数_Ansi文本) ＋ 1
    局部_Unicode长度 ＝ MultiByteToWideChar (参数_代码页, 0, 参数_Ansi文本, 局部_Ansi长度, ret, 0) × 2
    ret ＝ 取空白字节集 (局部_Unicode长度)
    .如果真 (MultiByteToWideChar (参数_代码页, 0, 参数_Ansi文本, 局部_Ansi长度, ret, 局部_Unicode长度) ＝ 0)
        返回 ({ 0, 0 })
    .如果真结束
    返回 (ret)
```
（`[Help]:4-20`）

要点（C++ 必须照做，否则长度语义不同）：

1. **代码页默认 936**；所有 `A` 版入口都不传第二个参数 → 永远是 cp936（**不是** CP_ACP，中文系统上两者相同，其他系统不同）。
2. `cchMultiByte ＝ 取文本长度 ＋ 1` → **把源串的终止 `\0` 也转换**，所以 `zyA2W` 的结果**自带 `{0,0}`**（这是 `_CloneString` 能安全工作的前提）。
3. `dwFlags ＝ 0` → **非法字节序列不会让函数失败**：MBTWC 在 `dwFlags=0` 下用默认字符替代/丢弃非法序列并返回已转换的宽字符数；只有严重错误（代码页不可用、缓冲区不足等）才返回 0。返回 0 时 `zyA2W` 返回 `{ 0, 0 }`（**长度 2 的空宽串**）→ 上层 `lstrlenW ＝ 0` → 得到空字符串，而不是报错。
   * 也就是说：**GBK 非法字节 → 静默降级**（表现为字符丢失或整串变空），不会返回错误码。C++ 版建议用 `MultiByteToWideChar(936, 0, …)` 保持同样容错，不要改成 `MB_ERR_INVALID_CHARS`。
4. 反方向 `zyW2A_Point`（`[Help]:57-72`）用 `cchWideChar ＝ -1`（自动按 0 终止）+ `WideCharToMultiByte_ptr`，`lpDefaultChar/lpUsedDefaultChar` 都传 0 → 不可映射字符用系统默认（`?`）替代；**第二次的返回值被忽略**（不判失败）。`zyW2A`（`[Help]:38-54`）则用 `取字节集长度 ÷ 2` 作长度（若入参带 `{0,0}` 就把终止符也转成 `\0`），失败返回 `""`。
5. `MultiByteToWideChar_ptr`/`zyString_AddPtr`（`[Str]:115-150`）提供「指针 + 显式字节长度」版本，默认也是 cp936；`zyString_AddPtr` 在 `len ＝ 0` 时会直接返回假（`MultiByteToWideChar_ptr ＝ 0`）。

## 3.5 `jv_raw`：元数据 ＝ 输出时的「原文回放」

| 类型 | 序列化时输出什么 | 代码 |
| --- | --- | --- |
| NULL | 固定字面量 `null`（**不读 raw**） | `[Json]:2212` |
| BOOL | 固定 `true`/`false`（`JsonValue_GetBool`） | `[Json]:2214` |
| DOUBLE / LONG / UNDEFINED | **`JsonValue_GetRawW (jsonValue)` 原样**（UTF-16） | `[Json]:2216-2220` |
| STRING | `"` ＋（转义后的）内容 ＋ `"` | `[Json]:2221-2231` |
| ARRAY / OBJECT | `[` … `]` / `{` … `}` | `[Json]:2233-2291` |

`Double/Long/Bool/Null/Undefined` 构造时若 `rawData ＝ { }`（可空/未传）则 **raw ＝ `zyA2W (到文本 (数值))`**：

```
.如果 (rawData ≠ { })
    _Set (this, #jv_raw, _CloneString (rawData, hHeap))
.否则
    raw ＝ zyA2W (到文本 (double))
    _Set (this, #jv_raw, _CloneString (raw, hHeap))
.如果结束
```
（`[Json]:281-286`；LONG 是 `[Json]:362-367`，`SetInt` 是 `[Json]:463-468`，Bool 是硬编码 `"true"/"false"` 字节集 `[Json]:489`）

→ **数字的输出文本有两种来源**：解析得到的节点保留输入 token（逐字符一致）；程序写入（`置双精度`/`置数值`/`置整数`/`置长整数`）或 `JsonValue_Clone` 得到的节点用 **易语言 `到文本(双精度小数型)`** 的格式。C++ 要「输入相同→输出相同」，必须：
1. 解析路径：raw 保存原始 token（含前导零、`+`、`E`、多余的 `.` 等），输出时**不做任何规范化**；
2. 程序写入/克隆路径：实现一个与 EPL `到文本(双精度)` **逐字符一致**的格式化函数（需实测标定，§9 T-4）。

---

# 4. 对象成员顺序（最关键的一节）

## 4.1 结论

**对象成员的顺序 ＝ 「第一次插入该键」的顺序（插入序，insertion-ordered map）。**

* `到文本` / `取所有键` / `取所有值` / `Clone` / `合并对象` / `JsonValue_SortObj` 全部按这个顺序；
* 重复键**就地替换值并保留原位置**（不追加到末尾、不改变序号）；
* 只有显式 `排序对象` / `排序` 才会打乱它（改的是这条有序数组，不是哈希桶）；
* 顺序**与哈希值、桶号、扩容次数完全无关**。

## 4.2 证明：哈希表 + 一条平行的有序数组

`HashEx_Create` 在建桶数组之外**同时建了一条 Array**（`[Hash]:4-43`）：

```
table ＝ HeapAlloc (hHeap, #HEAP_ZERO_MEMORY, initialCapacity × 4)
_Set (hHashTable, #ht_table, table) ' 数组，每个成员是一个链表
_Set (hHashTable, #ht_tableLenth, initialCapacity)
_SetFloat (hHashTable, #ht_loadFactor, loadFactor)
_Set (hHashTable, #ht_threshold, initialCapacity × loadFactor) ' 扩容临界值

array ＝ Array_Create (hHeap) ' 同时创建一个数组，为了能够获取加入时顺序
_Set (hHashTable, #htEx_eArray, array)
```

* 结构偏移：`ht_table=4`、`ht_tableLenth=8`、`ht_count=12`、`ht_threshold=16`、`ht_loadFactor=20`、`ht_isNewHeap=24`，扩展字段 `htEx_eArray=28`，总大小 `htEx_cbSize=32`（`[Const]:43-53`）。
* 默认参数：`initialCapacity ＝ 11`、`loadFactor ＝ 0.75`（`[Hash]:13-18`）→ `threshold ＝ int(11 × 0.75) ＝ 8`（写入 `#ht_threshold` 时按整数存，`_Set` 只存低 4 字节）。
* 插入时**两写**：桶链头插 + `Array_Add (array, value)`（`[Hash]:196-202`）：

```
' 插入链表
_Set (value, #jv_hashCode, hash)
_Set (value, #jv_key, _CloneStringPtr (key, lstrlenW_Ptr (key), hHeap))
_Set (value, #jv_next, _GetTableItem (table, index))
_SetTableItem (table, index, value)
Array_Add (array, value) ' 同时加入到数组中
_incPtr (hHashTable, #ht_count)
```

**所有「顺序相关」的读取函数都走 eArray，不走桶链**：

`HashEx_GetKeys`（`[Hash]:597-623`）—— `到文本`/`取所有键` 的顺序来源：

```
count ＝ _Get (hHashTable, #ht_count)
array ＝ _Get (hHashTable, #htEx_eArray)
重定义数组 (keys, 假, count)
.计次循环首 (count, i)
    pe ＝ Array_Get (array, i － 1)
    key ＝ _Get (pe, #jv_key)
    keys [i] ＝ 指针到字节集 (key, lstrlenW_Ptr (key) × 2 ＋ 2)
.计次循环尾 ()
返回 (count)
```

`HashEx_GetValues`（`[Hash]:626-651`）—— 直接 `RtlMoveMemory (pValues, _Get (array, #arr_table), count × 4)`，即**把 eArray 的数据段整块拷出**，所以顺序与 `GetKeys` 完全一致（`取所有值`、`JsonValue_Destory/Clear` 的释放顺序、`Clone`、`CombineProc` 都依赖它）。

`HashEx_GetAt`（`[Hash]:390-400`）＝ `Array_Get (eArray, index)` → **按位置访问对象成员**（`JsonValue_GetAtReadOnly` 对 OBJECT 用它，见 §5.6）。

`序列化调用点`（`[Json]:2259`，在 `JsonValue_ToStringProc` 的对象分支）：

```
count ＝ HashEx_GetKeys (val, names)
indent ＝ indent ＋ 1
.计次循环首 (count, i)
    ...写引号、names[i]、引号、冒号...
    JsonValue_ToStringProc (HashEx_Get (val, names [i]), zyString, autoFormat, indent, isTrans)
```

> 注意它是「按 eArray 取 key → 再用 key 回哈希查值」，而不是直接取 eArray 里的节点指针。因为键唯一，两者等价；但**若某成员的 key 被外部改写**（源码里没有这种路径）才会不一致。C++ 版可安全地直接按有序序列迭代。

**唯一的桶链遍历器是 `HashEx_Enum`**（`[Hash]:654-680`），它按 `table[0..len-1]` 逐桶逐链回调 → **哈希序**。全项目搜索：`HashEx_Enum` **无任何调用点**（死代码）→ 不影响任何输出。移植时不要用它实现枚举。

## 4.3 哈希函数（仅影响桶，不影响顺序）

```
.子程序 HashEx_GetHashCode, 整数型
    .参数 key, 整数型
    .参数 len, 整数型
    置入代码 ({ 86, 81, 49, 192, ... })
    ' push esi
    ' xor eax,eax
    ' mov esi,[ebp+08]
    ' cmp esi,0
    ' jz goback
    ' mov ecx,[ebp+12]
    ' jecxz goback
    ' lp:
    ' imul eax,31
    ' movzx ebx,byte [esi+ecx-1]
    ' add eax,ebx
    ' loop lp
```
（`[Hash]:336-359`）

* 逐 **字节** 从**尾部**往前：`h = h*31 + buf[len-1-i]`（`ecx` 从 `len` 递减，取 `[esi+ecx-1]`）。
* `len` 一律传 `lstrlenW (key) × 2`（字节数，`[Hash]:164,228,378,417,467,491,525`）→ 每个 UTF-16 代码单元贡献「低字节、高字节」两个字节，且**高字节在前**（因为倒序遍历）。
* 桶号：`index ＝ 位与 (hash, 2147483647) ％ tableLenth`（把负哈希折成正数后对长度取模）。
* 比较：先比 `jv_hashCode ＝ hash`，再 `lstrcmpW (stored_key, probe_key) ＝ 0`（区分大小写的代码单元序）。
* 扩容 `_Hash_Rehash`（`[Hash]:106-141`）：`newCapacity ＝ oldCapacity × 2 ＋ 1`（11→23→47→95→…），重挂桶链（保持每条旧链的前到后顺序），**完全不碰 eArray** → 扩容不改变成员顺序。
  * C++ 版若要完全对齐内部状态，请用同样的容量序列；但**输出层面只依赖 eArray**，所以哈希细节可以随意（只要查找正确）。

## 4.4 重复键：`置属性`/`合并`/解析时的 `Put` 语义

`HashEx_Put`/`HashEx_PutPtr`/`HashEx_Insert` 三者的「已存在」分支**代码相同**（`[Hash]:168-181`、`[Hash]:232-244`、`[Hash]:296-308`）：

```
.判断循环首 (pe ≠ 0)
    .如果真 (_Get (pe, #jv_hashCode) ＝ hash 且 lstrcmpW (_Get (pe, #jv_key), pKey) ＝ 0) ' 存在相同键，则替换旧值
        .如果 (prev ≠ 0) ' 有上一个节点
            _Set (prev, #jv_next, value) ' 则将上一个节点的next指向当前节点
        .否则
            _SetTableItem (table, index, value) ' 替换到头节点
        .如果结束
        _Set (value, #jv_key, _Get (pe, #jv_key)) ' 将被替换节点的key保存下来
        _Set (value, #jv_next, _Get (pe, #jv_next)) ' 将next保存下来
        _Set (value, #jv_hashCode, hash)
        ' 同时改变数组的值
        Array_ReplaceValue (array, pe, value)
        返回 (1)
    .如果真结束
    prev ＝ pe
    pe ＝ _Get (pe, #jv_next)
.判断循环尾 ()
```

规则：

1. **就地替换**：新节点接管旧节点的 **key 指针**（不重新分配 key，所以 key 的字符串内容/大小写**保持第一次插入时的那个**）与桶链后继；`Array_ReplaceValue` 在线性找到 eArray 里等于 `pe` 的那一格并写入 `value` → **位置不变**（`[Arr]:324-347`）。
2. 返回值三态：`0`＝新插入、`1`＝覆盖了旧值、`-1`＝句柄为 0 失败（`[Hash]:143,206` 注释）。上层 `JsonValue_Set*` 判的是 `ret ≠ -1`（`[Json]:1207`）。
3. **大小写以第一次为准**：`置属性("A",1)` 后 `置属性("a",2)` → 键仍是 `"A"`（因为新节点接管旧 key 指针），值变 2，位置仍在第一位。C++ 必须复现（很多库在这里做成「覆盖成新 key」）。
4. `HashEx_Insert (pMap, idx, key, jsonVal)`（用于 `插入属性`）：先做上面这段查重；**键已存在时 `idx` 被完全忽略**（直接返回 1）。只有新键才执行 `Array_Insert (array, idx, value)`（`[Hash]:323-325`），`idx < 0 或 idx > count` → 返回 -1 → `插入属性` 返回假。
5. 内存副作用（不影响输出）：覆盖路径下**被替换节点的 key 指针**如果原本非 0（该节点曾挂在别的对象里），那个克隆串会泄漏；旧节点对象本身则由 `JsonValue_Set*` 末尾的 `Release` 回收，但它的 key 已被新节点接管，所以**不会** double free。

## 4.5 顺序变化的全部触发点汇总

| API（`[Cls]`） | 底层 | 顺序效果 |
| --- | --- | --- |
| `置文本/置长整数/置属性/合并对象/…` | `JsonValue_SetA/W` → `HashEx_Put` | 新键**追加到末尾**；旧键**原地替换** |
| `K`/`KW`/`K_UTF8`（键不存在） | `JsonValue_GetW` → `JsonValue_Null` + `HashEx_Put`（`[Json]:1095-1103`） | **读取即插入**：`"key":null` 被追加到末尾 → 之后再 `到文本` 会多出这个成员！ |
| `插入属性(W)` | `HashEx_Insert` | 在有序数组的 `idx` 处插入（仅新键） |
| `移除属性(W)` | `HashEx_Remove` + `Release(child)`（`[Json]:2763-2815`） | `Array_RemoveFromValue` 左移补位 → 其余顺序不变 |
| `排序对象(W)` | `JsonValue_SortObj` → `Array_Sort (eArray, SortProcAsc/Desc, 0, 0)`（`[Json]:2881-2926`） | 按 `jv_key` 的 `lstrcmpW`（**UTF-16 代码单元序，区分大小写，非区域敏感**）升/降序 |
| `排序(W)`（自定义比较） | `JsonValue_Sort`：OBJECT 排 `htEx_eArray`，ARRAY 排 `jv_value` 数组（`[Json]:2846-2878`） | 自定义序；比较回调 `(JsonValue1, JsonValue2, userData) → <0/0/>0` |
| `清空(W)` | `HashEx_Clear` | 全部移除，顺序表清空（`jv_value` 句柄保留 → 仍输出 `{}`） |
| `分离` / 子节点 `Release` | `JsonValue_Detch` → `HashEx_RemoveFromValue` | 该成员从有序数组中补位移除 |
| `复制`/`Clone` | `HashEx_GetValues` 顺序逐个 `JsonValue_Set`（`[Json]:2965-2973`） | **保持原顺序**（含被 `排序对象` 改过的顺序） |

`排序` 的两个必须照抄的细节：

1. `JsonValue_Sort` 形参有 `algorithm`，但**两次调用都写死 0**（`[Json]:2864,2873`：`Array_Sort (array, sortProc, userData, 0)`）→ 「希尔排序」选项**永远不生效**（`api-spec.md` H-7）。
2. `排序对象(W)` 的参数是 `参数_降序`，实际传 `取反 (参数_降序)` 给 `asc`（`[Cls]:1353,1365`）→ `排序对象(, 真)`＝降序。

## 4.6 序列化顺序相关的两个「非直觉」输出规则

* `HashEx_GetSize` 用的是 `ht_count`（`[Hash]:96-103`），`取键数量`/`JsonValue_GetLength` 用它（`[Json]:1310-1315`）→ 与 eArray 长度一致（两处始终同步增减）。
* `HashEx_Destory`/`HashEx_Clear` 的 key 释放遍历桶链（`[Hash]:70-82`、`[Hash]:700-708`）→ 若某节点在「替换」中被摘出但仍被别处引用，其 key 的归属容易出错；这只影响内存，不影响输出。

---

# 5. 数组存储（Array 句柄）

## 5.1 结构与偏移

```
.常量 arr_cbSize, "20", , ArrayStruct
.常量 arr_hHeap, "0"
.常量 arr_count, "4"
.常量 arr_table, "8"           ' 见 [Const]:70
.常量 arr_tableLength, "12"
.常量 arr_isNewHeap, "16"
```
（`[Const]:67-72`）

`hArray` 指向 20 字节结构；`arr_table` 是一块 `tableLength × 4` 字节的**指针数组**（元素是 `JsonValue*`）。

## 5.2 创建与容量增长

```
.子程序 Array_Create, 整数型
    .参数 hHeap, 整数型, 可空
    .参数 initCount, 整数型, 可空, 默认为：0
    .参数 initCapacity, 整数型, 可空, 初始缓存容量 默认为：8
    ...
    .如果真 (initCount ＜ 0 或 initCapacity ＜ 0)
        返回 (0)
    .如果真结束
    .如果真 (hHeap ＝ 0)
        hHeap ＝ HeapCreate (0, 0, 0)
        isNewHeap ＝ 1
    .如果真结束
    hArray ＝ HeapAlloc (hHeap, #HEAP_ZERO_MEMORY, #arr_cbSize)
    _Set (hArray, #arr_tableLength, initCapacity)
    table ＝ HeapAlloc (hHeap, #HEAP_ZERO_MEMORY, initCapacity × 4)
```
（`[Arr]:4-34`）

增长（`[Arr]:136-157`）：

```
.子程序 _Array_ReArray, , , 动态增加成员
    .参数 count, 整数型, , 指定数量，不小于这个数量
    ...
    newCapacity ＝ oldCapacity × 2 ＋ count ＋ 1
    newTable ＝ HeapAlloc (hHeap, #HEAP_ZERO_MEMORY, newCapacity × 4)
    RtlMoveMemory (newTable, oldTable, oldCapacity × 4)
```

* 容量序列（单元素增长）：8 → 17 → 35 → 71 → 143 → …（`× 2 ＋ 1`）。
* 批量增长用 `count ＝ 待加入元素数`（`Array_AddArray`/`Array_InsertArray`，`[Arr]:201,234`）。
* 触发条件都是 **`count ≥ tableLength`**（`≥`，不是 `>`）：`Array_Add`（`[Arr]:171`）、`Array_Insert`（`[Arr]:268`）。
* **增长只搬 `oldCapacity` 项**（不是 `count` 项）→ 新槽位靠 `HEAP_ZERO_MEMORY` 天然为 0。

## 5.3 索引语义与逐项操作

| 操作 | 边界/语义 | 代码 |
| --- | --- | --- |
| `Array_Add (hArray, value)` | 追加到 `[count]`，`count++`，**返回 0-based 索引**；失败 -1。**不检查 value 是否为 0** | `[Arr]:159-177` |
| `Array_Get (hArray, index)` | `index ＜ 0 或 index ≥ count` → **静默返回 0**（不报错） | `[Arr]:282-299` |
| `Array_Set (hArray, index, value)` | 越界 → 假；**只覆盖，不释放旧值** | `[Arr]:302-322` |
| `Array_Insert (hArray, index, value)` | `index ＜ 0 或 index ＞ count` → -1（**允许 `index ＝ count`＝追加**）；`index ＜ count` 时把 `[index..count-1]` 右移一格再写 `[index]`，`count++` | `[Arr]:249-279` |
| `Array_Remove (hArray, index)` | 越界 → 假；把 `[index+1..count-1]` 左移一格，末槽 `ZeroMemory` 4 字节，`count--` → **顺序紧凑、无空洞** | `[Arr]:371-390` |
| `Array_RemoveFromValue (hArray, value)` | 线性找**第一个**指针相等的槽，左移补位，`count--`，返回真 | `[Arr]:392-417` |
| `Array_ReplaceValue (hArray, oldValue, value)` | 线性找第一个等于 `oldValue` 的槽写入 `value`（**位置保持**） | `[Arr]:324-347` |
| `Array_Clear` | `ZeroMemory (table, tableLength × 4)` ＋ `count ＝ 0`（**保留句柄与容量**） | `[Arr]:358-368` |
| `Array_ReDim (hArray, newCount, keepOldData)` | `newCapacity ＝ newCount × 2`；`keepOldData` 时拷 `min(oldCount, newCount)` 项；**直接把 `count` 设成 `newCount`**（新的尾部是 0 指针） | `[Arr]:96-134` |
| `Array_GetValues` | `重定义数组 (values, 假, asCount)` ＋ 整块拷 `asCount × 4` | `[Arr]:419-436` |
| `Array_GetCount` | 返回 `arr_count` | `[Arr]:349-355` |

`Array_InsertArray`（`[Arr]:212-247`）先搬尾部再写新块，注意它**不校验 `index`**（越界会内存写坏）——但没有任何 Json 层调用它，属死代码。

## 5.4 `JsonValue` 层的数组语义（`到文本`/`I`/`置成员` 的真正行为）

* **`JsonValue_Append`**（`[Json]:867-887`）：`jsonValue ＝ 0 或 jsonVal ＝ 0` → 假；若当前类型不是 ARRAY → 先 `JsonValue_SetArray`（**这会销毁原值**，见 §5.5）；然后 `_Set (jsonVal, #jv_parent, jsonValue)` ＋ `Array_Add`。返回 `Array_Add ≠ -1`。
* **`JsonValue_Insert`**（`[Json]:891-911`）：同上但用 `Array_Insert (pArray, index, jsonVal)` → **`index` 必须 0..count**，否则假。`插入成员(W)` 走它。
* **`JsonValue_GetAt`（自动创建/补齐版，`I`/`IW`/`选择` 的 INDEX 步）**（`[Json]:913-947`）：

```
.如果真 (_Get (jsonValue, #jv_type) ≠ #JVTYPE_ARRAY)
    .如果真 (JsonValue_SetArray (jsonValue) ＝ 假)
        返回 (0)
.如果真结束
count ＝ Array_GetCount (pArray)
.如果真 (index ≥ count) ' 如果超出范围添加null值
    newCount ＝ index ＋ 1
    .判断循环首 (count ＜ newCount)
        jv ＝ JsonValue_Null (jsonValue)
        Array_Add (pArray, jv)
        _Set (jv, #jv_parent, jsonValue)
        count ＝ count ＋ 1
    .判断循环尾 ()
.如果真结束
返回 (Array_Get (pArray, index))
```

  * **越界读会变成写**：`I(5)` 在一个只有 2 个成员的数组上会**补 4 个 `null`**（索引 2、3、4、5）并返回 `#5`；文档化的「注意：路径不存在的对象会自己创建」正是此意。
  * **负索引**：`index ≥ count` 不成立 → 落到 `Array_Get` → 越界返回 **0** → `置指针 (0)` 直接返回 → 得到一个「空 Json2」（指针 0），不报错、不创建。
  * `Array_Add` 时 `jv_parent` 是在 Add **之后**设置的（对 `JsonValue_Null` 而言顺序无关）。

* **`JsonValue_SetAt`（`置成员(W)`）**（`[Json]:985-1041`）：
  * 非 ARRAY → `SetArray`；
  * `index ≥ count` → 用 `JsonValue_Null` 补到 `index`，第 `index` 格放 `jsonVal`（逐个 `Array_Add`，任一失败则把刚建的 null `Release` 后返回假）；
  * `index ＜ count` 且旧值非 0 → `Release(旧值)`（旧值经 Detch 自己从数组摘掉，后面的元素左移）→ `Array_Insert (pArray, index, jsonVal)`（在新空出的位置插入）；
  * 旧值为 0（理论不会发生）→ `Array_Set`。
  * 净效果＝**原地替换**，长度不变。
* **`JsonValue_GetAtReadOnly`（`I2`/`I2W`/`取所有值` 等「不创建」版）**（`[Json]:950-982`）：
  * ARRAY → `Array_Get`（越界 0）；
  * **OBJECT → `HashEx_GetAt`＝按 eArray 位置取成员**（`I2(0)` 可以取到对象的第一个成员！）；
  * 其它类型 → 0。
  * 注意与 `JsonValue_GetAt` 的不对称：`GetAt` 对 OBJECT 会**销毁对象**变成数组，`GetAtReadOnly` 对 OBJECT 是按位置只读。
* `JsonValue_RemoveAt`（`移除成员(W)`，`[Json]:2819-2843`）：`Array_Get` 越界 → 0 → 返回假；否则 `Array_Remove` ＋ `Release(child)`（child 的 Detch 再摘一次 → `Array_RemoveFromValue` 已找不到，安全）。
* `JsonValue_ContainsAt`（`[Json]:2656-2678`，**死代码**，无调用点）：要求类型是 ARRAY 且 `count ≠ 0`，返回 `index ≥ 0 且 index ＜ count`。

## 5.5 类型转换时的销毁（`置数组` 与 `SetObject`）

```
.子程序 JsonValue_SetArray, 逻辑型, , 设置为数组
    type ＝ _Get (jsonValue, #jv_type)
    .如果真 (type ≠ #JVTYPE_ARRAY)
        JsonValue_Destory (jsonValue)
        _Set (jsonValue, #jv_value, Array_Create (hHeap))
        _Set (jsonValue, #jv_type, #JVTYPE_ARRAY)
```
（`[Json]:811-826`；`JsonValue_SetObject` 同形 `[Json]:847-864`）

* **同类型时什么都不做**（幂等，保留原内容）→ `置数组` 对一个数组是 no-op，`置对象` 对一个对象也是 no-op。
* 异类型时 `JsonValue_Destory` 递归释放旧内容（对象成员/数组成员/字符串/raw），然后建新容器 → `jv_key`/`jv_parent`/`jv_hashCode`/`jv_next` **保持不变**（节点仍在父对象的原位置上）。
* `hHeap` 在 `SetArray` 里未初始化（→0，新建私有堆），在 `SetObject` 里正确取节点 hHeap。见 §1.5。

## 5.6 排序算法（顺序敏感，必须逐句复现）

`Array_Sort`（`[Arr]:439-457`）：

```
.判断开始 (algorithm ＝ 0)
    _Array_FastSort (table, asCount, compareFunction, lParam, 0, asCount － 1)
.默认
    _Array_ShellSort (table, asCount, compareFunction, lParam)
.判断结束
```

* `asCount ＝ 0` 时 `right ＝ -1`，`_Array_FastSort` 首行 `left ≥ right` → 直接返回（安全）。
* 比较通过 `zyCall3 (proc, v1, v2, lParam)`（3 个 int 参数，返回 int）。**非整数型成员传的是指针**（注释明示 `[Arr]:441`），但 Json 层传的都是 `JsonValue*` 整数值。
* `_Array_FastSort`（`[Arr]:460-513`）是「三方取中 + 挖坑法 Hoare 分区」，**不稳定**：
  * `m ＝ left ＋ (right － left) ÷ 2`（这里是 `÷`＝普通除法赋给整数型 → 向零截断）；
  * 三次 `zyCall3(...) ＞ 0` 判断后 `_Xchg` 把 left/m/right 三端排好；
  * 基准值取 `table[left]`（注意：**在端点交换之后**才取，`[Arr]:492`）；
  * 内层 `判断循环首 (i ＜ j 且 cmp (table[j], 基准) ≥ 0)` → `j--`；不等式边界（`≥ 0` / `≤ 0`）决定相等元素是否停止移动 → **直接影响最终排列**，C++ 若写标准 quicksort 就会在「比较返回 0」的元素上给出不同次序；
  * 结束时把基准值写回 `table[i]`，递归 `(left, i-1)` 与 `(i+1, right)`。
* `_Array_ShellSort`（`[Arr]:516-547`）的 `gap` **初值 0**（局部变量未初始化）：`gap ＜ count` 循环先 `gap ＝ gap × 3 ＋ 1` → 1, 4, 13, …；且**内层用了 `_Xchg` 却又在末尾用 `RtlMoveMemory` 回写 `value`**（注释掉的版本才是标准插入排序）；因 `JsonValue_Sort` 写死 0，**这段永远不会被执行**（除 `Array_Sort` 的其它潜在调用者，本库没有）。

→ **验收建议**：用 `排序` + 自定义比较（对相等元素返回 0）跑一个 ≥6 元素的数组，把 EPL 产物的次序当作黄金用例；C++ 必须实现同一份 quicksort（同样的 `m`、同样的 `≥0/≤0`、同样的挖坑写回）才能一致。

---

---

# 6. 解析器 `JsonValue_Parsed3` 完整状态机

## 6.1 入口、参数、返回

```
.子程序 JsonValue_Parsed3, 整数型
    .参数 jsonVal, 整数型          ' 目标根节点（必须是已存在的 JsonValue；它的 hHeap 被所有子节点继承）
    .参数 pStr, 整数型             ' UTF-16LE 指针
    .参数 strLen, 整数型           ' 宽字符个数（不含终止符）
    .参数 transU, 逻辑型           ' ＝ 参数_Unicode解码：是否解码 \uXXXX
    .参数 errorPos, 整数型, 参考 可空
```
（`[Json]:1362-1367`）

* 返回 **错误码**（`#JSON_OK`＝0 才算成功），不是布尔：`JsonValue_Combine` 判的就是 `≠ 0`（`[Json]:2333`）。
* 调用方及长度来源：
  * `置代码`/`置代码W`：`_取文本长度W (str)`（＝`lstrlenW`）（`[Cls]:84,145`）
  * `解析`/`解析W`：`lstrlenW (str)`（`[Cls]:287,304`）
  * `合并`/`合并W`：先 `JsonValue_Object (jsonValue)` 建一个**临时新对象**，把文本解析进去，再和本对象合并（`[Json]:3222-3237`）
* 前置检查（`[Json]:1377-1386`）：

```
errorPos ＝ 0
.如果真 (jsonVal ＝ 0 或 pStr ＝ 0 或 strLen ＝ 0)
    返回 (#JSON_INVALIDARG)
.如果真结束
error ＝ #JSON_OK
hHeap ＝ _Get (jsonVal, #zyobj_hHeap)
hStack ＝ Stack_Create (hHeap)
.如果真 (hStack ＝ 0)
    返回 (#JSON_MEMORY)
.如果真结束
JsonValue_Clear (jsonVal) ' 清除源数据，如果后面执行失败也会清除
namePos ＝ -1
nameLen ＝ 0
i ＝ 0
```
→ **空文本 / 空指针 → `-2 JSON_INVALIDARG`（`errorPos` 已被置 0）**；解析**开始前**就清空目标节点（所以失败会留下一个被清空但类型未变的节点，见 §6.9）。

## 6.2 主循环（顶层分发器）

```
.判断循环首 (i ≤ strLen) ' 10170
    wchar ＝ _指针到短整数U (pStr ＋ i × 2)
    .判断开始 (wchar ＝ 32 或 wchar ＝ 9 或 wchar ＝ 13 或 wchar ＝ 10 或 wchar ＝ 0)
        i ＝ i ＋ 1
        到循环尾 ()
    .判断 (wchar ＝ 123) ' {
        error ＝ jsonValue_parsed_object (jsonVal, pStr, hStack, namePos, nameLen, transU)
        .如果真 (error ≠ #JSON_OK)
            跳出循环 ()
        .如果真结束
    .判断 (wchar ＝ 91) ' [
        error ＝ jsonValue_parsed_array (jsonVal, pStr, hStack, namePos, nameLen, transU)
        ...
    .默认
        error ＝ jsonValue_parsed_other (jsonVal, pStr, strLen, hStack, namePos, nameLen, transU, i)
        ...
    .判断结束
    i ＝ i ＋ 1
.判断循环尾 ()
```
（`[Json]:1391-1419`）

要点：

1. **循环条件是 `i ≤ strLen`**（不是 `<`）→ 会多读一个位置（终止符 0）。因为 `wchar ＝ 0` 被当作空白跳过，所以「文本里出现任何 `\0`」之后的内容**不会被跳过**：`i ≤ strLen` 只保证读到 `strLen`，但**空白分支会让 `i` 一直 ++ 到 `strLen+1` 才退出**，而 `jsonValue_parsed_other` 的分支里没有 `\0` 特判 → 只要 token 扫描到达终止符，`pos ＜ strLen` 会自然结束内层循环。
2. **顶层空白集只有 5 个：`0x20`、`0x09`、`0x0D`、`0x0A`、`0x00`**。**`0x0B`（垂直制表）、`0x0C`（换页）、`0x00A0`（NBSP）、`0xFEFF`（BOM）都不是空白**，会被送进 `jsonValue_parsed_other` 并导致错误。
3. **`{` 与 `[` 由主循环直接识别**（无需前置 token）；其它一切字符（包括 `}`、`]`、`,`、`:`、引号、字母、数字）都进 `jsonValue_parsed_other`。
4. `namePos`/`nameLen` 是 `JsonValue_Parsed3` 的**局部变量**，以「值」传给 `parsed_object`/`parsed_array`、以「参考」传给 `parsed_other` → 待写入的属性名跨调用保持（`[Json]:1637-1638`）。初值 `-1 / 0`，被 `parsed_other` 在成功写入成员后重置（`[Json]:2047-2048`）。

### 收尾（`[Json]:1422-1440`）

```
.如果真 (error ≠ #JSON_OK)
    Stack_Destroy (hStack)
    JsonValue_Clear (jsonVal)
    errorPos ＝ i
    返回 (error)
.如果真结束
.如果真 (Stack_GetTopPos (hStack) ≠ 0)
    Stack_Destroy (hStack)
    JsonValue_Clear (jsonVal)
    errorPos ＝ i
    返回 (#JSON_OBJECTPARENT)
.如果真结束
Stack_Destroy (hStack)
返回 (error)
```

→ **未闭合的容器 → `-5 JSON_OBJECTPARENT`**；`Stack_Destroy` 只释放栈自身，**不会**回滚已经建好的节点（回滚由 `JsonValue_Clear` 完成）。
`jsonValue_parsed_get_error_line`（行列换算）在源码里**存在但被注释掉未调用**（`[Json]:1426,1434`；函数体 `[Json]:1442-1464`）→ **`参数_错误位置` 永远只是一个扁平的宽字符下标**。

## 6.3 容器栈与状态（`jvp_*`）

```
.常量 jvp_needName, "1"
.常量 jvp_needValue, "2"
.常量 jvp_isEnded, "3"
```
（`[Const]:12-14`；注意 `jvp_needName ＝ 1` 与 `JVPS_NAME ＝ 1` 数值相同，代码里两种写法混用，见 `[Json]:1901`）

栈条目 8 字节：`[value(父节点指针)][status]`（`[Stack]:110-111`）。用到的栈操作：`Stack_Get`（看栈顶不弹）、`Stack_Pop`、`Stack_Set`（改栈顶）、`Stack_Push`、`Stack_GetTopPos`。

状态机语义（每个容器一条状态）：

| 状态 | 含义 | 谁能把它设成这个 |
| --- | --- | --- |
| `needName(1)` | 该容器接下来期望「成员名」。根对象/子对象压栈时即为此态 | `parsed_object` 压栈；`parsed_other` 在 `:` 之前 |
| `needValue(2)` | 期望「值」。根数组/子数组压栈时即为此态；对象的 `:` 之后 | `parsed_array` 压栈；`:` 分支（`[Json]:1866-1867`） |
| `isEnded(3)` | 刚刚从「子容器」返回（`}`/`]` 弹栈后给**新栈顶**打上标记） | `}`/`]` 分支（`[Json]:1737-1742`、`[Json]:1885-1890`） |

`isEnded` 的作用：容器结束后紧跟的 `,` 属于**父容器**的分隔符，必须被父容器吞掉而不是当成一个新 token 的开头。处理方式是「见到 `isEnded` 就先改成 needName/needValue，若当前字符正好是 `,` → 直接 `返回 (#JSON_OK)`」（`[Json]:1677-1684`、`[Json]:1773-1780`）。

## 6.4 `jsonValue_parsed_object` / `jsonValue_parsed_array`（`[Json]:1467-1629`）

两者结构完全对称（差别只是容器类型、压栈状态、根节点的类型转换函数）：

| 情形 | 规则 |
| --- | --- |
| **根**（`Stack_Get` 返回 0 ⇒ 栈空） | 若当前节点类型不对 → `JsonValue_SetObject`/`SetArray`（**销毁已有内容**，`[Json]:1484-1488`/`[Json]:1563-1567`）；然后压栈，状态 `needName`（对象）/ `needValue`（数组） |
| **栈顶状态 ≠ `needValue`** | `返回 (#JSON_INVALIDNAME)`（`[Json]:1496-1498`/`[Json]:1579-1581`）。→ 即「在期望属性名的位置放容器」是错的（例如 `{"a"{"b"}}` 少冒号） |
| **父是数组** | `JsonValue_Object/Array (jsonVal)`（继承**根**的 hHeap）→ `JsonValue_Append` → 压栈 |
| **父是对象** | 需要 `namePos ≠ -1 且 nameLen ≠ 0`，否则 `#JSON_INVALIDNAME`；从源串切 `[namePos, nameLen)` 个宽字符成 `name` 字节集（带 `{0,0}`）→ **`JsonValue_TranslationIn (name, transU)` 解码名称里的转义** → `JsonValue_SetW (jvParent, name, jv)` → 压栈 |

* 注意：子节点构造时传的堆来源参数是 **`jsonVal`（根节点）**而不是 `jvParent`（`[Json]:1501,1522,1586,1607`）；由于子节点继承根节点 hHeap、而所有后代同堆，结果一致。
* `parsed_array` 的根分支里有一句多余的 `zyThisCall (jv, #zy_Release)`（压栈失败时释放**未初始化**的局部 `jv`，`[Json]:1570-1573`）→ 潜在崩溃点，正常路径不触发。移植时按「压栈失败就什么都不做」实现即可（或照抄）。
* **`{`/`[` 出现在「值位置」且此前该 token 已有内容** → `pos ＝ valueStart; 返回 (#JSON_INVALIDVALUE)`（`[Json]:1724-1735`、`[Json]:1871-1883`）。也就是说 `1{`、`abc[` 之类的混合 token 被拒。

## 6.5 `jsonValue_parsed_other`（值/名扫描，`[Json]:1632-2053`）—— 核心 token 扫描器

它的参数里 `namePos`、`nameLen`、`pos` 都是 **`参考`**（`[Json]:1637-1640`）→ `pos` 就是主循环的 `i`，所以子函数**直接改写扫描位置**（这是错误位置语义的来源）。

### A) 空栈 ⇒「根标量」分支（`[Json]:1663-1671`）

```
wchar ＝ _指针到短整数U (pStr ＋ pos × 2)
valueStart ＝ pos
.判断循环首 (pos ＜ strLen)
    pos ＝ pos ＋ 1
    valueLen ＝ valueLen ＋ 1
.判断循环尾 ()
value ＝ 取空白字节集 (valueLen × 2 ＋ 2)
RtlMoveMemory_int (_取指针字节集 (value), pStr, valueLen × 2)
```

* 把**从当前位置到 `strLen` 的全部剩余字符**当成一个 token；
* **拷贝起点是 `pStr` 而不是 `pStr ＋ valueStart × 2`** → 若 `pos ＞ 0`（前面有空白）就会拷错内容（源码缺陷，行为见下）；
* 因此根标量的实际语义（**逐条按 `strLen`、`pos` 推导**）：
  * `解析("null")`、`置代码("123")`、`置代码("true")`、`置代码("\"abc\"")` → **成功**，把根节点设成对应标量（`JsonValue_SetNull/SetLong/SetBool/SetString`）。
  * **前置空白 ＝ 静默丢尾字符**（因为切片起点写死成 `pStr`）：`置代码(" 123")`（`strLen＝4`、`pos＝1`、`valueLen＝3`）→ 切片 `" 12"` → `ValueIsNumber` 内部忽略空白 → **LONG，值 12**，`jv_raw ＝ " 12"` → `到文本` 返回 `" 12"`（**丢掉最后一位数字，且不报错**）。`置代码("1 2")` → 切片 `"1 2"` → LONG 值 1、raw `"1 2"` → 输出 `"1 2"`。
  * **`null`/`true`/`false` 是精确串比较**，所以前后任何空白都会让它落空 → 再落到 `ValueIsNumber` 失败 → **`-6 JSON_INVALIDVALUE`，`errorPos ＝ valueStart`**（`置代码(" null")`、`置代码("null ")` 都失败）。
  * `置代码("null")` → 根类型变 NULL；`到文本` → `null`。
* 根分支**不入栈、不产生子节点**（直接改根）。

### B) 父是数组分支（`[Json]:1675-1769`）

扫描循环的分支表（按源码顺序，**先匹配者生效**，`且 quoteChar ＝ 0` 的优先级问题见 §6.6）：

| 分支条件 | 行为 | 行 |
| --- | --- | --- |
| `status ＝ isEnded` | 改状态为 `needValue` 并写回栈；若当前字符是 `,` → **`返回 (#JSON_OK)`** | 1677-1684 |
| `status ≠ needValue` | `返回 (#JSON_INVALIDVALUE)` | 1686-1688 |
| 空白（`32/9/13/10[且 quoteChar=0]`） | `pos++`；`valueLen ＝ 0` 时同时 `valueStart ＝ pos`（即**跳过 token 前的空白**）；**`valueLen` 不增加** | 1694-1699 |
| `"`(34) 或 `'`(39) | `quoteChar＝0`→设成当前引号；否则若相同→`quoteChar＝0`。**不 `到循环尾`** → 落到表尾，引号被计入 `valueLen` | 1700-1709 |
| `\`(92) | 下一个字符是 `u` → `pos += 6`、`valueLen += 6`；否则 `pos += 2`、`valueLen += 2`（即**转义序列按原文整体计入**）；`到循环尾` | 1711-1722 |
| `{` 且 `quoteChar＝0` | `valueLen≠0` → 报错在 `valueStart`；否则 `返回 parsed_object(...)` | 1724-1729 |
| `[` 且 `quoteChar＝0` | 同上 → `返回 parsed_array(...)` | 1730-1735 |
| `]` 且 `quoteChar＝0` | `Stack_Pop`；若新栈顶非空 → 把它的状态改为 `isEnded`；`valueLen＝0` → `返回 (#JSON_OK)`（空数组/尾逗号）；否则 **跳出循环**（处理已积累的 token） | 1736-1748 |
| `,` 且 `quoteChar＝0` | **跳出循环**（处理 token） | 1749-1750 |
| 其它 | 落到表尾 `pos++`、`valueLen++` | 1752-1756 |
| 循环后 `quoteChar ≠ 0` | `pos ＝ valueStart`；`返回 (#JSON_INVALIDVALUE)`（引号未闭合） | 1758-1761 |
| `valueLen ＝ 0` | `返回 (#JSON_OK)`（**静默跳过空元素**，如 `[,,]` 不会生成任何成员） | 1763-1769 |

最终 token ＝ 从 `valueStart` 起连续 `valueLen` 个宽字符：

```
value ＝ 取空白字节集 (valueLen × 2 ＋ 2)
RtlMoveMemory_int (_取指针字节集 (value), pStr ＋ valueStart × 2, valueLen × 2)
```
（`[Json]:1763-1765`）

### C) 父是对象分支（`[Json]:1771-1933`）——「先名后值」两段式扫描器

同一个循环里用 `status` 区分「正在读名」还是「正在读值」，`namePos/nameLen` 与 `valueStart/valueLen` 分别累加。分支表：

| 分支 | 名状态（`needName`） | 值状态（`needValue`） | 行 |
| --- | --- | --- | --- |
| 空白 | `nameLen＝0` 时 `namePos＝pos`；`nameLen` 不增加 | `valueLen＝0` 时 `valueStart＝pos`；`valueLen` 不增加 | 1792-1805 |
| 开引号（`quoteChar＝0`） | `nameLen≠0` → **`返回 (#JSON_INVALIDNAME)`**；否则 `pos++`、`namePos＝pos`（引号本身不计入名字） | `valueLen++`（**开引号计入值 token**） | 1806-1819 |
| 闭引号（`quoteChar＝wchar`） | `quoteChar＝0`；`pos++`；`nameEnd＝1`（引号不计入名字） | `quoteChar＝0`；`pos++`；`valueLen++`（**闭引号也计入值 token**） | 1820-1833 |
| `\` | `u` → `pos+=6, nameLen+=6`；否则 `pos+=2, nameLen+=2` | 同样但累加 `valueLen` | 1836-1856 |
| `:` 且 `quoteChar＝0` | `nameLen＝0` → `#JSON_INVALIDNAME`；否则 `pos++`、`valueStart＝pos`、`valueLen＝0`、`status＝needValue`、写回栈；`到循环尾` | 条件命中但内层 `如果真` 不成立 → **落到表尾，`:` 成为值的一部分** | 1858-1869 |
| `{`/`[` 且 `quoteChar＝0` | 需 `valueLen＝0`，否则报错；委托 `parsed_object/array` 并**直接返回**（属性名此时生效） | 同 | 1871-1883 |
| `}` 且 `quoteChar＝0` | 弹栈 + 给新栈顶打 `isEnded`；`nameLen＝0` → `返回 (#JSON_OK)`（空对象/尾逗号后直接闭合）；否则跳出循环 | 同 | 1884-1896 |
| `,` 且 `quoteChar＝0` | 跳出循环 | 跳出循环 | 1897-1898 |
| 默认 | 若 `status＝needName 且 nameEnd≠0` → **`#JSON_INVALIDNAME`**（引号名之后不能再有其它字符） | 无特殊 | 1899-1903 |
| 表尾 | `pos++`；`status＝needName` → `nameLen++` 否则 `valueLen++` | | 1907-1912 |

循环后的成对校验（`[Json]:1915-1924`）：

```
.判断开始 (nameLen ＝ 0 且 valueLen ≠ 0) ' 有值但是没有名称
    pos ＝ namePos
    返回 (#JSON_INVALIDNAME)
.判断 (nameLen ≠ 0 且 valueLen ＝ 0) ' 有名称但是没有值
    pos ＝ valueStart
    返回 (#JSON_INVALIDVALUE)
.默认
.判断结束
```

* 两个都不为 0 → 拷出 `value`（`[Json]:1927-1928`）。
* 两个都为 0 → 落到「默认」→ 拷出**空** value → 后续 `GetValueType` 返回 UNDEFINED → `-6 JSON_INVALIDVALUE`，`pos ＝ valueStart`，而对象分支里 `valueStart` 在进入函数时被清 0（`[Json]:1788`）→ **`errorPos` 会是 0**（文档化：某些结构错误报出的位置是 0，不可信）。
* 收尾：`.如果真 (preStatus ≠ #jvp_isEnded) status ＝ #jvp_needName; Stack_Set (hStack, jvParent, status)`（`[Json]:1929-1932`）→ 把父对象状态复位成「期望下一个属性名」。`preStatus` 是本函数局部变量（**只有在本函数内刚做过 `]`/`}` 弹栈时才非 0**），所以这一句基本总是执行。

### D) 值落库（名/值都拿到后，`[Json]:1940-2053`）

```
pValue ＝ _取指针字节集 (value)
wchar ＝ _指针到短整数U (pValue)
.如果 (wchar ＝ 34 或 wchar ＝ 39) ' 引号或单引号
    quoteChar ＝ wchar
    nextChar ＝ _指针到短整数U (pValue ＋ (valueLen － 1) × 2) ' 最后一个字符
    .如果真 (quoteChar ≠ nextChar)
        pos ＝ valueStart
        返回 (#JSON_INVALIDVALUE)
    .如果真结束
    childType ＝ #JVTYPE_STRING
.否则
    childType ＝ JsonValue_Parsed_GetValueType (value)
.如果结束
```

* **引号判定只看「首字符是引号 + 末字符是同一引号」**，不校验中间：所以 `"a"b"` 的 token（若扫描器产出了它）会被判为合法 STRING，内容取 `lstrlenW - 2`（即丢掉首尾各一个字符）——真正的保护来自扫描器的 `quoteChar` 状态机。
* `'...'` 单引号完全等价于 `"..."`（作为输入语法被接受）。
* STRING 分支（`[Json]:2003-2016`）：

```
pValue ＝ _取指针字节集 (value)
JsonValue_TranslationIn (pValue, transU)
valueLen ＝ lstrlenW (value) － 2
pValue ＝ pValue ＋ 2
... JsonValue_SetString / JsonValue_String (jsonVal, pValue, valueLen)
```
  即：**先在原地解码转义**，再把长度减 2、指针前移 2 字节（跳过开引号）→ `_CloneStringPtr` 按该长度精确拷贝。
  * 因为减的是 `lstrlenW (value) － 2` 而不是原始 `valueLen`，若解码过程中产生了**内嵌 0**（`\u0000`）或 token 里含引号尾巴，长度会按「第一个 0」算 → 见 §3.3 的截断后果。
* NULL/BOOL/DOUBLE/LONG 分支的「根 / 非根」双路径与 §2.5 的 Double 提升（`[Json]:1956-2001`）。BOOL 的真假由 `lstrcmpW (value, "true") ＝ 0` 决定（`[Json]:1967-1969`）。
* UNDEFINED → `pos ＝ valueStart; 返回 (#JSON_INVALIDVALUE)`（`[Json]:2018-2020`）。
* 挂到父容器（`[Json]:2029-2051`）：数组 `JsonValue_Append (jvParent, jv)`；对象则再次从 `pStr ＋ namePos × 2` 切出名字、`JsonValue_TranslationIn (pName, transU)`、`JsonValue_SetW (jvParent, name, jv)`，然后 **`namePos ＝ -1`、`nameLen ＝ 0`**。
  * 名字被**切两次**（一次给容器分支 `[Json]:1527-1531`，一次给标量分支 `[Json]:2038-2041`），逻辑一致。
* 失败路径统一 `JsonValue_Release (jv)` ＋ `pos ＝ valueStart` ＋ `#JSON_MEMORY`（`[Json]:2023-2046`）。

## 6.6 **输入扫描里的一处优先级歧义（必须实测确定，直接决定含空格字符串能否解析）**

两处空白分支原文完全相同（数组分支 `[Json]:1694`、对象分支 `[Json]:1792`）：

```
.判断开始 (wchar ＝ 32 或 wchar ＝ 9 或 wchar ＝ 13 或 wchar ＝ 10 且 quoteChar ＝ 0)
```

| 读法 | 条件等价式 | 对 `"hello world"` / `"outdoor "` 的后果 |
| --- | --- | --- |
| **R1（`且` 优先于 `或`，多数语言惯例）** | `(wchar=32) ∨ (wchar=9) ∨ (wchar=13) ∨ ((wchar=10) ∧ quoteChar=0)` | 引号内的空格/制表**不计入 `valueLen`/`nameLen`**，而 token 是按长度**连续切片** → 值切片丢掉闭引号 → 首尾引号校验失败 → **`-6 JSON_INVALIDVALUE`（`errorPos＝valueStart`）**；键切片同样少算 → **键名被静默截短**（如 `a b`→`a `）。即「任何含内部空格的值都会报错、键都会变形」 |
| **R2（`quoteChar ＝ 0` 修饰整个或式，即字面意图）** | `((wchar=32) ∨ (wchar=9) ∨ (wchar=13) ∨ (wchar=10)) ∧ quoteChar=0` | 引号内空白正常计入 token → 含空格字符串解析成功。 |

**倾向 R2 的证据**：本模块自带资源里有「必须能被解析」的含内部空格字符串——`源码txt\资源\表头对应属性名JSON.txt` 含 `"product title"`（且整体是带换行缩进的 pretty-printed JSON），`源码txt\资源\测试JSON.txt` 含 `"v":"outdoor "`（值的尾部有空格）；`[Boot]:96-102` 的调试流程也是「读一个大 JSON → `解析 (str, 真, error, errorPos)` → 失败就打印 error/errorPos → 成功就 `到文本 (“”)`」。若 R1 成立，这类数据一律报 -6，与库的实际使用不符。

**决定性实测（务必先做，再决定 C++ 写法）**（下面的 `<…>` 表示把该 JSON **原文**作为 `参数_JSON文本` 传给 `置代码`，注意 EPL 文本定界符是全角 `“ ”`，内部的 ASCII `"` 原样写即可）：
1. `置代码 (<{"a":"outdoor "}>)` → R2：返回真、`到文本 ()` ＝ `{"a":"outdoor "}`；R1：**返回假、错误码 -6**（切片丢掉闭引号）。
2. `置代码 (<{"a b":1}>)`（**键**内含空格）→ R2：`到文本` ＝ `{"a b":1}`；R1：键切片只有 2 个字符 → **`{"a ":1}`（`b` 被静默吞掉且不报错）** —— 这条最能区分两种读法，因为名字切片同样是「从 `namePos` 起连续 `nameLen` 个字符」（`[Json]:2040`）。
3. `置代码 (<{"a":1 2}>)`（值内空格、无引号）→ 两种读法都会因 `"` 被当开引号、`}` 被并入值而在收尾产生类型不认识 → -6；本条只用于确认「无引号值里的空格被跳过」这一共同行为。

**C++ 落地建议**：实现成可开关的一个布尔（`whitespaceSuppressedInsideQuote`），默认按 R2；用上面的用例对拍真实 `.fne` 后再固定。

## 6.7 输入侧转义解码：`JsonValue_TranslationIn`（`[Json]:3014-3106`）

**原地（in-place）在 UTF-16 缓冲区上改写**，输入必须恰好是 `nameLen×2+2` 或 `valueLen×2+2` 大小（调用点：`[Json]:1531,1616,2005,2041`）。它按「反斜杠」逐个查找（`StrStrW_bin2 (value, { 92, 0, 0, 0 })` ＝ shell32 `StrStrW` 找子串 `\`，返回**指针**而非下标，`[DLL]:119-121`）：

```
.判断开始 (nextChar ＝ 92 或 nextChar ＝ 47 或 nextChar ＝ 34)   ' \\ \/ \"  → 直接丢掉反斜杠
    overLen ＝ Len － (pos － value)
    .如果真 (overLen ≥ 2)
        RtlMoveMemory_int (pos, pos ＋ 2, overLen － 2)
    .如果真结束
    ZeroMemory (pos ＋ overLen － 2, 2)
    Len ＝ Len － 2
.判断 (nextChar ＝ 98)  newChar ＝ 8      ' \b → U+0008
.判断 (nextChar ＝ 116)  newChar ＝ 9     ' \t → U+0009
.判断 (nextChar ＝ 110)                   ' \n → 若前一个字符不是 CR，则插入 CR，再写 LF（＝变成 CRLF）
    .如果真 (pos ＞ value 且 _指针到短整数U (pos － 2) ≠ 13)
        preChar ＝ 13
    .如果真结束
    newChar ＝ 10
.判断 (nextChar ＝ 114)  newChar ＝ 13    ' \r → U+000D
.判断 (nextChar ＝ 102)  newChar ＝ 12    ' \f → U+000C
.判断 (nextChar ＝ 117 且 transU)          ' \uXXXX（仅当 参数_Unicode解码＝真）
    RtlMoveMemory_int (pBuff ＋ 4, pos ＋ 8, 4)
    StrToIntEx_ptr (pBuff, #STIF_SUPPORT_HEX, char1)
    RtlMoveMemory_int (pBuff ＋ 4, pos ＋ 4, 4)
    StrToIntEx_ptr (pBuff, #STIF_SUPPORT_HEX, char2)
    RtlMoveMemory_int (pos, _取变量地址 (char1), 1)
    RtlMoveMemory_int (pos ＋ 1, _取变量地址 (char2), 1)
    RtlMoveMemory_int (pos ＋ 2, pos ＋ 12, value ＋ Len － pos － 12)
    ZeroMemory (value ＋ Len － 12 ＋ 2, 12 － 2)
    Len ＝ Len － 10
.默认
.判断结束
```
（`[Json]:3048-3085`）

### 精确规则

| 输入序列 | 存储结果 | 说明 |
| --- | --- | --- |
| `\\` `\/` `\"` | `\` `/` `"` | 删掉反斜杠，尾部左移 2 字节 |
| `\b` `\t` `\r` `\f` | U+0008 / 0009 / 000D / 000C | 单个宽字符替换，尾部左移 |
| `\n` | **U+000D U+000A（CRLF）**，除非紧邻的前一个字符已经是 CR（那就是纯 LF） | `preChar ≠ 0` 分支写两个字符、不做左移、`Len` 不变（`[Json]:3089-3091`） |
| `\uXXXX`（`transU＝真`） | 1 个宽字符 | **手动大小端拼装**：`char1 ＝ 十六进制(X3X4)`（低字节）写入 `pos`，`char2 ＝ 十六进制(X1X2)`（高字节）写入 `pos+1`；两次都用 `StrToIntExW (0x.., STIF_SUPPORT_HEX)`，只写入 1 个**字节**（`_取变量地址 (char1), 1` → 取整数低字节）→ 结果就是该 code unit |
| `\uXXXX`（`transU＝假`） | **原样 6 个字符 `\uXXXX`**（反斜杠保留） | `且 transU` 使该分支不匹配 → 落到默认 → 什么都不做 |
| 其它（`\x`、`\0`、`\a`…） | **原样 2 个字符**（反斜杠保留） | 默认分支 |
| 反斜杠后紧跟 `0`（串尾） | 走 `\u`/默认分支，`nextChar＝0` → 默认 → 保留 | 不会越界 |

* **代理对（surrogate pair）**：`\uD83D\uDE00` 被当作**两个独立 code unit** 各自解码（各自 6→1），拼在内存里正好是一个合法的 UTF-16 代理对 → **不需要特殊处理，天然支持非 BMP**。但：若 `transU＝假`，则存下的是字面文本 `\uD83D\uDE00`（24 个字符）。
* 非法十六进制（如 `\uZZZZ`）：`StrToIntExW` 失败、`char1/char2` 保持**上次的值**（局部变量未初始化 → 首次为 0）→ 写入 0,0 即内嵌终止符 → **字符串在该处被截断**。移植时建议：解析失败写 0 并截断（＝与 R1 一致）。
* 循环推进：`pos ＝ StrStrW_bin2 (pos ＋ 2, {92,0,0,0})`（`[Json]:3104`）→ **从「反斜杠后第二个宽字符」继续搜索**，即每个反斜杠只处理一次；`\n` 分支写入了两个字符（CR LF）后仍从 `pos+2` 继续 → 不会重复处理。
* `Len` 全程以**字节**为单位（`Len ＝ lstrlenW_Ptr (value) × 2`，`[Json]:3043`），减少量分别是 2（删一个宽字符）和 10（`\uXXXX` 6 个字符 → 1 个字符）。

## 6.8 输出侧转义：`JsonValue_TranslationOut`（`[Json]:3145-3220`）

在**副本**上工作：`buffLen ＝ strLen × 6 ＋ 2`，先整块拷入原串，再从左到右扫描（`i` 是宽字符下标）：

```
.判断开始 (wchar ＝ 92 或 wchar ＝ 47 或 wchar ＝ 34) ' \ / "
    RtlMoveMemory_int (pos ＋ 2, pos, buffLen － i × 2 － 2)
    RtlMoveMemory_int (pos, _取变量地址 (transChar), 2)   ' 插入一个反斜杠
    i ＝ i ＋ 1
.判断 (wchar ＝ 8)  → \b      .判断 (wchar ＝ 9)  → \t     .判断 (wchar ＝ 10) → \n
.判断 (wchar ＝ 13) → \r      .判断 (wchar ＝ 12) → \f
```
（`[Json]:3177-3211`）

* **被转义的字符集是**：`\`、`/`、`"`、U+0008、U+0009、U+000A、U+000D、U+000C。
* **不转义**：其它控制字符（U+0000–U+0007、U+000B、U+000E–U+001F）、所有非 ASCII（中文、emoji）→ **原样写出宽字符**。
* **`/` 会被加上反斜杠**：存储里的 `a/b` 输出为 `"a\/b"`。这是输入→输出的一条**必然差异**（合法 JSON，但字符串变了）。
* `\n` 的反向过程：因为 `TranslationIn` 把 `\n` 变成了 CRLF，`TranslationOut` 会写成 `\r\n` 两个转义序列（`[Json]:3194-3205` 分别处理 13 和 10）。
* 不做 `\uXXXX` 编码（`zyToUnicodeString` 存在于 `[Help]:691-706` 但解析/序列化路径不调用它）。
* `value ＝ 0` → `outValue ＝ { 0, 0 }` 且返回假（`[Json]:3159-3162`）；`ToStringProc` 不检查返回值 → 输出 `""`（两个引号）。

## 6.9 错误码 ↔ 失败原因 完整映射

常量（`[Const]:89-95`）：

```
JSON_OK ＝ 0 / JSON_ERROR ＝ -1（未知，源码从不返回） / JSON_INVALIDARG ＝ -2
JSON_MEMORY ＝ -3 / JSON_INVALIDNAME ＝ -4 / JSON_OBJECTPARENT ＝ -5
JSON_INVALIDVALUE ＝ -6
```

| 码 | 触发点（全部） | `errorPos` 值 |
| --- | --- | --- |
| `-2` | `jsonVal=0` 或 `pStr=0` 或 `strLen=0`（`[Json]:1378`） | 0（入口处已 `errorPos ＝ 0`） |
| `-3` | `Stack_Create` 失败（`[Json]:1385`）；`parsed_object/array` 里 `SetObject/SetArray` 失败、子节点构造失败、`Append/SetW` 失败、`Stack_Push` 失败（`[Json]:1486,1503,1508,1513,1524,1534,1538,…`）；值节点构造失败或 `Append/SetW` 失败（`[Json]:1962,1975,1986,1999,2013,2025,2033,2045`） | 多数**未调整**（＝主循环当前 `i`）；`[Json]:2024,2032,2044` 会先 `pos ＝ valueStart` |
| `-4` | 容器出现在期望属性名的位置（`[Json]:1497,1580`）；父是对象但 `namePos＝-1 或 nameLen＝0`（`[Json]:1520,1605`）；对象状态不是 `needName`（`[Json]:1783`）；引号名之后又出现其它字符（`[Json]:1902`）；`:` 之前没有名字（`[Json]:1861`）；开引号时名字已非空（`[Json]:1812`）；**「有值但没有名」**（`[Json]:1917`） | `if [Json]:1916` 分支 → `pos ＝ namePos`；其余 → 主循环进入本 token 时的位置 |
| `-6` | 数组状态不是 `needValue`（`[Json]:1687`）；`{`/`[` 前面已有 token 内容（`[Json]:1727,1733,1874,1881`）；引号未闭合（`[Json]:1760`、`[Json]:1948`）；**「有名但没有值」**（`[Json]:1921`）；值类型 UNDEFINED（`[Json]:2020`）；根标量分支的类型不认识（`[Json]:1962,…`） | 显式 `pos ＝ valueStart`（多数）；`[Json]:1916` 用 `namePos` |
| `-5` | 扫描完成但栈非空（容器未闭合）（`[Json]:1430-1435`） | `i`（循环结束时的位置，**可能是 `strLen ＋ 1`**） |

### `errorPos` 的精确语义（照抄要点）

1. **单位**：**UTF-16 宽字符下标**，0-based（**不是字节下标**）。对 `置代码`/`解析`（ANSI 入参）来说，`errorPos` 也是**转换后宽串的字符下标**，与 GBK 字节偏移不一致（中文混排时不等价）。
2. `jsonValue_parsed_other` 的 `pos` 是 `参考` → 各失败分支把 `pos` 改成 `valueStart` / `namePos`，主循环退出时 `errorPos ＝ i` 取的就是这个被改过的值（`[Json]:1425`）。
3. 成功 token 处理完后 `pos` 停在**定界符（`,`/`]`/`}`）上**，主循环再 `i++` 越过它。
4. 未闭合容器 → `errorPos ＝ strLen ＋ 1`；结构错乱（`{,}`、`{}` 后多余内容）→ 常见 `errorPos ＝ 0`。
5. `jsonValue_parsed_get_error_line`（行列）未被调用 → 没有 line/col 语义。

### 静默通过（返回 `#JSON_OK`／上层 `真`，但**有信息丢失或语义偏离**）

| 输入 | 结果 |
| --- | --- |
| `{"a":1,}`、`[1,2,]` | 尾逗号被忽略，**不会**多出成员/元素（`[Json]:1744-1745,1892-1893`） |
| `[1,,2]`、`{,}` 之外的多余逗号 | `valueLen＝0` → 空元素静默跳过（`[Json]:1767-1768`） |
| `{a:1}`、`{'a':1}`、`{'a':'b'}` | 无引号键、单引号键/值全部接受 |
| `{"a":1}{"b":2}`、`[1][2]` | 第二个根容器**复用同一个根节点**继续填充 → 合并成一个对象/数组（`[Json]:1481-1492`：栈空 → 根分支，类型已对 → 不再转换，只压栈） |
| `[1 2]`（数组里漏逗号） | ✅ **成功但内容被改写**：扫描时空格「跳过但不计数」→ `valueLen＝2` → 切片是**连续的** `valueStart` 起 2 个字符 ＝ `"1 "`（`2` 掉在切片之外）→ `ValueIsNumber ("1 ")` 通过（空格被内部忽略）→ LONG→**DOUBLE**（§2.5），值 `_wtoi64("1 ") ＝ 1`，`jv_raw ＝ "1 "` → **`到文本` 输出 `[1 ]`（尾随空格保留、`2` 消失）**。这是「切片长度与扫描位置解耦」造成的一类典型失真 | `[Json]:1690-1769` |
| `\xNN`、`\/`（无必要）、`\u` 未解码 | 原样保留（见 §6.7） |
| `{"a":1,"a":2}` | 键唯一化：值＝2，位置＝第 1 个（§4.4） |
| 解析失败 | 节点被 `JsonValue_Clear` 清空后返回错误；**`置代码`/`解析` 返回假**，但节点已经是「空的同类型节点」而不是原内容 |

## 6.10 语法接受 / 拒绝速查表

| 语法 | 接受? | 依据 |
| --- | --- | --- |
| 空白（空格/Tab/CR/LF）作分隔 | ✅ | `[Json]:1395` |
| `\v`(0x0B)、`\f`(0x0C)、NBSP、**BOM(0xFEFF)** 作分隔 | ❌（落入其它 token 分支，通常 -6） | `[Json]:1395` 白名单只有 5 个值 |
| 前置 BOM 的 UTF-16 文本 | ❌（`0xFEFF` 非空白）→ **调用方必须剥 BOM**；`置代码W`/`zyA2W` 也不产生 BOM | 同上 + `api-spec.md` F-7 |
| `//` 与 `/* */` 注释 | ❌ 完全不支持（`/` 只是普通字符，会污染 token） | `[Json]:1632-2053` 无注释分支 |
| 尾逗号 | ✅ 静默忽略 | `[Json]:1744,1892` |
| 连续逗号 / 空元素 | ✅ 静默跳过 | `[Json]:1767` |
| 单引号字符串 | ✅ 完全等价 | `[Json]:1700,1806,1943` |
| 无引号对象键 | ✅ | `[Json]:1790-1914`（名字状态不要求引号） |
| 无引号字符串值 | ❌（`abc` → UNDEFINED → -6） | `[Json]:2018-2020` |
| 键/值内部空白 | 见 §6.6（R1/R2 歧义） | `[Json]:1694,1792` |
| `NaN`/`Infinity`/`-Infinity` | ❌ -6 | `[Json]:2085-2157` 字符白名单 |
| 前导 `+`（`+5`） | ✅ LONG（值 5） | `[Json]:2109` 放行 43 |
| 前导 `-`（`-5`） | ✅ LONG（值 -5） | 同上 |
| 中段 `-`/`+`（`1-2`、`1e-5`） | ✅ DOUBLE（`f＝1`） | `[Json]:2138-2144,2154` |
| 前导零（`007`） | ✅ LONG，值 7，raw `"007"` | 无检查 |
| 十六进制 `0x10` | ❌ -6（`x` 不在白名单） | `[Json]:2148` |
| 指数（`1e5`、`1E+5`） | ✅ DOUBLE | `[Json]:2131-2137` |
| 两个点 `1.2.3` | ❌ -6（`done ＞ 1`） | `[Json]:2126` |
| 两个 e `1e2e3` | ❌ -6 | `[Json]:2133` |
| 单独 `.` | ✅ DOUBLE（值 0） | `[Json]:2110-2116` |
| `1e`（指数缺数字） | ✅ DOUBLE（`到数值` 决定值） | `[Json]:2131` |
| 深嵌套 | **无显式层数上限**。容器栈按 8 字节/层，初始 1024 项，溢出时 `newLength ＝ length × 2`；注意 `[Stack]:106` 把 `tableLength` 写成了 `newLength × 8`（**字节当项数**）→ 第二次扩容要到 16384 项才发生 | `[Stack]:98-108` |
| 递归 | `parsed_object/array/other` 相互递归（EPL 调用栈），每层约 1 帧 + 若干局部变量 → 实际深度上限由进程栈决定（万级） | `[Json]:1729,1735,1876,1883` |
| `\uXXXX` | 取决于 `参数_Unicode解码`（默认见下） | `[Json]:3073` |
| 顶层标量（`123`/`“s”`/`true`/`null`） | ✅；但**带前后空白时**：数字类**静默丢尾字符**（切片起点写死 `pStr`），`null/true/false` 直接 `-6`（§6.5-A） | `[Json]:1663-1671` |
| 多个顶层值（`1 2`、`{...}...`） | `1 2` ✅（整段剩余被当**一个** token，值取到第一个数字段、raw 保留整段）；`{"a":1}{"b":2}` ✅（第二个根容器复用同一节点，见上表）；`{"a":1} 2` ✅（`2` 作为新的根标量 token 覆盖根节点类型 → **前面的对象被 `JsonValue_SetLong` 销毁**） | `[Json]:1663-1671,1481-1492,1994` |

### `参数_Unicode解码` 的实际默认值（三条入口不一致）

| 入口 | 代码 | 默认真实值 |
| --- | --- | --- |
| `置代码` | `参数_Unicode解码 ＝ 选择 (是否为空 (参数_Unicode解码), 真, 参数_Unicode解码)`（`[Cls]:66`） | **真** |
| `置代码W` | 同上（`[Cls]:133`） | **真** |
| `置代码UTF8` | `.如果真 (是否为空 (参数_Unicode解码)) 参数_Unicode解码 ＝ 假`（`[Cls]:99-101`） | **假** |
| `解析` / `解析W` | 直接把可空参数透传（`[Cls]:287,304`）；EPL 可空逻辑型未传 → 假 | **假** |
| `合并` / `合并W` | `选择 (是否为空 (参数_USC2解码), 真, ...)`（`[Cls]:1400,1429`） | **真** |

（参数尾注释一律写「默认为假」→ 与实现矛盾，见 `api-spec.md` H-1。**必须逐入口保持默认值差异**，因为它决定 `\uXXXX` 是解码成 1 个字符还是原样保留 6 个字符，进而决定输出文本。）

---

# 7. 路径解析（`JsonValue_SelectW/SelectA`、`SelectNoCreateW`、`QueryW`）

## 7.1 语法（由 `JsonValue_PathParsed` 定义，`[Json]:2327-2431`）

支持形式：`a.b.c[0]`、`[0].a.b.c`、`a[0][1]`、`[0]`、空（＝当前节点）。

```
.判断开始 (wchar ＝ 92) ' \ 转义 . 或 [ ]
    nextChar ＝ _指针到短整数U (pStr ＋ (pos ＋ 1) × 2)
    .如果真 (nextChar ＝ 46 或 nextChar ＝ 91 或 nextChar ＝ 93 或 nextChar ＝ 92)
        RtlMoveMemory_int (pStr ＋ pos × 2, pStr ＋ (pos ＋ 1) × 2, (strLen － pos － 1) × 2)
        ZeroMemory (pStr ＋ (strLen － 1) × 2, 2)
        strLen ＝ strLen － 1
    .如果真结束
.判断 (wchar ＝ 46 或 pos ＝ strLen) ' 点 或 结尾
    hasName ＝ namePos ＜ pos
    ... 生成 JVPS_NAME 步骤（name ＝ _CloneStringPtr (pStr ＋ namePos × 2, pos － namePos)）
    namePos ＝ pos ＋ 1
.判断 (wchar ＝ 91) ' [
    indexPos ＝ pos ＋ 1
    hasIndex ＝ 真
    hasName ＝ namePos ＜ pos          ' 中括号前若已有文字，先作为名字步骤产出
    ...
    namePos ＝ pos ＋ 1
.判断 (wchar ＝ 93) ' ]
    .如果真 (hasIndex)
        nameLen ＝ pos － indexPos
        indexStr ＝ _CloneStringPtr (pStr ＋ indexPos × 2, nameLen, hHeap)
        stepIndex ＝ StrToInt (indexStr)
        _Free (indexStr, hHeap)
        _Set (pStep, #jvstep_flags, #JVPS_INDEX)
        _Set (pStep, #jvstep_index, stepIndex)
        Array_Add (pArray, pStep)
    .如果真结束
    namePos ＝ pos ＋ 1
```
（`[Json]:2362-2424`）

### 规则清单

1. **步骤结构** `JsonValuePathStep`（`[Type]:3-6`、`[Const]:109-112`）：`flags`（0 偏移，`JVPS_NAME＝1` 或 `JVPS_INDEX＝2`）、`name`（4 偏移，UTF-16 指针或 0）、`index`（8 偏移）。`jvstep_cbSize ＝ 12`。
2. **一个步骤只有一种类型**：`a[0]` → **两个**步骤（NAME `a`、INDEX 0）；不存在「name+index」复合步骤（`SelectW` 用 `位与 (stepFlags, JVPS_NAME) ＝ JVPS_NAME` 判断，实际等价于相等比较）。
3. **转义**：只有 `\.`、`\[`、`\]`、`\\` 被识别，且是**破坏性原地删除反斜杠**（左移尾部、`strLen--`）→ **会修改调用方传进来的 字节集 缓冲区**。`\<其它字符>`（如 `\u0041`、`\ `）**保留两个字符**，反斜杠成为名字的一部分。
4. **没有引号语法**：路径里不支持 `"a.b"`、`'a.b'` 这类带引号的键，引号只是普通字符。
5. **分隔符**：`.`；结尾处（`pos ＝ strLen`）也触发一次名字冲刷 → `"a.b"` 产出 2 步；`"a."` 的第 2 步 `namePos＝2`、`pos＝2` → `hasName` 为假 → **产出 1 步**；`".a"` → 首个 `.` 时空名被丢弃 → 产出 1 步（**前导点被忽略**）。
6. **空路径**：`path ＝ { }`（空字节集）或 `lstrlenW (path) ＝ 0` → 返回 0 → `SelectW/SelectNoCreateW` **直接返回当前节点**（`[Json]:2447-2454`、`[Json]:2500-2507`）。`JsonValue_SelectA` 传 `“”` → `zyA2W(“”) ＝ {0,0}` → 同样返回当前节点。
7. **`[]`（空中括号）**：`hasIndex` 为真、`nameLen＝0` → `StrToInt (空串)` 行为见 §7.2 注 → 步骤 `index` 为 0/1。
8. **数组下标是 0-based**（`[Cls]:657` 尾注释「索引从0开始」）；负数不特殊处理。
9. 步骤数组本身用 `Array_Create (GetProcessHeap ())`，节点用 `_Alloc (#jvstep_cbSize, hHeap)`；结束由 `JsonValue_FreePathParsed` 释放 name + step + 数组（`[Json]:2297-2325`）。若某步 `_Alloc` 失败会**提前 `返回 (pArray)`**（带着已建好的部分步骤），调用方照常使用已建部分。

## 7.2 索引取值的疑点（`StrToInt`）

`[Json]:2415`：`stepIndex ＝ StrToInt (indexStr)`，而 `[DLL]:213-215` 声明为

```
.DLL命令 StrToInt, 整数型, "shlwapi.dll", "StrToIntW"
    .参数 lpSrc, 整数型
```

`shlwapi!StrToIntW(PCWSTR, int* piRet)` 的**返回值是 BOOL，数值通过第二个出参**返回，而这里只声明了一个参数、且把返回值当索引。候选解读：

| 解读 | `选择("a[2]")` 实际用到的索引 |
| --- | --- |
| **P-a（按 ABI 字面）** | 恒为 **1**（解析成功）或 **0**（失败），并且会因为少传一个参数造成 `retn 8` 的栈失衡 |
| **P-b（按文档/示例意图）** | **2**（即 `strtol` 十进制） |

`api-spec.md` 未讨论此处；`[Boot]:81-89` 的示例（`置文本 (“中学.二班[0]”, “张三”)`）与 `[Cls]:326` 的参数说明（`a.b.c[0]`）都假定 P-b。**处理办法**：用真实 `.fne` 跑一次

```
json.创建 () ：置文本 (“a[2]”, “X”) → 到文本 ()   ' 期望 {"a":[null,null,"X"]}（P-b）；若得 {"a":["X"]} 或崩溃则是 P-a
```

再定 C++ 实现。本规格其余部分按 **P-b（十进制整数，允许 `-`，溢出/非法 → 0）** 描述；若实测为 P-a，请把所有 `[n]` 改成「`n` 解析成功→1，失败→0」。

## 7.3 `JsonValue_SelectW`（自动创建版，`[Json]:2434-2484`）

```
.判断开始 (位与 (stepFlags, #JVPS_NAME) ＝ #JVPS_NAME 且 stepName ≠ 0)
    jv ＝ JsonValue_GetW (jv, 指针到字节集 (stepName, lstrlenW_Ptr (stepName) × 2 ＋ 2))
.判断 (位与 (stepFlags, #JVPS_INDEX) ＝ #JVPS_INDEX)
    .如果真 (i ＝ 1 且 位与 (stepFlags, #JVPS_NAME) ＝ 0) ' 第一个 且 没有名称
        .如果真 (JsonValue_SetArray (jv) ＝ 假)
            jv ＝ 0
            跳出循环 ()
        .如果真结束
    .如果真结束
    jv ＝ JsonValue_GetAt (jv, stepIndex)
.默认
.判断结束
```

* **NAME 步** → `JsonValue_GetW`（`[Json]:1076-1104`）：
  * 当前节点类型 ≠ OBJECT → **`JsonValue_SetObject`：销毁原值（含文本/数字/数组全部内容），变成空对象**；
  * 键存在 → 返回该子节点（不 AddRef）；
  * 键不存在 → **新建 `JsonValue_Null` 并 `HashEx_Put`（追加到有序数组末尾）＋ 设 parent** → 即「读操作会改文档」。
* **INDEX 步** → `JsonValue_GetAt`（`[Json]:913-947`）：
  * 当前节点类型 ≠ ARRAY → `JsonValue_SetArray`（销毁原值变空数组）；
  * `index ≥ count` → **用 `JsonValue_Null` 补齐到 `index`**（每个都设 parent）后返回第 `index` 个；
  * `index ＜ 0` → 返回 0（不补齐）。
  * **首步特例**：路径第一个步就是 `[i]`（无名字）时，先**显式** `SetArray` 再 `GetAt`（`[Json]:2469-2476`）→ 效果相同，但保证「对对象根使用 `[0]`」会**清空该对象**。
* **自动创建的类型选择**＝**由「下一步的类型」决定**：缺失的中间节点先建成 NULL，下一步若是 NAME 则升级为 OBJECT，若是 INDEX 则升级为 ARRAY。因此
  `置文本 ("a.b[0].c", "x")` → `{"a":{"b":[{"c":"x"}]}}`。
* **类型不匹配的既有节点**：`SelectW` 一律**替换成需要的容器类型**（销毁原值，保留 `jv_key`/`jv_parent`）→ 不报错、不返回 0。
* 任一步返回 0（例如负索引、`_Alloc` 失败）→ `jv ＝ 0`，但循环**不中止**：`JsonValue_GetW (0, …)` 会先判 `jsonValue ＝ 0` 返回 0 → 最终返回 0（除首步 INDEX 会显式跳出）。

## 7.4 `JsonValue_SelectNoCreateW`（不创建版，`[Json]:2487-2537`）

与 7.3 的差别**只有三处**：

| | NAME 步 | INDEX 步 | 找不到 |
| --- | --- | --- | --- |
| SelectNoCreateW | `JsonValue_GetReadOnlyW`：**要求节点当前就是 OBJECT**，否则返回 0；**不新建**成员 | `JsonValue_GetAtReadOnly`：ARRAY→`Array_Get`；**OBJECT→按 eArray 位置取成员**；其它→0 | **`jv ＝ 0 → 跳出循环`**，返回 0 |
| SelectW | `JsonValue_GetW`（自动 SetObject + 建 null 成员） | `JsonValue_GetAt`（自动 SetArray + null 补齐） | 继续循环，最终 0 |

* 两版都**不回滚**：`SelectW` 中途失败时前面已经创建的 null 成员/补齐元素**留在文档里**。
* 使用 `SelectNoCreateW` 的公开方法（`[Cls]`）：`到文本*`、`取类型*`、`取父*`、`取键数量*`、`取属性名*`、`取源数据*`、`取文本*`、`取逻辑*`、`取双精度*`、`取数值*`、`取长整数*`、`取整数*`、`取所有键*`、`取所有值*`、`排序*`、`排序对象*`、`K2*`、`I2*`。
* 使用 `SelectW`（创建）的：`清空*`、`置属性*`、`添加成员*`、`插入成员*`、`置成员*`、`移除属性*`、`移除成员*`、`选择*`、`置对象*`、`置数组*`、`置文本*`、`置空*`、`置逻辑*`、`置双精度*`、`置数值*`、`置长整数*`、`置整数*`、`插入属性*`、`复制*`、`合并*`、`合并对象*`、`合并数组*`、`K*`、`I*`。

## 7.5 `JsonValue_QueryW`（`是否存在`，`[Json]:2546-2602`）

* 用 `GetReadOnlyW` / `GetAtReadOnly`（**完全不创建**），任一步落空 → `ret ＝ 假` 并跳出；
* **空路径 → 返回假**（`pArrayStep ＝ 0 → 返回 (假)`；`count ＝ 0 → 返回 (假)`，`[Json]:2563-2570`）→ `是否存在(“”)` 即使当前节点存在也返回假；
* `pStep ＝ 0` 时置 `ret ＝ 假` 但**继续循环**（`到循环尾`）而不是跳出（`[Json]:2574-2577`）→ 若最后一步是 0，返回假但 `jv` 保留上一个值；正常流程不会产生 0 步骤。

## 7.6 `K` 与 `K2`、`I` 与 `I2` 的完整差异（`[Cls]`）

| | `K`/`KW`/`K_UTF8`（`[Cls]:503-546`） | `K2`/`K2W`（`[Cls]:548-573`） | `I`/`IW`（`[Cls]:707-744`） | `I2`/`I2W`（`[Cls]:747-772`） |
| --- | --- | --- | --- | --- |
| 是否 `初始化 ()` | ✅ | ✅ | ✅ | ✅ |
| 路径解析器 | `JsonValue_SelectA/W`（**创建**） | `JsonValue_SelectNoCreateW`（**不创建**） | `JsonValue_SelectA/W`（**创建**） | `JsonValue_SelectNoCreateW` |
| 末端取值 | `JsonValue_GetA/W`（缺失 → **建 null 成员**、非对象 → **变对象**） | `JsonValue_GetReadOnlyA/W`（缺失/类型不符 → 0） | `JsonValue_GetAt`（越界 → **null 补齐**、非数组 → **变数组**） | `JsonValue_GetAtReadOnly`（数组按索引 / 对象按位置 / 其它 0） |
| 返回值 | `Json2`，`置指针`（AddRef） | 同 | 同 | 同 |
| `设置是否为顶层 (假)` | ✅ **调了** | ❌ **没调**（保持 `_初始化` 的「真」） | ✅ **调了** | ❌ **没调** |
| 副作用 | 可能往文档里加 `"key":null` / `[null,…]` | 无 | 可能补齐 null | 无 |

* 返回的子对象**与原树共享节点**（只是多一个引用）：改它即改原文档；父节点析构时它会 `Detch`+`Destory` → 该 Json2 变量随后再被使用即为野指针（源码未防护）。
* `是否已初始化` 只在 `初始化`/`置代码*`/`解析*`/`置指针` 里置真，`销毁` 不置回（`api-spec.md` H-12）。

---

# 8. 引用计数与「顶层（top-level）」语义

## 8.1 谁持有那一份引用

* 构造出来的节点 `refCount ＝ 1`。
* **容器持有子节点的那一份引用（不额外 AddRef）**：`JsonValue_Set*/Append/GetAt/GetW` 只是把指针写进桶链/eArray 并设 `jv_parent`（`[Json]:1202,885,1035,1072`）；因此容器析构时逐个 `Release`（`[Json]:55,66,110,120`）刚好把它归零。
* **易语言 `Json2` 变量的那一份引用靠 `置指针 → JsonValue_Addref` 获得**（`[Cls]:161-170`），变量作用域结束/`清除`/`销毁` 时由 EPL 运行时 Release。
* `JsonValue_Addref/Release`（`[Json]:170-184`）对 `this ＝ 0` 都返回 0，不做任何事 → 对空指针安全。
* 所以一个被 `K()` 取出的成员引用数典型为 **2**（父容器 1 + Json2 变量 1）；父先释放时子不会真死（还剩 1），反之亦然。
* `复制`/`置属性`/`添加成员`/`插入成员`/`置成员`/`合并对象` 都先 `JsonValue_Clone`（新节点 ref＝1），然后交给容器持有（不再 AddRef）；失败时显式 `JsonValue_Release`（`[Cls]:462-472,1323-1326`）。

## 8.2 释放链（一次 `Release` 到底做了什么）

```
Release(refCount 4) → 0 ?
  → JsonValue_Detch (this)                 ' 从 jv_parent 的容器里摘掉自己 + 清 jv_parent
  → zyThisCall (this, #zy_Destory)         ' JsonValue_Destory：递归 Release 所有子节点 → 销毁容器 → 释放字符串/raw
  → zyDeleteObject (this)                  ' 释放 pFunctions 与节点本体
```

**`清除`（`JsonValue_Clear`，无引用计数参与）**：只递归 `Release` 子节点并清空容器 → 节点自身类型/句柄保留（§1.4）。所以：

* `清空` 之后 `取键数量` ＝ 0、`到文本` ＝ `{}`（对象）/`[]`（数组）；
* 子节点的 Json2 变量在父 `清空` 后**仍可安全使用**（引用还在），但已脱离父（`jv_parent`＝0）→ `取父` 返回 0、再赋值为新树成员需显式 `置属性`。

## 8.3 `设置是否为顶层 (假)` 改了什么

```
.子程序 设置是否为顶层, , 公开
    .参数 是否为顶层, 逻辑型
    集_是否为顶层 ＝ 是否为顶层
```
（`[Cls]:152-155`）

* **只改 `Json2` 类实例自己的逻辑型成员 `集_是否为顶层`**（`[Cls]:5`），**完全不碰 JsonValue 节点**（没有写 `jv_parent`、没有改引用计数）。
* `_初始化` 把它置为 **真**（`[Cls]:8-9`）。
* 只有 `置代码` / `置代码W`（以及经 `置代码UTF8`→`置代码W`）**读它**（`[Cls]:60-63`、`[Cls]:128-131`）：

```
.如果真 (集_是否为顶层 ＝ 假)
    清空 ()
    返回 (合并 (参数_JSON文本, , 参数_Unicode解码, 真))
.如果真结束
```

* 为什么必须这样：顶层节点由 `m_object` **独占**根引用，`置代码` 的正常路径是「建新对象 → `m_object.清除()`（释放旧根，`refCount` 1→0 → `Detch`+析构整棵树）→ 写入新指针 → 解析」。但对**子节点**（`K()`/`I()` 取出来的、或 `创建*` 返回的）来说：
  1. 它的引用数≥2（父容器也有一份），`清除` 不会真的销毁它，但会把它的 `jv_parent` **置 0**（`Detch`）→ 从原树摘除；
  2. 更要命的是 `置代码` 会**丢弃原节点**、换成一个全新的根对象 → 调用方拿到的 `Json2` 变量指向一个和原文档无关的孤儿节点；
  3. 所以设计成「非顶层 ＝ 就地改写」：先 `清空()` 保住节点本体与它在父中的位置（`jv_key`/`jv_parent` 不变），再 `合并(文本, , Unicode解码, 参数_是否添加 ＝ 真)` 把解析结果灌进去。
* 由此带来的**可观测差异**（必须照抄）：

| 场景 | 行为 |
| --- | --- |
| 顶层 `置代码 (T)` | 整棵树重建；根被解析结果**完整替换**；未传的 `参数_错误代码/位置` 只在文本非空时被写 |
| 顶层 `置代码 (“”)` | 只把 `m_object` 换成新的空**对象**并返回真（`[Cls]:47-56`）；`置代码W ({0,0})` 同（`[Cls]:116-125`） |
| 子节点 `置代码 (T)` | **不新建根**；`清空` + `合并(T, 路径缺省, USC2 ＝ 参数_Unicode解码, 是否添加 ＝ 真)` |
| 子节点 `置代码 (T)` 且该子节点**不是对象类型** | `JsonValue_Combine` 返回假 → 走 `合并` 的兜底：`json_.置代码 (T)` 再 `合并对象 (json_)` → `JsonValue_CombineObject` 会把目标**非对象节点 `SetObject` 清空成对象**再逐键写入 → 结果是一个对象（数字/字符串/数组原值丢失）。**若目标节点没有父**（`JsonValue_CombineProc` 的 else 分支：`key ＝ jv_key`、`parent ＝ GetParent`，两者任一为 0 就 `Release(descJson)` 并返回假）→ 整个 `合并` 返回**假** |
| `K2`/`I2` 拿到的子节点（`集_是否为顶层` 仍为真）再 `置代码` | **按顶层处理** → 从父树摘出并替换成新根 → 原文档里那个位置仍是旧值，但变量指向新节点（语义陷阱，`api-spec.md` H-2） |

## 8.4 父链对 `到文本` 的影响

* `JsonValue_ToStringProc` **只向下递归**（读 `jv_value` 容器里的子节点），从不读 `jv_parent` → **子节点只序列化自己那棵子树**，不会带出祖先的其它成员（`[Json]:2196-2295`）。
* `到文本` 前还会做 `JsonValue_SelectNoCreateW`（路径为空 → 当前节点）→ 所以对 `K("a")` 的结果调 `到文本()` 得到的是 `a` 的值本身（例如 `{"b":1}`），而不是 `{"a":{"b":1}}`。
* 父链**唯一**影响输出的间接途径：`JsonValue_CombineProc` 在类型不匹配时用 `jv_key ＋ jv_parent` 去 `JsonValue_Set (parent, key, descJson)`（`[Json]:3293-3306`）——即「合并时整节点替换」。这被 §8.3 的第二行场景使用。
* 另需注意：`JsonValue_GetAt`（`I`）对**对象**会 `SetObject→SetArray` 的销毁路径（§5.4）、`JsonValue_GetW`（`K`）对**数组**会 `SetObject` 销毁 → 一次「读」就能改变一棵子树，从而改变后续 `到文本`。

## 8.5 合并（`合并`/`合并W`/`合并对象`/`合并数组`/`合并对象Plus`）的顺序与所有权

`JsonValue_CombineProc`（`[Json]:3239-3308`）：

```
.如果 (typeSrc ＝ #JVTYPE_OBJECT 且 typeDesc ＝ #JVTYPE_OBJECT)
    countDesc ＝ JsonValue_GetValues (descJson, valuesDesc)
    .计次循环首 (countDesc, i)
        key ＝ _Get (valuesDesc [i], #jv_key)
        .如果 (JsonValue_ContainsKey (jsonValue, key))
            jvSrc ＝ JsonValue_Get (jsonValue, key)
            jv ＝ JsonValue_Clone (valuesDesc [i])
            ret ＝ JsonValue_CombineProc (jvSrc, jv, isAdd)
        .否则
            .如果真 (isAdd)
                jv ＝ JsonValue_Clone (valuesDesc [i])
                ret ＝ JsonValue_Set (jsonValue, key, jv)
            .如果真结束
        .如果结束
    .计次循环尾 ()
```

* **遍历顺序 ＝ descJson 的 eArray 顺序**；已存在的键原地合并/递归替换，**新键追加到 src 的有序数组末尾** → 合并后顺序＝「原有顺序 ＋ 新增键按 desc 出现次序排尾」。
* **返回值的坑**：`ret` 初值假，且**只有进入 `否则(isAdd)` 或已存在键的分支才会赋值** →
  * `descJson` 是空对象（`{}`）→ 循环 0 次 → **返回假** → 上层 `合并W` 的兜底（`json_.置代码W` + `合并对象`）被触发（`[Cls]:1405-1412`）。
  * `isAdd` 为假且键都不存在 → `ret` 保持**假** → 同样触发兜底（兜底路径是 `JsonValue_CombineObject`，它会 **无条件添加** 所有缺失键！）→ **`合并(T)` 传 `参数_是否添加 ＝ 假` 时，实际效果仍可能把新键加进来**。C++ 版要一致就必须照抄这条兜底。
* `JsonValue_Combine`（`[Json]:3222-3237`）：解析用的临时对象 `descJson ＝ JsonValue_Object (jsonValue)`（继承本节点堆），解析失败 → `Release(descJson)` → 假；成功后**所有权交给 CombineProc**（内部可能 `Set (parent, key, descJson)` 直接挂树，或在不匹配且无父时 `Release`）。
* `JsonValue_CombineObject`（`[Json]:3317-3369`）：要求源是 OBJECT；目标非 OBJECT 则 `SetObject`（销毁）；按 `desMap` 的 `htEx_eArray` 顺序逐个 `Clone` + `JsonValue_Set`（同键原地替换、新键追加）。
* `JsonValue_CombineArray`（`[Json]:3371-3416`）：要求源是 ARRAY；目标非 ARRAY 则 `SetArray`（销毁）；**按源顺序 `Append` 到目标末尾**（追加，不清目标）。
* `合并对象Plus` ＝ 先试 Object 语义，假则试 Array 语义（`[Cls]:1451-1460`）。

---

# 9. 附录 A：序列化输出精确规格（`JsonValue_ToStringProc`）

（这是「相同输入→相同输出」的另一半，`[Json]:2160-2295`；参数：`autoFormat`＝`参数_格式化`，`isTrans`＝`参数_是否转义`）

## 9.1 输出缓冲

* `zyString_Create (initBuffSize)`：`bufferLen ＝ 0` 时用 **1024** 字节（`[Str]:37-39`），堆＝`GetProcessHeap()`；结构 28 字节：`pFunctions(0) refCount(4) pParentThis(8) hHeap(12) pStr(16) bufferLen(20) strLen(24)`（`[Const]:74-77`）。
* 增长（`zyString_ReBuffer`，`[Str]:50-84`）：已有缓冲时 `newLen ＝ bufferLen × 2 ＋ lpStrLen × 2 ＋ 2`；触发条件是 `bufferLen ＜ (strLen ＋ len) × 2 ＋ 2`（**单位是字节**，比较时已 ×2）→ 追加后总长 ≤ `bufferLen - 2` 时不扩容。C++ 用 `std::u16string` 只需保证内容一致，容量可不一致（`参数_初始缓存大小` 不影响输出内容，只影响内存）。
* 追加函数一律**不计入终止符**：`len ＝ lstrlenW(...)`，写入 `len` 个宽字符，`strLen +＝ len`；`zyString_AddChar` 写 1 个宽字符（`[Str]:247-271`）。缓冲区尾部始终保留 0（`HEAP_ZERO_MEMORY` ＋ 每次预留 2 字节）。
* 取结果：`zyString_GetStrW` → `指针到字节集 (pStr, strLen × 2 ＋ 2)`（**带 `{0,0}`**，`[Str]:223-237`）；`zyString_GetStr` → `zyW2A_Point (pStr, 936)`（`[Str]:204-220`）。
  * ⚠ `zyW2A_Point` 用 `cchWideChar ＝ -1` 且 `取空白文本 (Ansi长度)`，而 `Ansi长度` 由 `WideCharToMultiByte(…, -1, …)` 得到**包含终止符** → 返回的 `文本型` 尾部**可能含一个 0 字节**（长度比可见字符多 1）。对拍时请用 `取文本长度` 验证（§9 T-5）。

## 9.2 各类型输出的字节级规则

| 类型 | 输出 |
| --- | --- |
| NULL | 字面量 `null`（`[Json]:2212`） |
| BOOL | `true` / `false`（由 `JsonValue_GetBool`：LONG/DOUBLE/BOOL 的 `≠0`）（`[Json]:2214`） |
| DOUBLE / LONG / UNDEFINED | `jv_raw` 原文；`raw ＝ 0` → **什么都不输出**（空）（`[Json]:2216-2220`） |
| STRING | `"` ＋（`isTrans` ? `TranslationOut (内容)` : 内容原样）＋ `"`；内容指针为 0 → 仅 `""`（`[Json]:2221-2231`） |
| ARRAY | `[` 元素 `,` 元素 `]`，**`autoFormat` 对数组完全无效**（无换行、无缩进）；元素顺序＝`Array_GetValues`（即插入/插入/排序后的顺序）；空数组 → `[]`（`[Json]:2233-2249`） |
| OBJECT | 见下（`[Json]:2251-2291`） |
| 其它（type 未知/节点 `jv_value ＝ 0`） | **什么都不输出**（连 `[]`/`{}` 都没有） |

OBJECT 完整规则（缩进 `indent` 是 **`参考` 变量**，`[Json]:2200`）：

```
{
  （autoFormat ? CRLF）
  对每个成员 i = 1..count（按 eArray 顺序）：
      （autoFormat 且 indent ＞ 0 ? Tab × indent）     ' indent 已 ＋1
      "  key（isTrans ? TranslationOut(key) : key 原样） "  :
      <子节点递归>
      （i ≠ count ? ,）
      （autoFormat ? CRLF）
  indent--
  （autoFormat 且 indent ＞ 0 ? Tab × indent）
}
```

* 换行是 **CRLF（`{13,0,10,0}`）**，缩进是 **Tab（`{9,0}`）**；`取重复字节集 (indent, { 9, 0 }) ＋ { 0, 0 }`（`[Json]:2263,2287`）。
* **根对象闭合前不缩进**（`indent` 已回到 0 → `indent ＞ 0` 不成立）；嵌套对象的闭合花括号前有 `父层 indent` 个 Tab。
* 空对象＋autoFormat → `{\r\n}`（**里面有换行**）。空对象无 autoFormat → `{}`。
* 成员之间是「`,` 然后 CRLF」，最后一个成员后**只有 CRLF 没有逗号**。
* 键**总是带引号**输出（无论解析时是否带引号）。
* 值侧的字符串内容**不做 \u 化**，非 ASCII 原样宽字符输出（→ ANSI 版会按 cp936 落地）。

## 9.3 `到文本` / `取文本` 的额外收尾（类层，会改写输出）

* `到文本(W)`：路径用 `SelectNoCreateW`；`参数_是否转义` 默认**真**；`参数_初始缓存大小 ≤ 0 → 1024`（`[Cls]:308-339`）。节点为 0 → 返回 `{0,0}` / `“”`。
* `取文本(W)` 先试 `JsonValue_GetStringExA/W`（STRING→内容；NULL/BOOL/LONG/DOUBLE→raw；其它→空），为空再退化为整棵子树的 `到文本`，并把 `“”`、`{}`、`[]`、`null` 统一映射为**空**（`[Cls]:1069-1167`）→ 因此 `取文本` 对数字 `0` 返回 `"0"`（非空），但对 `null` 成员返回**空**。

---

# 10. 附录 B：C++ 移植前的「必须先实测」清单

下列点**无法只靠源码 100% 定论**，本文已在对应小节标注；建议用现有 `.fne`/易语言 IDE 各跑一次并把结果当黄金用例。

| # | 待定问题 | 相关位置 | 决定性用例 |
| --- | --- | --- | --- |
| T-1 | `到数值(GBK文本)` 与 `strtod` 的差异（`1e5`、`.5`、`1.2.3` 前置部分、溢出、`+`） | `[Help]:649` | `置代码 (<{"a":1e400}>)` → `取双精度` |
| T-2 | **§6.6 的 `或/且` 优先级**（字符串内空格是否被吞） | `[Json]:1694,1792` | 首选 `置代码 (“{””a b””:1}”)` → `到文本`（R1 给 `{"a ":1}`，R2 给 `{"a b":1}`）；再用 `置代码 (“{””a””:””outdoor ””}”)`（R1 报 -6） |
| T-3 | `StrToInt` 返回值语义（路径 `[n]` 的真实索引）＋ `JsonValue_GetInt` 对 STRING 走它 | `[Json]:2415,440`、`[DLL]:213` | `置文本 ("a[2]", "X")` → `到文本`；`置文本("s","78"); 取整数("s")` |
| T-4 | `到文本(双精度)` 的格式化规则（决定 `置双精度` 与 `Clone` 后的数字输出） | `[Json]:284,335,416,466`、`[Json]:2949-2952` | `创建双精度(1/3).到文本()`、`置双精度(1.5E+20)`，以及 `复制` 一个解析出的 `1.23456789012345` |
| T-5 | `到文本()`（A 版）返回文本是否含尾部 0 字节 | `[Help]:57-72`、`[Str]:204-220` | `取文本长度 (json.到文本 ())` |
| T-6 | `_wtoi64` 溢出行为（>2^63 的整数 token） | `[Help]:684` | `置代码 (<{"a":9223372036854775808}>)` → `取长整数`、`到文本` |
| T-7 | `\u0000` 造成的静默截断（存储与输出） | `[Json]:3073-3082,2006` | `置代码 (<{"a":"x\u0000y"}>)` → `到文本` |
| T-8 | `_Array_FastSort` 在相等元素上的次序（自定义比较返回 0） | `[Arr]:460-513` | `排序` 6 个成员，比较函数恒返回 0 |
| T-9 | `排序(W)` 的 `参数_算法` 是否真的被忽略（写死 0） | `[Json]:2864,2873` | `排序(, , , 1)` 与 T-8 结果比对 |
| T-10 | `置文本W` 的终止符判断用了 `取字节集左边`（H-6） | `[Cls]:925-927` | 连续两次 `置文本W` 同一变量后 `取文本W` |

## 10.1 落地优先级建议（按「对输出影响」排序）

1. §4 顺序（插入序 + 覆盖保位 + 读取即插入 null）——错则所有多键对象的输出都错。
2. §3.5 + §2.5 数字原文回放（raw）与「容器内整数＝DOUBLE 型」——错则数字回写与 `取类型` 错。
3. §6.7/§6.8 转义双向表（`\n`→CRLF、`/`→`\/`、未解码 `\u` 保留再转义）——错则字符串回写错。
4. §6.10 宽松语法（尾逗号/单引号/无引号键/合并根）+ §6.9 错误码与位置语义。
5. §7 路径的自动创建/补齐规则（读操作会改文档）。
6. §8 引用计数与 `集_是否为顶层` 的合并分支。
7. §5/§1.5 容器容量与堆（只影响内存占用与调试对齐，不影响输出）。


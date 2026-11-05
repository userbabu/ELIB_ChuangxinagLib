// 由 gen_json2_table.mjs 从 zyJson2.51 原模块生成，勿手改。
// Json2 成员命令的操作码与实现名；顺序即命令表顺序（只能往末尾追加）。
#ifndef CX_JSON2_TABLE_H
#define CX_JSON2_TABLE_H

JSON_CMD(J2ObjConstruct); JSON_CMD(J2ObjCopy); JSON_CMD(J2ObjDestruct);

#define J2OP_SetCode 1
#define J2OP_SetCodeUtf8 2
#define J2OP_SetCodeW 3
#define J2OP_Release 4
#define J2OP_SetTopFlag 5
#define J2OP_GetPtr 6
#define J2OP_SetPtr 7
#define J2OP_CreateObject 8
#define J2OP_CreateArray 9
#define J2OP_CreateNull 10
#define J2OP_CreateBool 11
#define J2OP_CreateTextW 12
#define J2OP_CreateText 13
#define J2OP_CreateDouble 14
#define J2OP_CreateLong 15
#define J2OP_ParseW 16
#define J2OP_Parse 17
#define J2OP_ToTextW 18
#define J2OP_ToText 19
#define J2OP_GetTyp 20
#define J2OP_GetTypW 21
#define J2OP_GetParent 22
#define J2OP_GetParentW 23
#define J2OP_GetKeyCount 24
#define J2OP_GetKeyCountW 25
#define J2OP_Detach 26
#define J2OP_RemoveAttrW 27
#define J2OP_RemoveAttr 28
#define J2OP_RemoveItem 29
#define J2OP_RemoveItemW 30
#define J2OP_Clear 31
#define J2OP_ClearW 32
#define J2OP_SetAttr 33
#define J2OP_SetAttrW 34
#define J2OP_K 35
#define J2OP_KUtf8 36
#define J2OP_KW 37
#define J2OP_K2 38
#define J2OP_K2W 39
#define J2OP_GetAttrNameW 40
#define J2OP_GetAttrName 41
#define J2OP_AppendItem 42
#define J2OP_AppendItemW 43
#define J2OP_InsertItem 44
#define J2OP_InsertItemW 45
#define J2OP_I 46
#define J2OP_IW 47
#define J2OP_I2 48
#define J2OP_I2W 49
#define J2OP_SetItem 50
#define J2OP_SetItemW 51
#define J2OP_ExistsW 52
#define J2OP_Exists 53
#define J2OP_Select 54
#define J2OP_SelectW 55
#define J2OP_SetObject 56
#define J2OP_SetObjectW 57
#define J2OP_SetArray 58
#define J2OP_SetArrayW 59
#define J2OP_SetText 60
#define J2OP_SetTextUtf8 61
#define J2OP_SetTextW 62
#define J2OP_SetNullW 63
#define J2OP_SetNull 64
#define J2OP_SetBoolW 65
#define J2OP_SetBool 66
#define J2OP_SetDoubleW 67
#define J2OP_SetDouble 68
#define J2OP_SetNumberW 69
#define J2OP_SetNumber 70
#define J2OP_SetLongW 71
#define J2OP_SetLong 72
#define J2OP_SetIntW 73
#define J2OP_SetInt 74
#define J2OP_GetRaw 75
#define J2OP_GetRawW 76
#define J2OP_GetText 77
#define J2OP_GetTextUtf8 78
#define J2OP_GetTextW 79
#define J2OP_GetBoolW 80
#define J2OP_GetBoolA 81
#define J2OP_GetDoubleW 82
#define J2OP_GetDoubleA 83
#define J2OP_GetNumberW 84
#define J2OP_GetNumberA 85
#define J2OP_GetLongW 86
#define J2OP_GetLongA 87
#define J2OP_GetIntW 88
#define J2OP_GetIntA 89
#define J2OP_GetAllKeysW 90
#define J2OP_GetAllKeys 91
#define J2OP_GetAllValuesW 92
#define J2OP_GetAllValues 93
#define J2OP_CopyW 94
#define J2OP_Copy 95
#define J2OP_SortObjW 96
#define J2OP_SortObj 97
#define J2OP_SortW 98
#define J2OP_Sort 99
#define J2OP_MergeW 100
#define J2OP_Merge 101
#define J2OP_MergeObjPlus 102
#define J2OP_MergeObj 103
#define J2OP_MergeArr 104
#define J2OP_InsertAttrW 105
#define J2OP_InsertAttr 106
#define J2OP_SetTextAry 107
#define J2OP_SetTextAryW 108
#define J2OP_SetTextAryUtf8 109
#define J2OP_SetNumberAryUtf8 110
#define J2OP_SetIntAryUtf8 111
#define J2OP_SetBoolAryUtf8 112

JSON_CMD(J2SetCode);
JSON_CMD(J2SetCodeUtf8);
JSON_CMD(J2SetCodeW);
JSON_CMD(J2Release);
JSON_CMD(J2SetTopFlag);
JSON_CMD(J2GetPtr);
JSON_CMD(J2SetPtr);
JSON_CMD(J2CreateObject);
JSON_CMD(J2CreateArray);
JSON_CMD(J2CreateNull);
JSON_CMD(J2CreateBool);
JSON_CMD(J2CreateTextW);
JSON_CMD(J2CreateText);
JSON_CMD(J2CreateDouble);
JSON_CMD(J2CreateLong);
JSON_CMD(J2ParseW);
JSON_CMD(J2Parse);
JSON_CMD(J2ToTextW);
JSON_CMD(J2ToText);
JSON_CMD(J2GetTyp);
JSON_CMD(J2GetTypW);
JSON_CMD(J2GetParent);
JSON_CMD(J2GetParentW);
JSON_CMD(J2GetKeyCount);
JSON_CMD(J2GetKeyCountW);
JSON_CMD(J2Detach);
JSON_CMD(J2RemoveAttrW);
JSON_CMD(J2RemoveAttr);
JSON_CMD(J2RemoveItem);
JSON_CMD(J2RemoveItemW);
JSON_CMD(J2Clear);
JSON_CMD(J2ClearW);
JSON_CMD(J2SetAttr);
JSON_CMD(J2SetAttrW);
JSON_CMD(J2K);
JSON_CMD(J2KUtf8);
JSON_CMD(J2KW);
JSON_CMD(J2K2);
JSON_CMD(J2K2W);
JSON_CMD(J2GetAttrNameW);
JSON_CMD(J2GetAttrName);
JSON_CMD(J2AppendItem);
JSON_CMD(J2AppendItemW);
JSON_CMD(J2InsertItem);
JSON_CMD(J2InsertItemW);
JSON_CMD(J2I);
JSON_CMD(J2IW);
JSON_CMD(J2I2);
JSON_CMD(J2I2W);
JSON_CMD(J2SetItem);
JSON_CMD(J2SetItemW);
JSON_CMD(J2ExistsW);
JSON_CMD(J2Exists);
JSON_CMD(J2Select);
JSON_CMD(J2SelectW);
JSON_CMD(J2SetObject);
JSON_CMD(J2SetObjectW);
JSON_CMD(J2SetArray);
JSON_CMD(J2SetArrayW);
JSON_CMD(J2SetText);
JSON_CMD(J2SetTextUtf8);
JSON_CMD(J2SetTextW);
JSON_CMD(J2SetNullW);
JSON_CMD(J2SetNull);
JSON_CMD(J2SetBoolW);
JSON_CMD(J2SetBool);
JSON_CMD(J2SetDoubleW);
JSON_CMD(J2SetDouble);
JSON_CMD(J2SetNumberW);
JSON_CMD(J2SetNumber);
JSON_CMD(J2SetLongW);
JSON_CMD(J2SetLong);
JSON_CMD(J2SetIntW);
JSON_CMD(J2SetInt);
JSON_CMD(J2GetRaw);
JSON_CMD(J2GetRawW);
JSON_CMD(J2GetText);
JSON_CMD(J2GetTextUtf8);
JSON_CMD(J2GetTextW);
JSON_CMD(J2GetBoolW);
JSON_CMD(J2GetBoolA);
JSON_CMD(J2GetDoubleW);
JSON_CMD(J2GetDoubleA);
JSON_CMD(J2GetNumberW);
JSON_CMD(J2GetNumberA);
JSON_CMD(J2GetLongW);
JSON_CMD(J2GetLongA);
JSON_CMD(J2GetIntW);
JSON_CMD(J2GetIntA);
JSON_CMD(J2GetAllKeysW);
JSON_CMD(J2GetAllKeys);
JSON_CMD(J2GetAllValuesW);
JSON_CMD(J2GetAllValues);
JSON_CMD(J2CopyW);
JSON_CMD(J2Copy);
JSON_CMD(J2SortObjW);
JSON_CMD(J2SortObj);
JSON_CMD(J2SortW);
JSON_CMD(J2Sort);
JSON_CMD(J2MergeW);
JSON_CMD(J2Merge);
JSON_CMD(J2MergeObjPlus);
JSON_CMD(J2MergeObj);
JSON_CMD(J2MergeArr);
JSON_CMD(J2InsertAttrW);
JSON_CMD(J2InsertAttr);
JSON_CMD(J2SetTextAry);
JSON_CMD(J2SetTextAryW);
JSON_CMD(J2SetTextAryUtf8);
JSON_CMD(J2SetNumberAryUtf8);
JSON_CMD(J2SetIntAryUtf8);
JSON_CMD(J2SetBoolAryUtf8);

#endif  // CX_JSON2_TABLE_H

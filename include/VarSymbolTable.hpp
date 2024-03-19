#define VarSymbolTableSize 10000

// 返回标识符在变量表中的序号，若不存在，则添加进变量表
// 返回值:
//  >=0 : 标识符序号
//  -1  : 变量符号表已满
//  -2  : 内存资源不足
int var(char *ident);
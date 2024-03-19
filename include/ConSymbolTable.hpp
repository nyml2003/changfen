#ifndef CHANG_FEN_LEX_CON_SYMBOL_TABLE_HPP
#define CHANG_FEN_LEX_CON_SYMBOL_TABLE_HPP
typedef int TableItemId;

#ifdef __cplusplus
extern "C" {
#endif
/**
 * @brief 添加标识符(Identifiers)
 * @param str 标识符字符串
 * @return void
 * @note 会创建一个新的标识符项，并将其添加到符号表中
 */
void appendIdent(char *str);
/**
 * @brief 添加符号(Signs)
 * @param id 符号项的序号，定义在lexical_analyzer.h中
 * @return void
 * @note 会创建一个新的符号项，并将其添加到符号表中
 */
void appendSign(TableItemId id);
/**
 * @brief 添加常数(Constants)
 * @param num 常数值
 * @return void
 * @note 会创建一个新的常数项，并将其添加到符号表中
 */
void appendConstant(int num);
/**
 * @brief 添加错误项
 * @param error 未识别的字符
 * @return void
 * @note 会创建一个新的错误项，并将其添加到符号表中
 */
void appendError(char error);
/**
 * @brief 输出符号表
 * @return void
 * @note 输出符号表中的所有项
 */
void put();
/**
 * @brief 输出符号表
 * @return void
 * @note 输出符号表中的所有项，并在每个项后输出一个换行符
 */
void putln();
/**
 * @brief 添加常数项到常数表
 * @param num 常数字符串
 * @return 常数项的序号
 * @note 兼容VarSymbolTable中的var函数
 */
int con(char *num);
#ifdef __cplusplus
};
#endif

#endif // CHANG_FEN_LEX_CON_SYMBOL_TABLE_HPP
//g++ "ConSymbolTable.cpp" -fPIC -shared -o libConSymbolTable.so
// gcc -o rubbish lexical_analyzer.c -L ./ -lVarSymbolTable -lConSymbolTable
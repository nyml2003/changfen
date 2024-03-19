#ifndef CHANG_FEN_LEX_EXAM_TABLE_HPP
#define CHANG_FEN_LEX_EXAM_TABLE_HPP
typedef int TableItemId;

#ifdef __cplusplus
extern "C" {
#endif


typedef struct TableItem TableItem;
typedef struct LexTable LexTable;
typedef struct ErrorItem ErrorItem;
typedef struct IdentItem IdentItem;
typedef struct SignItem SignItem;
typedef struct IntItem IntItem;

/**
 * @brief 添加一个标识符
 * @param str
 */
void appendIdent(char *str);
/**
 * @brief 添加一个标识符, 并记录行号
 * @param str
 * @param line
 */
void appendIdentWithLine(char *str, int line);
/**
 * @brief 添加一个符号
 * @param id
 */
void appendSign(TableItemId id);
/**
 * @brief 添加一个符号, 并记录行号
 * @param id
 * @param line
 */
void appendSignWithLine(TableItemId id, int line);
/**
 * @brief 添加一个整数
 * @param num
 */
void appendConstant(char *num);
/**
 * @brief 添加一个整数, 并记录行号
 * @param num
 * @param line
 */
void appendConstantWithLine(char* num, int line);
/**
 * @brief 添加一个错误
 * @param error
 */
void appendError(char* error);
/**
 * @brief 添加一个错误, 并记录行号
 * @param error
 * @param line
 */
void appendErrorWithLine(char* error, int line);
/**
 * @brief 输出表中的内容
 */
void put();
/**
 * @brief 输出表中的内容, 并换行
 * @param line
 */
void putln();

#ifdef __cplusplus
};
#endif

#endif // CHANG_FEN_LEX_EXAM_TABLE_HPP

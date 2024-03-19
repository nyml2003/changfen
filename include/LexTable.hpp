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

void appendIdent(char *str);
void appendIdentWithLine(char *str, int line);
void appendSign(TableItemId id);
void appendSignWithLine(TableItemId id, int line);
void appendConstant(char *num);
void appendConstantWithLine(char* num, int line);
void appendError(char* error);
void appendErrorWithLine(char* error, int line);
void put();
void putln();

#ifdef __cplusplus
};
#endif

#endif // CHANG_FEN_LEX_EXAM_TABLE_HPP

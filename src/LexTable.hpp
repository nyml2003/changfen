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

LexTable *newLexTable();
void destroyTable(LexTable *table);
void appendIdent(LexTable *table, char *str);
void appendIdentWithLine(LexTable *table, char *str, int line);
void appendSign(LexTable *table, TableItemId id);
void appendSignWithLine(LexTable *table, TableItemId id, int line);
void appendConstant(LexTable *table, char *num);
void appendConstantWithLine(LexTable *table, char* num, int line);
void appendError(LexTable *table, char* error);
void appendErrorWithLine(LexTable *table, char* error, int line);
void put(LexTable *table);
void putln(LexTable *table);

#ifdef __cplusplus
};
#endif

#endif // CHANG_FEN_LEX_EXAM_TABLE_HPP

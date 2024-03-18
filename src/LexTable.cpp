//
// Created by venty on 2024/3/13.
//
#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS 
#endif
#include "LexTable.hpp"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "lexical_analyzer.h"
#include <vector>

const char* tokenToString[]={
        "empty",
        "identiter",
        "constant",
        "int",
        "if",
        "else",
        "while",
        "for",
        "read",
        "write",
        "+",
        "-",
        "*",
        "/",
        "<",
        "<=",
        ">",
        ">=",
        "!=",
        "==",
        "=",
        "(",
        ")",
        ",",
        ";",
        "{",
        "}",
        "return",
        "break",
        "continue",
        "&&",
        "||",
        "&",
        "|",
        "!",
        "^",
        "<<",
        ">>",
        "[",
        "]"
};

struct TableItem
{
    TableItem(){
        line = -1;
    }
    TableItem(int line){
        this->line = line;
    }
    TableItemId id;
    int line;
    virtual void put() = 0;
    virtual ~TableItem() = default;
};

struct ErrorItem : TableItem {
    char* error;
    ErrorItem(char* error){
        this->id = -1;
        this->error = (char *)malloc(strlen(error) + 1);
        strcpy(this->error, error);
    }
    ErrorItem(char* error, int line): TableItem(line){
        this->id = -1;
        this->error = (char *)malloc(strlen(error) + 1);
        strcpy(this->error, error);
    }
    void put() override {
        if (line == -1)
            printf("error: '%s'\n", error);
        else printf("error: '%s' at line %d\n", error, line);
    }
    ~ErrorItem() {
        free(error);
    }
};

struct IdentItem : TableItem {
    char *name;
    IdentItem(char *str){
        this->id = ID_IDENTITER;
        this->name = (char *)malloc(strlen(str) + 1);
        strcpy(this->name, str);
    }
    IdentItem(char *str, int line): TableItem(line){
        this->id = ID_IDENTITER;
        this->name = (char *)malloc(strlen(str) + 1);
        strcpy(this->name, str);
    }
    void put() override {
        if (line == -1)
            printf("( %d, %s )", id, name);
        else printf("at line %d: ( %s, %s )", line, tokenToString[id], name);
    }
    ~IdentItem(){
        free(name);
    }
};

struct SignItem : TableItem {
    SignItem(TableItemId id){
        this->id = id;
    }
    SignItem(TableItemId id, int line): TableItem(line){
        this->id = id;
    }
    void put() override {
        if (line == -1)
            printf("( %d )", id);
        else printf("at line %d: ( sign, %s )", line, tokenToString[id]);
    }
    ~SignItem() {
    }
};


struct IntItem : TableItem {
    int num;
    IntItem(int num){
        this->id = ID_CONSTANT;
        this->num = num;
    }
    IntItem(int num, int line): TableItem(line){
        this->id = ID_CONSTANT;
        this->num = num;
    }
    void put() override{
        if (line == -1)
            printf("( %d, %d )", id, num);
        else printf("at line %d: ( %s, %d )", line, tokenToString[id], num);
    }
    ~IntItem() {
    }
};

struct LexTable {
    std::vector<TableItem *> table;
    LexTable()
    {
        table.clear();
    }
    void appendIdent(char *str){
        table.push_back(new IdentItem(str));
    }
    void appendIdent(char *str, int line){
        table.push_back(new IdentItem(str, line));
    }
    void appendSign(TableItemId id){
        table.push_back(new SignItem(id));
    }
    void appendSign(TableItemId id, int line){
        table.push_back(new SignItem(id, line));
    }
    void appendConstant(int num){
        table.push_back(new IntItem(num));
    }
    void appendConstant(int num, int line){
        table.push_back(new IntItem(num, line));
    }
    void appendError(char* error){
        table.push_back(new ErrorItem(error));
    }
    void appendError(char* error, int line){
        table.push_back(new ErrorItem(error, line));
    }
    void put(){
        for (auto &item : table) {
            item->put();
        }
    }
    void putln(){
        for (auto &item : table) {
            item->put();
            printf("\n");
        }
    }
    ~LexTable()
    {
        for (auto &item : table) {
            delete item;
        }
        table.clear();
    }
};

extern "C" LexTable *newLexTable(){
    return new LexTable();
}

extern "C" void destroyTable(LexTable *table)
{
    delete table;
}

extern "C" void appendIdent(LexTable *table, char *str){
    table->appendIdent(str);
}

extern "C" void appendIdentWithLine(LexTable *table, char *str, int line)
{
    table->appendIdent(str, line);
}

extern "C" void appendSign(LexTable *table, TableItemId id){
    table->appendSign(id);
}

extern "C" void appendSignWithLine(LexTable *table, TableItemId id, int line)
{
    table->appendSign(id, line);
}

extern "C" void appendConstant(LexTable *table, char *num){
    table->appendConstant(atoi(num));
}

extern "C" void appendConstantWithLine(LexTable *table, char* num, int line)
{
    table->appendConstant(atoi(num), line);
}

extern "C" void appendError(LexTable *table, char* error){
    table->appendError(error);
}

extern "C" void appendErrorWithLine(LexTable *table, char* error, int line){
    table->appendError(error, line);
}

extern "C" void put(LexTable *table){
    table->put();
}

extern "C" void putln(LexTable *table){
    table->putln();
}





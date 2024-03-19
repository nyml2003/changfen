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
            printf("( %s, %s )", tokenToString[id], name);
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
            printf("( sign, %s )", tokenToString[id]);
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
            printf("( %s, %d )", tokenToString[id], num);
        else printf("at line %d: ( %s, %d )", line, tokenToString[id], num);
    }
    ~IntItem() {
    }
};

struct LexTable {
    std::vector<TableItem *> table;
    static LexTable *instance;
    static LexTable *getInstance(){
        if (instance == nullptr)
            instance = new LexTable();
        return instance;
    }
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

LexTable *LexTable::instance = nullptr;


extern "C" void appendIdent(char *str){
    LexTable::getInstance()->appendIdent(str);
}

extern "C" void appendIdentWithLine(char *str, int line)
{
    LexTable::getInstance()->appendIdent(str, line);
}

extern "C" void appendSign(TableItemId id){
    LexTable::getInstance()->appendSign(id);
}

extern "C" void appendSignWithLine(TableItemId id, int line)
{
    LexTable::getInstance()->appendSign(id, line);
}

extern "C" void appendConstant( char *num){
    LexTable::getInstance()->appendConstant(atoi(num));
}

extern "C" void appendConstantWithLine(char* num, int line)
{
    LexTable::getInstance()->appendConstant(atoi(num), line);
}

extern "C" void appendError(char* error){
    LexTable::getInstance()->appendError(error);
}

extern "C" void appendErrorWithLine(char* error, int line){
    LexTable::getInstance()->appendError(error, line);
}

extern "C" void put(){
    LexTable::getInstance()->put();
}

extern "C" void putln(){
    LexTable::getInstance()->putln();
}





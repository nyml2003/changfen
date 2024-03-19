#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS 
#endif
#include "ConSymbolTable.hpp"
#include "stdio.h"
#include "string.h"
#include "lexical_analyzer.h"
#include <vector>
#include "stdlib.h"

struct TableItem
{
    TableItemId id;
    virtual void put() = 0;
    virtual ~TableItem() = default;
};

struct ErrorItem : TableItem {
    char error;
    ErrorItem(char error){
        this->id = -1;
        this->error = error;
    }
    void put() override {
        printf("error: '%c'", error);
    }
    ~ErrorItem() {
    }
};

struct IdentItem : TableItem {
    char *name;
    IdentItem(char *str){
        this->id = ID_IDENTITER;
        this->name = (char *)malloc(strlen(str) + 1);
        strcpy(this->name, str);
    }
    void put() override {
        printf("( %d, %s )", id, name);
    }
    ~IdentItem(){
        free(name);
    }
};

struct SignItem : TableItem {
    SignItem(TableItemId id){
        this->id = id;
    }
    void put() override {
        printf("( %d, - )", id);
    }
    ~SignItem() {
    }
};


struct IntItem : TableItem {
    int num;
    IntItem(char* num, int radix){
        this->id = ID_CONSTANT;
        this->num = strtol(num, nullptr, radix);
    }
    void put() override{
        printf("( %d, %d )", id, num);
    }
    ~IntItem() {
    }
};

struct ConSymbolTable {
    std::vector<TableItem *> table;
    static ConSymbolTable * instance;
    ConSymbolTable()
    {
        table.clear();
    }
    static ConSymbolTable *getInstance()
    {
        if (instance == nullptr)
        {
            instance = new ConSymbolTable();
        }
        return instance;
    }
    void appendIdent(char *str){
        table.push_back(new IdentItem(str));
    }
    void appendSign(TableItemId id){
        table.push_back(new SignItem(id));
    }
    void appendConstant(char* num, int radix){
        table.push_back(new IntItem(num, radix));
    }
    void appendError(char error){
        table.push_back(new ErrorItem(error));
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
    ~ConSymbolTable()
    {
        for (auto &item : table) {
            delete item;
        }
        table.clear();
    }
};

ConSymbolTable *ConSymbolTable::instance = nullptr;



extern "C" void appendIdent( char *str){
    ConSymbolTable::getInstance()->appendIdent(str);
}

extern "C" void appendSign(TableItemId id){
    ConSymbolTable::getInstance()->appendSign(id);
}

extern "C" void appendConstant(char *num, int radix){
    ConSymbolTable::getInstance()->appendConstant(num, radix);
}

extern "C" int con(char *num, int radix){
    ConSymbolTable::getInstance()->appendConstant(num, radix);
    return ConSymbolTable::getInstance()->table.size() - 1;
}

extern "C" void appendError(char error){
    ConSymbolTable::getInstance()->appendError(error);
}

extern "C" void put(){
    ConSymbolTable::getInstance()->put();
}

extern "C" void putln(){
    ConSymbolTable::getInstance()->putln();
}

extern "C" int getLastConstant(){
    return ((IntItem *)ConSymbolTable::getInstance()->table.back())->num;
}



#include <cassert>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <leveldb/table.h>
#include <filename.h>
#include <dirent.h>
#include <sys/stat.h>
#include <zconf.h>
#include "leveldb/db.h"
#include "leveldb/options.h"
#include "leveldb/env.h"
#include "db/version_set.h"
#include "db/version_edit.h"
#include "leveldb/comparator.h"

using namespace std;
using namespace leveldb;

string Trim(string& str)
{
    str.erase(0,str.find_first_not_of(" \t\r\n"));
    str.erase(str.find_last_not_of(" \t\r\n") + 1);
    return str;
}

bool ValueInTable( leveldb::Table* table, std::string value);

void GetFirstEntry(leveldb::Table* table, std::string* key,std::string* value);

bool ValueInTable(leveldb::Table *table, std::string value) {
    uint64_t seq = 2;
    std::string tmp_value = value;
    leveldb::PutFixed64(&tmp_value, seq);
    return table->ValueInTable(value);
}

void GetFirstEntry(leveldb::Table *table, std::string *key, std::string *value) {
    leveldb::Iterator *table_iter = table->NewIterator(leveldb::ReadOptions());
    table_iter->SeekToFirst();
    if (table_iter->Valid()) {
        leveldb::ParsedInternalKey ikey;
        leveldb::ParseInternalKey(table_iter->key(), &ikey);
        *key = ikey.user_key.ToString();
        *value = table_iter->value().ToString();
//        std::cout << std::endl << "first table's first entry:" << std::endl << "user_key:" << user_key << ", user_value:" << user_value << std::endl << std::endl;
    }
}

leveldb::Status BuildTable_0_100000(leveldb::DB *db);

leveldb::Status MyOpen(std::string db_name, leveldb::DB **db, leveldb::Options &options) {
    bool new_db = false;

    options.comparator = MyComparator();
    leveldb::Status status = leveldb::DB::Open(options, db_name, db);
    if (!status.ok()) {
        new_db = true;
        delete (*db);
        options.create_if_missing = true;
        status = leveldb::DB::Open(options, db_name, db);
        assert(status.ok());
        status = BuildTable_0_100000(*db);
    }

    if (true == new_db) {
        delete (*db);
        status = leveldb::DB::Open(options, db_name, db);
    }

    if (status.ok() && (*db) != nullptr) {
        std::cout << "MyOpen db successful." << std::endl;
    }

    return status;
}




leveldb::Status
OpenFirstTable(std::string db_name, leveldb::DB *db, const leveldb::Options &options, leveldb::Table **table);

// 往db中写入 key(0-100000), value(100000-199999)
leveldb::Status BuildTable_0_100000(leveldb::DB *db) {

    leveldb::Status status;
    for (int i = 0; i < 1000; i++) {
        status = db->Put(leveldb::WriteOptions(), std::to_string(i), std::to_string(i + 1000));
        assert(status.ok());
    }
    std::cout << "Put entries 0-100000 successful." << std::endl;
    return status;
}

void Getfilepath(const char *path, const char *filename,  char *filepath)
{
    strcpy(filepath, path);
    if(filepath[strlen(path) - 1] != '/')
        strcat(filepath, "/");
    strcat(filepath, filename);
//    printf("path is = %s\n",filepath);
}

bool DeleteFile(const char* path) {
    DIR *dir;
    struct dirent *dirinfo;
    struct stat statbuf;
    char filepath[256] = {0};
    lstat(path, &statbuf);

    if (S_ISREG(statbuf.st_mode))//判断是否是常规文件
    {
        remove(path);
    } else if (S_ISDIR(statbuf.st_mode))//判断是否是目录
    {
        if ((dir = opendir(path)) == NULL)
            return 1;
        while ((dirinfo = readdir(dir)) != NULL) {
            Getfilepath(path, dirinfo->d_name, filepath);
            if (strcmp(dirinfo->d_name, ".") == 0 || strcmp(dirinfo->d_name, "..") == 0)//判断是否是特殊目录
                continue;
            DeleteFile(filepath);
            rmdir(filepath);
        }
        closedir(dir);
    }
    return 0;
}

leveldb::FileMetaData *GetTargetFile(uint64_t file_number, leveldb::DB *db);

uint64_t GetFirstFileNUmber(leveldb::DB *db) {
    leveldb::Version *current = db->GetCurrentVersion();
    return current->GetFiles(0)[0]->number;
}

leveldb::Status
OpenFirstTable(std::string db_name, leveldb::DB *db, const leveldb::Options &options, leveldb::Table **table) {
    leveldb::Status status;
    leveldb::FileMetaData *target_file = GetTargetFile(GetFirstFileNUmber(db), db);
    leveldb::RandomAccessFile *file = nullptr;
    status = options.env->NewRandomAccessFile(leveldb::TableFileName(db_name, target_file->number), &file);
    status = leveldb::Table::Open(options, file, target_file->file_size, table);

    return status;
}

leveldb::FileMetaData *GetTargetFile(uint64_t file_number, leveldb::DB *db) {
//    std::cout << "In GetTargetFile. " << std::endl;
    leveldb::Version *current = db->GetCurrentVersion();
    for (int i = 0; i < leveldb::config::kNumLevels; i++) {
        std::vector<leveldb::FileMetaData *> files_ = current->GetFiles(i);
        for (leveldb::FileMetaData *file : files_) {
            if (file_number == file->number) {
                return file;
            }
        }
    }
}

// 打开数据库
leveldb::Status MyOpen(std::string db_name, leveldb::DB **db, leveldb::Options &options);

void TEST_Filter(std::string db_name){
    DeleteFile(db_name.data());

    leveldb::DB *db;
    leveldb::Options options;
    options.filter_policy = leveldb::NewBloomFilterPolicy(10); // 指定 bloomfilter
    leveldb::Status status = MyOpen(db_name, &db, options);
    assert(status.ok());

    std::cout << "filter name : " << options.filter_policy->Name() << std::endl;

// 打开第一个 table
    leveldb::Table *table = nullptr;
    assert(OpenFirstTable(db_name, db, options, &table).ok());

// 获取第一个 table 的第一个 key,value
// user_key:0, user_value:100000
    std::string key;
    std::string value;
    GetFirstEntry(table, &key, &value);
    bool flag = ValueInTable(table, value);
    std::cout << (true == flag ? "TEST_Filter success" : "TEST_Filter failed")<<std::endl;
}


int main() {
    TEST_Filter("./testFilter223.db");
}

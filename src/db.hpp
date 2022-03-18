#include <iostream>
#include <sqlite3.h>
#include <stdexcept>
#include <string>
#include <vector>

struct Chapter {
    int id;
    std::string title;
    std::string body;
    bool include;

    void print()
    {
        std::cout << this->id << std::endl;
        std::cout << this->title << std::endl;
        std::cout << this->body << std::endl;
        std::cout << (this->include ? "true" : "false") << std::endl;
    }
};

class Database {
public:
    Database(std::string);
    Database(std::string, std::string, std::string, double);
    ~Database() { sqlite3_close(db); }

    void init_common(std::string);

    int close() { return sqlite3_close(db); }

    int update_metadata(std::string, std::string, double);
    int update_title(std::string);
    int insert(std::string);

    std::string get_body(int);
    std::vector<Chapter> get_chapters();

    int set_body(int, std::string);
    int set_include(int, bool);
    int set_title(int, std::string);

private:
    sqlite3* db;

    static int exec_cb(void*, int, char**, char**);
    static int all_chapters_cb(void*, int, char**, char**);
    static int body_cb(void*, int, char**, char**);
};
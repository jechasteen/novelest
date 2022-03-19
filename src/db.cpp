#include "db.hpp"
#include <sstream>

using Chapters = std::vector<Chapter>;

Database::Database(std::string filename)
{
    init_common(filename);
}

Database::Database(std::string filename, std::string title, std::string author, double target)
{
    init_common(filename);

    std::stringstream sql;
    sql << "INSERT INTO metadata(title, author, target) "
        << "VALUES('" << title << "','" << author << "'," << target << ");";

    char* errmsg;
    int rc = sqlite3_exec(db, sql.str().c_str(), exec_cb, nullptr, &errmsg);
    if (rc != SQLITE_OK) {
        std::cerr << "Error in Database explicit constructor: " << errmsg << std::endl;
        sqlite3_free(errmsg);
    }
}

void Database::init_common(std::string filename)
{
    int res = sqlite3_open(filename.c_str(), &db);
    if (res != SQLITE_OK) {
        sqlite3_close(db);
        throw std::invalid_argument("Couldn't open file");
    }

    std::string sql = "CREATE TABLE IF NOT EXISTS chapters("
                      "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                      "title TEXT NOT NULL,"
                      "body LONGTEXT,"
                      "include BOOL);";
    char* err;
    res = sqlite3_exec(db, sql.c_str(), exec_cb, 0, &err);
    if (res != SQLITE_OK) {
        std::cout << "Sqlite error: " << err << std::endl;
        sqlite3_free(err);
    }

    sql = "CREATE TABLE IF NOT EXISTS metadata("
          "title TEXT NOT NULL,"
          "author TEXT NOT NULL,"
          "target DOUBLE NOT NULL,"
          "last INT,"
          "FOREIGN KEY (last) REFERENCES chapters(id));";
    res = sqlite3_exec(db, sql.c_str(), exec_cb, 0, &err);
    if (res != SQLITE_OK) {
        std::cout << "Sqlite error: " << err << std::endl;
        sqlite3_free(err);
    }
}

int Database::update_metadata(std::string title, std::string author, double target)
{
    std::stringstream sql;
    sql << "UPDATE metadata Set ";
    if (!title.empty()) {
        sql << "title = '" << title << "', ";
    }
    if (!author.empty()) {
        sql << "author = '" << author << "', ";
    }
    if (target > 0) {
        sql << "target = " << target;
    }

    char* errmsg;
    int rc = sqlite3_exec(db, sql.str().c_str(), exec_cb, nullptr, &errmsg);
    if (rc != SQLITE_OK) {
        std::cerr << "Error in update_metadata: " << errmsg << std::endl;
        sqlite3_free(errmsg);
        return 1;
    }
}

int Database::update_title(std::string title)
{
    std::stringstream sql;
    sql << "UPDATE metadata SET title='" << title << "';";
    char* errmsg;
    int rc = sqlite3_exec(db, sql.str().c_str(), exec_cb, nullptr, &errmsg);
    if (rc != SQLITE_OK) {
        std::cerr << "Error in update_title: " << errmsg << std::endl;
        sqlite3_free(errmsg);
    }
}

// Returns newly-created row's id
int Database::insert(std::string title)
{
    sqlite3_stmt* stmt;
    std::stringstream sql;
    sql << "INSERT INTO chapters(title, include) "
        << "VALUES('" << title << "', 1) RETURNING ID;";

    int rc = sqlite3_prepare_v2(db, sql.str().c_str(), -1, &stmt, NULL);
    if (rc != SQLITE_OK)
        throw std::string(sqlite3_errmsg(db));

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
        return id;
    }
}

std::string Database::get_body(int id)
{
    std::stringstream sql;
    sql << "SELECT body FROM chapters WHERE id=" << id;
    std::string response;
    char* errmsg;
    int rc = sqlite3_exec(db, sql.str().c_str(), body_cb, &response, &errmsg);
    if (rc != SQLITE_OK) {
        std::cerr << "Error in get_body: " << errmsg << std::endl;
        sqlite3_free(errmsg);
    }

    return response;
}

std::vector<Chapter> Database::get_chapters()
{
    Chapters chapters;
    char* errmsg;
    int rc = sqlite3_exec(db, "SELECT * FROM chapters", all_chapters_cb, &chapters, &errmsg);
    if (rc != SQLITE_OK) {
        std::cerr << "Error in get_chapters: " << errmsg << std::endl;
        sqlite3_free(errmsg);
    } else {
        std::cerr << chapters.size() << " chapters returned." << std::endl;
    }

    return chapters;
}

double Database::get_target()
{
    double target;
    char *errmsg;
    int rc = sqlite3_exec(db, "SELECT target FROM metadata", target_cb, &target, &errmsg);
    if (rc != SQLITE_OK) {
        std::cerr << "Error in get_target: " << errmsg << std::endl;
        sqlite3_free(errmsg);
    }

    return target;
}

int Database::set_body(int id, std::string body)
{
    std::stringstream sql;
    sql << "UPDATE chapters SET body = '" << body << "' WHERE id = " << id << ";";

    char* errmsg;
    int rc = sqlite3_exec(db, sql.str().c_str(), exec_cb, nullptr, &errmsg);
    if (rc != SQLITE_OK) {
        std::cerr << "Error in set_body: " << errmsg << std::endl;
        sqlite3_free(errmsg);
        return 1;
    }
}

int Database::set_include(int id, bool include)
{
    std::stringstream sql;
    sql << "UPDATE chapters SET include = " << include // (include ? "1" : "0")
        << " WHERE id = " << id << ";";

    char* errmsg;
    int rc = sqlite3_exec(db, sql.str().c_str(), exec_cb, nullptr, &errmsg);
    if (rc != SQLITE_OK) {
        std::cerr << "Error in set_include: " << errmsg << std::endl;
        sqlite3_free(errmsg);
        return 1;
    }
}

int Database::set_title(int id, std::string title)
{
    std::stringstream sql;
    sql << "UPDATE chapters SET title = '" << title << "' WHERE id = " << id << ";";

    char* errmsg;
    int rc = sqlite3_exec(db, sql.str().c_str(), exec_cb, nullptr, &errmsg);
    if (rc != SQLITE_OK) {
        std::cerr << "Error in set_title: " << errmsg << std::endl;
        sqlite3_free(errmsg);
        return 1;
    }
}

int Database::exec_cb(void* unused, int argc, char** argv, char** azColName)
{
    for (int i = 0; i < argc; i++) {
        std::cout << azColName[i] << " : " << (argv[i] ? argv[i] : "NULL") << std::endl;
    }
    return 0;
}

int Database::all_chapters_cb(void* data, int argc, char** argv, char** col_name)
{
    Chapters* chapters = static_cast<Chapters*>(data);
    try {
        Chapter chapter;
        for (int i = 0; i < argc; i++) {
            std::string col(col_name[i]);
            const char* val = argv[i] ? argv[i] : "";
            if (col == "id") {
                chapter.id = atoi(val);
            }
            if (col == "title") {
                chapter.title = std::string(val);
            }
            if (col == "body") {
                chapter.body = std::string(val);
            }
            if (col == "include") {
                chapter.include = atoi(val) == 0 ? false : true;
            }
        }
        chapters->emplace_back(chapter);
    } catch (...) {
        return 1;
    }
    return 0;
}

int Database::body_cb(void* data, int argc, char** argv, char** col_name)
{
    std::string* str = static_cast<std::string*>(data);
    try {
        if (argc != 1)
            return 1;
        if (argv[0])
            *str = std::string(argv[0]);
    } catch (...) {
        return 1;
    }
    return 0;
}

int Database::target_cb(void *data, int argc, char **argv, char **col_name)
{
    double *target = static_cast<double*>(data);
    try {
        if (argc != 1)
            return 1;
        if (argv[0])
            *target = atof(argv[0]);
    } catch (...) {
        return 1;
    }
    return 0;
}
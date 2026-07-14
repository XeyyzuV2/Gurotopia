#pragma once

#include <mysql/mysql.h>
#include <mutex>

extern MYSQL *db;
extern std::mutex db_mutex; // @note serialises DB access across threads (main loop + HTTPS)

extern void mysql_connect();

class hStmt
{
public:
    hStmt(const std::string &query);
   ~hStmt();

    hStmt           (const hStmt&) = delete;
    hStmt& operator=(const hStmt&) = delete;

    std::lock_guard<std::mutex> lock; // @note acquired before any stmt work (declared first so init order matches)
    MYSQL_STMT *pStmt;
};


extern MYSQL_BIND make_bind_in(const signed &buffer);
extern MYSQL_BIND make_bind_in(const unsigned &buffer);
extern MYSQL_BIND make_bind_in(const long &buffer);
extern MYSQL_BIND make_bind_in(const long long &buffer);
extern MYSQL_BIND make_bind_in(const float &buffer);
extern MYSQL_BIND make_bind_in(const std::string &buffer);

extern MYSQL_BIND make_bind_out(signed &buffer);
extern MYSQL_BIND make_bind_out(unsigned &buffer);
extern MYSQL_BIND make_bind_out(long &buffer);
extern MYSQL_BIND make_bind_out(long long &buffer);
extern MYSQL_BIND make_bind_out(float &buffer);
extern MYSQL_BIND make_bind_out(std::string &buffer);

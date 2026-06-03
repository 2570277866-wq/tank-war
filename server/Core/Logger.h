#pragma once

#include <fstream>
#include <string>
#include <mutex>
#include <ctime>
#include <iostream>

class Logger {
public:
    static Logger& Get() {
        static Logger instance;
        return instance;
    }

    void Info(const std::string& msg)  { Log("INFO", msg); }
    void Warn(const std::string& msg)  { Log("WARN", msg); }
    void Error(const std::string& msg) { Log("ERROR", msg); }
    void Game(const std::string& msg)  { Log("GAME", msg); }
    void Cheat(const std::string& msg) { Log("CHEAT", msg); }

private:
    std::mutex mtx;
    std::ofstream file;

    Logger() {
        file.open("server.log", std::ios::app);
    }

    ~Logger() {
        if (file.is_open()) file.close();
    }

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    void Log(const char* level, const std::string& msg) {
        std::lock_guard<std::mutex> lock(mtx);

        time_t now = time(nullptr);
        char timeBuf[32];
        strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", localtime(&now));

        std::string line = std::string("[") + timeBuf + "][" + level + "] " + msg;

        std::cout << line << std::endl;

        if (file.is_open()) {
            file << line << std::endl;
            file.flush();
        }
    }
};

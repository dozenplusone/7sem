#ifndef _HW1_LOGGING_H
#define _HW1_LOGGING_H

#include <filesystem>
#include <fstream>
#include <string>

namespace hw1 {
    namespace fs = std::filesystem;

    class Logger;
} // namespace hw1

class hw1::Logger {
    fs::path root;
public:
    Logger(const fs::path&);
    void operator()(const fs::path&, const std::string&) const;
};

hw1::Logger::Logger(const fs::path &path)
    : root(path)
{
    fs::remove_all(root);
    fs::create_directory(root);
}

void hw1::Logger::operator()(
    const fs::path &path,
    const std::string &text
) const
{
    std::ofstream out(root / path, std::ios::app);
    out << text << std::endl;
    out.close();
}

#endif // _HW1_LOGGING_H

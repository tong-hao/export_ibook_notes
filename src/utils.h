#pragma once
#include <cstdlib> // for getenv
#include <filesystem>
#include <optional>
namespace ibook {

std::optional<std::string> findDatabaseFile(const std::string &directory) {
  // Manually expand the tilde '~' to the user's home directory
  std::string dirPathStr = directory;
  if (dirPathStr[0] == '~') {
    const char *homeDir = std::getenv("HOME");
    if (homeDir) {
      dirPathStr.replace(0, 1, homeDir);
    } else {
      // Return empty optional if the HOME environment variable is not set
      return std::nullopt;
    }
  }

  std::filesystem::path dirPath = std::filesystem::path(dirPathStr)
                                      .lexically_normal()
                                      .make_preferred()
                                      .lexically_normal();
  dirPath = dirPath.empty() ? std::filesystem::current_path() : dirPath;

  for (const auto &entry : std::filesystem::directory_iterator(dirPath)) {
    if (entry.path().extension() == ".sqlite") {
      return entry.path().string();
    }
  }
  return std::nullopt;
}

std::string to_string(const unsigned char *p) {
  return p ? reinterpret_cast<const char *>(p) : "";
}

std::vector<std::string> split(const std::string &str,
                               const std::string &delimiters) {
  std::vector<std::string> result;
  std::size_t pos, lastPos = 0;

  int i = 0;
  while ((pos = str.find_first_of(delimiters, lastPos)) != std::string::npos) {
    i++;
    if (i > 10) {
      break;
    }
    if (pos > lastPos) {
      result.push_back(str.substr(lastPos, pos - lastPos));
    }
    lastPos = pos + 1;
  }

  if (lastPos < str.length()) {
    result.push_back(str.substr(lastPos, std::string::npos));
  }

  return result;
}

std::string now() {
  std::time_t now = std::time(nullptr);
  char date[20];
  std::strftime(date, sizeof(date), "%Y-%m-%d %H:%M", std::localtime(&now));
  return std::string(date);
}

} // namespace ibook

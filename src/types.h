#pragma once

#include "utils.h"
#include <string>

namespace ibook {

struct Note {
  std::string BroaderText;
  std::string SelectedText;
  std::string MarkNote;
  std::string Chapter;
  std::string Created;
  std::string Modified;
  std::string Location;
};

struct Book {
  std::string Title;
  std::string Author;
};

bool compareByLocation(const Note &note1, const Note &note2) {
  auto l1 = split(note1.Location, "/,:");
  auto l2 = split(note2.Location, "/,:");

  size_t len = std::min(l1.size(), l2.size());
  for (size_t i = 0; i < len; i++) {
    try {
      int poc_1 = std::stoi(l1[i]);
      int poc_2 = std::stoi(l2[i]);
      if (poc_1 != poc_2) {
        return poc_1 < poc_2;
      }
    } catch (const std::exception &e) {
    }
  }

  return l1.size() < l2.size();
}

} // namespace ibook

#include <algorithm>
#include <array>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <sqlite3.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

#include "types.h"
#include "utils.h"

namespace ibook {
// dir
std::string dir_output = "./ibook_notes";
std::string dir_ibook_db =
    "~/Library/Containers/com.apple.iBooksX/Data/Documents/BKLibrary";
std::string dir_notes_db =
    "~/Library/Containers/com.apple.iBooksX/Data/Documents/AEAnnotation";

// sql
std::string sql_list_book = R"(
SELECT ZASSETID, ZTITLE AS Title, ZAUTHOR AS Author 
FROM ZBKLIBRARYASSET
WHERE ZTITLE IS NOT NULL
)";

std::string sql_export_node = R"(
SELECT 
ZANNOTATIONREPRESENTATIVETEXT as BroaderText,
ZANNOTATIONSELECTEDTEXT as SelectedText,
ZANNOTATIONNOTE as Note,
ZFUTUREPROOFING5 as Chapter,
ZANNOTATIONCREATIONDATE as Created,
ZANNOTATIONMODIFICATIONDATE as Modified,
ZANNOTATIONASSETID,
ZANNOTATIONLOCATION as LOCATION 
FROM ZAEANNOTATION 
WHERE ZANNOTATIONSELECTEDTEXT IS NOT NULL and ZANNOTATIONDELETED=0
)";

class DBConnection {
private:
  sqlite3 *db;

public:
  DBConnection(const std::string &fileName) {
    int rc = sqlite3_open_v2(fileName.c_str(), &db,
                             SQLITE_OPEN_READONLY | SQLITE_OPEN_WAL, nullptr);
    if (rc) {
      std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
      sqlite3_close(db);
      exit(1);
    }
  }

  ~DBConnection() { sqlite3_close(db); }

  sqlite3 *getDB() { return db; }
};

std::map<std::string, Book> listBooks() {
  std::map<std::string, Book> books;
  auto fBookDb = findDatabaseFile(dir_ibook_db.c_str());
  if (!fBookDb) {
    std::cout << "Cannot get database file for ibook in " << dir_ibook_db
              << std::endl;
    return books;
  }

  std::cout << "booksDatabaseFile:" << fBookDb.value_or("") << std::endl;
  DBConnection booksDB(fBookDb.value_or(""));
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(booksDB.getDB(), sql_list_book.c_str(), -1, &stmt,
                              nullptr);
  if (rc != SQLITE_OK) {
    std::cerr << "SQL error: " << sqlite3_errmsg(booksDB.getDB()) << std::endl;
    exit(1);
  }

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    auto assetID = to_string(sqlite3_column_text(stmt, 0));
    auto title = to_string(sqlite3_column_text(stmt, 1));
    auto author = to_string(sqlite3_column_text(stmt, 2));

    books.emplace(assetID, Book{title, author});
  }

  sqlite3_finalize(stmt);
  return books;
}

std::map<std::string, std::vector<Note>> listNotes() {
  std::map<std::string, std::vector<Note>> notes;
  auto fNotes = findDatabaseFile(dir_notes_db.c_str());
  if (!fNotes) {
    return notes;
  }

  std::cout << "notesDatabaseFile:" << fNotes.value_or("") << std::endl;
  DBConnection notesDB(fNotes.value_or(""));
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(notesDB.getDB(), sql_export_node.c_str(), -1,
                              &stmt, nullptr);
  if (rc != SQLITE_OK) {
    std::cerr << "SQL error: " << sqlite3_errmsg(notesDB.getDB()) << std::endl;
    exit(1);
  }

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    auto broaderText = to_string(sqlite3_column_text(stmt, 0));
    auto selectedText = to_string(sqlite3_column_text(stmt, 1));
    auto markNote = to_string(sqlite3_column_text(stmt, 2));
    auto chapter = to_string(sqlite3_column_text(stmt, 3));
    auto created = to_string(sqlite3_column_text(stmt, 4));
    auto modified = to_string(sqlite3_column_text(stmt, 5));
    auto assetID = to_string(sqlite3_column_text(stmt, 6));
    auto location = to_string(sqlite3_column_text(stmt, 7));

    Note n;
    n.BroaderText = broaderText;
    n.SelectedText = selectedText;
    n.MarkNote = markNote;
    n.Chapter = chapter;
    n.Created = created;
    n.Modified = modified;
    n.Location = location;

    notes[assetID].push_back(n);
  }

  // 排序笔记
  for (auto &[assetID, noteVec] : notes) {
    std::cout << assetID << ", count:" << noteVec.size() << std::endl;
    std::sort(noteVec.begin(), noteVec.end(), compareByLocation);
  }

  sqlite3_finalize(stmt);
  return notes;
}

void exportNotes(const std::map<std::string, Book> &books,
                 const std::map<std::string, std::vector<Note>> &notes) {
  mkdir(dir_output.c_str(), 0777);
  chdir(dir_output.c_str());

  int i = 0;
  int j = 0;
  int b = 0;

  for (const auto &[assetID, bookNotes] : notes) {
    std::string bookTitle;
    std::string body;

    if (books.count(assetID) > 0) {
      bookTitle = books.at(assetID).Title;
      body += "# " + bookTitle + "\n";
      body += "- Author: " + books.at(assetID).Author + "\n";
      body += "- assetID: " + assetID + "\n";
    } else {
      bookTitle = assetID;
      body += "- assetID: " + assetID + "\n";
    }

    body += "- Export date: " + now() + "\n";

    j = 0;
    std::string curChapter = "";
    for (const auto &noteData : bookNotes) {
      auto broaderText = noteData.BroaderText;
      auto selectedText = noteData.SelectedText;
      auto markNote = noteData.MarkNote;
      auto chapter = noteData.Chapter;

      if (broaderText.empty() && selectedText.empty()) {
        continue;
      }

      std::string highlightedText = selectedText;

      if (j == 0) {
        b++;
      }
      i++;
      j++;

      if (!chapter.empty() && curChapter != chapter) {
        body += "\n---\n";
        body += "### Chapter: " + chapter + "\n";
        curChapter = chapter;
      }

      if (highlightedText == curChapter) {
        continue;
      }

      body += highlightedText + "\n";
      if (!markNote.empty()) {
        body += "> Note: " + markNote + "\n";
      }

      body += "\n";
    }

    std::ofstream file(bookTitle + ".md");
    file << body;
    file.close();
  }

  std::cout << "导出完成： " << dir_output << std::endl;
}

} // namespace ibook

int main() {
  auto books = ibook::listBooks();
  std::cout << "发现图书数量:" << books.size() << std::endl;

  auto notes = ibook::listNotes();
  std::cout << "笔记对应的图书数量:" << notes.size() << std::endl;

  ibook::exportNotes(books, notes);
  return 0;
}

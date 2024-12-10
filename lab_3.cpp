#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include <ctime>
#include <map>
#include <algorithm>

namespace fs = std::filesystem;

class File {
protected:
    std::string filename;
    std::string extension;
    std::time_t creationTime;
    std::time_t lastUpdateTime;

public:
    File(const std::string& path) {
        filename = fs::path(path).filename().string();
        extension = fs::path(path).extension().string();
        creationTime = fs::last_write_time(path).time_since_epoch().count();
        lastUpdateTime = fs::last_write_time(path).time_since_epoch().count();
    }

    virtual void displayInfo() const {
        std::cout << "Filename: " << filename << "\n"
                  << "Extension: " << extension << "\n"
                  << "Creation Time: " << creationTime << "\n"
                  << "Last Updated: " << lastUpdateTime << "\n";
    }

    std::string getFilename() const { return filename; }

    virtual bool isChanged(const std::time_t& lastSnapshotTime) const {
        return lastUpdateTime > lastSnapshotTime;
    }

    virtual ~File() {}
};

class ImageFile : public File {
    // No image parsing logic included as no third-party libraries are allowed.
public:
    ImageFile(const std::string& path) : File(path) {}

    void displayInfo() const override {
        File::displayInfo();
        std::cout << "Type: Image File\n";
    }
};

class TextFile : public File {
    size_t lineCount, wordCount, charCount;

    void analyzeFile(const std::string& path) {
        std::ifstream file(path);
        std::string line;
        lineCount = wordCount = charCount = 0;

        while (std::getline(file, line)) {
            lineCount++;
            wordCount += std::count_if(line.begin(), line.end(), [](char c) { return c == ' '; }) + 1;
            charCount += line.length();
        }
        file.close();
    }

public:
    TextFile(const std::string& path) : File(path) {
        analyzeFile(path);
    }

    void displayInfo() const override {
        File::displayInfo();
        std::cout << "Type: Text File\n"
                  << "Lines: " << lineCount << "\n"
                  << "Words: " << wordCount << "\n"
                  << "Characters: " << charCount << "\n";
    }
};

class ProgramFile : public File {
    size_t lineCount, classCount, methodCount;

    void analyzeFile(const std::string& path) {
        std::ifstream file(path);
        std::string line;
        lineCount = classCount = methodCount = 0;

        while (std::getline(file, line)) {
            lineCount++;
            if (line.find("class ") != std::string::npos) classCount++;
            if (line.find("void ") != std::string::npos || line.find("(") != std::string::npos) methodCount++;
        }
        file.close();
    }

public:
    ProgramFile(const std::string& path) : File(path) {
        analyzeFile(path);
    }

    void displayInfo() const override {
        File::displayInfo();
        std::cout << "Type: Program File\n"
                  << "Lines: " << lineCount << "\n"
                  << "Classes: " << classCount << "\n"
                  << "Methods: " << methodCount << "\n";
    }
};

class FolderMonitor {
    std::string folderPath;
    std::time_t lastSnapshotTime;
    std::map<std::string, File*> files;

    void detectChanges() {
        std::vector<std::string> currentFiles;

        for (const auto& entry : fs::directory_iterator(folderPath)) {
            currentFiles.push_back(entry.path().string());
        }

        // Check for deleted files
        for (auto it = files.begin(); it != files.end();) {
            if (std::find(currentFiles.begin(), currentFiles.end(), it->first) == currentFiles.end()) {
                std::cout << it->second->getFilename() << " was deleted.\n";
                delete it->second;
                it = files.erase(it);
            } else {
                ++it;
            }
        }

        // Check for new and modified files
        for (const auto& file : currentFiles) {
            if (files.find(file) == files.end()) {
                addFile(file);
                std::cout << fs::path(file).filename().string() << " is a new file.\n";
            } else if (files[file]->isChanged(lastSnapshotTime)) {
                std::cout << fs::path(file).filename().string() << " has changed.\n";
            }
        }
    }

    void addFile(const std::string& path) {
        std::string ext = fs::path(path).extension().string();
        if (ext == ".txt")
            files[path] = new TextFile(path);
        else if (ext == ".png" || ext == ".jpg")
            files[path] = new ImageFile(path);
        else if (ext == ".cpp" || ext == ".java")
            files[path] = new ProgramFile(path);
        else
            files[path] = new File(path);
    }

public:
    FolderMonitor(const std::string& path) : folderPath(path), lastSnapshotTime(std::time(0)) {
        for (const auto& entry : fs::directory_iterator(folderPath)) {
            addFile(entry.path().string());
        }
    }

    void commit() {
        lastSnapshotTime = std::time(0);
        std::cout << "Snapshot updated.\n";
    }

    void status() {
        detectChanges();
    }

    void info(const std::string& filename) {
        for (const auto& [path, file] : files) {
            if (file->getFilename() == filename) {
                file->displayInfo();
                return;
            }
        }
        std::cout << "File not found: " << filename << "\n";
    }

    ~FolderMonitor() {
        for (auto& [_, file] : files) {
            delete file;
        }
    }
};

int main() {
    FolderMonitor monitor("./test_folder");

    std::string command, filename;
    while (true) {
        std::cout << "Enter command (commit, status, info [filename], exit): ";
        std::cin >> command;

        if (command == "commit") {
            monitor.commit();
        } else if (command == "status") {
            monitor.status();
        } else if (command == "info") {
            std::cin >> filename;
            monitor.info(filename);
        } else if (command == "exit") {
            break;
        } else {
            std::cout << "Unknown command.\n";
        }
    }

    return 0;
}


#include <iostream>
#include <filesystem>
#include <cstdint>
#include <iomanip>
#include <string>
using namespace std;

namespace fs = std::filesystem;

int main()
{
    cout << "FolderFlow Guardian" << endl;
    cout << "SPL-1 Project" << endl;

    string folderPath;

    cout << "\nEnter folder path: ";
    getline(cin, folderPath);

    if (folderPath.empty())
        folderPath = ".";

    try
    {
        if (!fs::exists(folderPath) || !fs::is_directory(folderPath))
        {
            cout << "Invalid folder path!" << endl;
            return 1;
        }

        int files = 0;
        int folders = 0;
        std::uintmax_t totalBytes = 0;
        std::uintmax_t sizeErrors = 0;

       
    for (fs::recursive_directory_iterator it(folderPath, fs::directory_options::skip_permission_denied), end;
     it != end; ++it)
    {
        const auto& entry = *it;

        if (entry.is_directory() && entry.path().filename() == ".git")
        {
        it.disable_recursion_pending();
        continue;
        }

            if (entry.is_directory())
            {
                cout << "[FOLDER] ";
                folders++;
            }
            else
            {
                cout << "[FILE] ";
                files++;
            }

cout << entry.path().lexically_relative(folderPath).string();

    if (entry.is_regular_file())
    {
    std::error_code ec;
    auto fileBytes = entry.file_size(ec);

    if (!ec)
{
    totalBytes += fileBytes;
    cout << " (" << fileBytes << " bytes)";
}
else
{
    sizeErrors++;
    cout << " [size unavailable: " << ec.message() << "]";
}
    }

cout << endl;

        }
        cout << "\nTotal Files: " << files << endl;
        cout << "Total Folders: " << folders << endl;
        cout << "Total Size: " << totalBytes << " bytes" << endl;

        cout << fixed << setprecision(2);
        cout << "Total Size (KB): " << totalBytes / 1000.0 << endl;
        cout << "Total Size (MB): " << totalBytes / 1000000.0 << endl;
        if (sizeErrors > 0)
{
    cout << "Warning: could not measure " << sizeErrors
         << " file(s). Total size includes only successfully measured files."
         << endl;
}
    }
    catch (const fs::filesystem_error& e)
    {
        cerr << "Scanning error: " << e.what() << endl;
        return 1;
    }

    return 0;
}

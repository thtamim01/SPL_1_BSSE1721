
#include <iostream>
#include <filesystem>
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

        for (const auto& entry : fs::directory_iterator(folderPath))
        {
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

            cout << entry.path().filename().string() << endl;
        }

        cout << "\nTotal Files: " << files << endl;
        cout << "Total Folders: " << folders << endl;
    }
    catch (const fs::filesystem_error& e)
    {
        cerr << "Scanning error: " << e.what() << endl;
        return 1;
    }

    return 0;
}

#include <windows.h> //Windows API imports
#include <iostream> //IO steams for gettung input and printing to stdout
#include <string> // used to convert bufers to strings and concat
#include <vector> //store args
#include <iomanip> //aligns columns of output
#include <Aclapi.h> //used to get file owner infor
/*
Cameron Natoli
Dfor 740 Midterm
*/
using namespace std;

//converts file time numbrs to a human readable sttring for ease of use
//FILETIME# is a timestamp of UTC nanoseconds since Jan 1st 1601, ft passes the value by refernce instead of copying the value 
string FileTimeToString(const FILETIME& ft) {
    //SYSTEMTIME stores tiem in human readblae formating , stores  both UTC and converted to Local
    SYSTEMTIME stUTC, stLocal;
    //convert from FILETIME in nanoseconds to UTC
    FileTimeToSystemTime(&ft, &stUTC);
    //convert to local from UTC
    SystemTimeToTzSpecificLocalTime(NULL, &stUTC, &stLocal);

    char buffer[100];
    //frmating
    sprintf_s(buffer, "%02d/%02d/%04d %02d:%02d:%02d",
        stLocal.wMonth, stLocal.wDay, stLocal.wYear,
        stLocal.wHour, stLocal.wMinute, stLocal.wSecond);

    return string(buffer);
}

// Gets file owner
//input of string as the file name
string GetFileOwner(const string& filename) {
    //variables for Security ID and the desrciptor
    PSID ownerSid = NULL;
    PSECURITY_DESCRIPTOR sd = NULL;
    //retrives info about a file
    if (GetNamedSecurityInfoA(
        filename.c_str(),
        SE_FILE_OBJECT,
        OWNER_SECURITY_INFORMATION,
        &ownerSid,
        NULL,
        NULL,
        NULL,
        &sd) != ERROR_SUCCESS) {
        return "N/A";
    }
    //variables to hold vaules
    char name[256], domain[256];
    DWORD nameSize = 256, domainSize = 256;
    SID_NAME_USE sidType;
    //converts SID to humanreadable format
    if (LookupAccountSidA(
        NULL,
        ownerSid,
        name,
        &nameSize,
        domain,
        &domainSize,
        &sidType)) {
        return string(domain) + "\\" + string(name);
    }
//failsafe if errors
    return "Unknown";
}

// Print file info
//WIN32_FIND_DATAA holds info about files and drectorys
void PrintFileData(const WIN32_FIND_DATAA& data, const string& fullPath, bool showOwner) {
    //gets flags to determine if file is a dir or file
    string type = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? "<DIR>" : "FILE";
    // gets file size
    LARGE_INTEGER size;
    size.HighPart = data.nFileSizeHigh;
    size.LowPart = data.nFileSizeLow;
    //print out of type size and Cration
    cout << setw(6) << type << "  ";
    cout << setw(12) << size.QuadPart << "  ";
    cout << FileTimeToString(data.ftCreationTime) << "  ";
    //if flag is set show owner
    if (showOwner) {
        cout << setw(25) << GetFileOwner(fullPath) << "  ";
    }
    //stdout of filename 
    cout << data.cFileName << endl;
}

// Recursive directory listing
//uses path and the 3 flags to determine functionality
void ListDirectory(const string& path, bool showHidden, bool recursive, bool showOwner) {
    //appends \\* to end of path input so it searchs all entires in the folder
    string searchPath = path + "\\*";
    //stores data about files and folders
    WIN32_FIND_DATAA findData;
    //starts search and attaches a handle 
    HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);
    //error handling if path doesnt exist or is inaccessiable
    if (hFind == INVALID_HANDLE_VALUE) {
        cout << "Cannot open directory: " << path << endl;
        return;
    }
    //loop throgu all files in the dir
    do {
        string name = findData.cFileName;

        if (name == "." || name == "..")
            continue;
        //if /a is used shows hidden files, and normal files aswell
        bool isHidden = (findData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN);
        if (!showHidden && isHidden)
            continue;

        string fullPath = path + "\\" + name;

        PrintFileData(findData, fullPath, showOwner);

        // Recurse into subdirectories if /s is used
        if (recursive && (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            ListDirectory(fullPath, showHidden, recursive, showOwner);
        }
        //goes to next file in the dir
    } while (FindNextFileA(hFind, &findData));
    //close the handle
    FindClose(hFind);
}

// CD 
//path holds input of destiation
void ChangeDirectory(const string& path) {
    //API to set the CWD to the path c_str converts  string to form useable by API
    if (SetCurrentDirectoryA(path.c_str())) {
        //gets pwd and prints it to stdout
        char buffer[MAX_PATH];
        GetCurrentDirectoryA(MAX_PATH, buffer);
        cout << "Current Directory: " << buffer << endl;
    }
    //if fails error handle
    else {
        cout << "Failed to change directory." << endl;
    }
}




// Main driver
int main() {
    //loop sentiel
    bool exitProgram = false;

    //program loop 
    while (!exitProgram) {
        // Show current directory prompt
        char buffer[MAX_PATH];
        GetCurrentDirectoryA(MAX_PATH, buffer);
        cout << buffer << "> ";  // like a shell prompt

        // get user input
        string input;
        getline(cin, input);

        if (input.empty()) continue;  // ignore empty commands

        // Split input into arguments
        vector<string> args;
        size_t pos = 0;
        while ((pos = input.find(' ')) != string::npos) {
            args.push_back(input.substr(0, pos));
            input.erase(0, pos + 1);
        }
        args.push_back(input);  // last word

        // exit commands
        if (args[0] == "exit" || args[0] == "0" || args[0] == "stop" || args[0] == "done" || args[0] == "out") {
            exitProgram = true;
            continue;
        }
        //handles the pwd componet
        if (input == "pwd") {
            char buffer[MAX_PATH];
            //gets the CD and prints to stdout
            if (GetCurrentDirectoryA(MAX_PATH, buffer)) {
                cout << buffer << endl;
            }
            //error handler
            else {
                cout << "Error retrieving current directory.\n";
            }
            continue;
        }
        // Reset flags
        bool showHidden = false, recursive = false, showOwner = false;
        string targetPath = "";
        bool pathProvided = false;

        // Parse arguments
        for (size_t i = 0; i < args.size(); i++) {
            string arg = args[i];

            if (!arg.empty() && arg[0] == '/') {
                if (arg == "/a") showHidden = true;
                else if (arg == "/s") recursive = true;
                else if (arg == "/q") showOwner = true;
            }
            else {
                targetPath = arg;
                pathProvided = true;
            }
        }

        // If path provided → change directory
        if (pathProvided) {
            if (!SetCurrentDirectoryA(targetPath.c_str())) {
                cout << "Failed to change directory." << endl;
            }
        }
        else {
            // List current directory
            ListDirectory(".", showHidden, recursive, showOwner);
        }

        cout << endl;  // separate commands
    }

    cout << "Exiting program..." << endl;
    return 0;
}
#include "windows.h"
#include <string>
#include <iostream>
#include <iomanip>

using namespace std;

#pragma pack(push, 1)

typedef struct
{
    uint8_t jump[3];
    uint8_t name[8];
    int16_t bytesPerSector;
    uint8_t sectorPerCluster;
    uint8_t reserved1[7];
    uint8_t mediaDesc;
    int16_t reserved2;
    int16_t sectorPerTrack;
    int16_t numHeads;
    uint8_t reserved3[12];
    int64_t numSectors;
    int64_t mftCluster;
    int64_t mftMirrorCluster;
    int32_t mftSize;
    uint8_t clustersPerIndex;
    uint8_t reserved4[3];
    int32_t checksum;
    uint8_t buf[428];
} NtfsBootSector;

#pragma pack(pop)

int64_t SetFileSeek(void* h, int64_t distance, int16_t seekMethod);
void PrintHex(const uint8_t* data, int size);
int ReadCluster(void* hDisk, uint8_t* destination, uint64_t clusterNumber, uint16_t clusterSize);

int main()
{
    int result;
    wstring volumeName;
    cout << "Enter volume name: ";
    wcin >> volumeName;
    wstring filename = L"\\\\.\\" + volumeName + L":";
    HANDLE hDisk = CreateFileW(
        filename.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );
    if (hDisk == INVALID_HANDLE_VALUE)
    {
        wcerr << L"Error opening file " << filename.c_str();
        exit(1);
    }
    WCHAR fstype[MAX_PATH + 1];
    result = GetVolumeInformationByHandleW(hDisk, nullptr, 0, 0, 0, 0, fstype, MAX_PATH + 1);
    if (result == 0)
    {
        cerr << "Error in GetVolumeInformationByHandleW\n";
        CloseHandle(hDisk);
        exit(EXIT_FAILURE);
    }
    if (wmemcmp(fstype, L"NTFS", 4) != 0)
    {
        cerr << "NTFS not found\n";
        CloseHandle(hDisk);
        exit(EXIT_FAILURE);
    }
    NtfsBootSector ntfsBoot;
    DWORD bytesToRead = 512;
    DWORD bytesRead;
    int64_t position = SetFileSeek(hDisk, 0, FILE_BEGIN);

    if (position == -1)
    {
        cerr << "Error SetFileSeek\n";
        CloseHandle(hDisk);
        exit(EXIT_FAILURE);
    }
    BOOL readResult = ReadFile(hDisk, &ntfsBoot, bytesToRead, &bytesRead, nullptr);
    if (!readResult || bytesRead != bytesToRead)
    {
        cerr << "Error read file";
        CloseHandle(hDisk);
        exit(EXIT_FAILURE);
    }
    uint16_t clusterSize = ntfsBoot.bytesPerSector * ntfsBoot.sectorPerCluster;
    uint64_t numClusters = ntfsBoot.numSectors / ntfsBoot.sectorPerCluster;
    cout << "NTFS Boot Sector in hex:\n\t____________________________________\n";
    PrintHex(reinterpret_cast<const BYTE*>(&ntfsBoot), 512);
    cout << "\t------------------------------------\n";
    cout << "NTFS Info:" << endl;
    cout << "Sector size: " << ntfsBoot.bytesPerSector << " bytes" << endl;
    cout << "Total Sectors count: " << ntfsBoot.numSectors << endl;
    cout << "Cluster size: " << clusterSize << " bytes" << endl;
    cout << "Total Clusters count: " << numClusters << endl;
    cout << "Logical Cluster Number for the file $MFT: " << ntfsBoot.mftCluster << endl;
    cout << "Logical Cluster Number for the file $MFTMirror: " << ntfsBoot.bytesPerSector * ntfsBoot.mftMirrorCluster << endl;
    cout << "MFT size: " << ntfsBoot.mftSize << " clusters" << endl;

    uint64_t lcn; // Logical Cluster Number
    cout << "Enter Logical Cluster Number: ";
    cin >> lcn;
    if (cin.fail())
    {
        cerr << "INT is needed. Exiting." << endl;
        exit(EXIT_FAILURE);
    }
    if (lcn >= numClusters)
    {
        cerr << "No such LCN";
        CloseHandle(hDisk);
        exit(1);
    }
    BOOL needPrintCluster = FALSE;
    int temp;
    cout << "Print cluster[1 - yes, 0 - no]: ";
    cin >> temp;
    needPrintCluster = (temp == 0) ? FALSE : TRUE;
    BYTE* buffer = new BYTE[clusterSize];
    result = ReadCluster(hDisk, buffer, lcn, clusterSize);
    if (result != 0)
    {
        cerr << "Error reading cluster";
        CloseHandle(hDisk);
        delete[] buffer;
        exit(1);
    }
    if (needPrintCluster)
    {
        PrintHex(buffer, clusterSize);
    }
    delete[] buffer;
    CloseHandle(hDisk);
    system("pause");
    return 0;
}
int64_t SetFileSeek(HANDLE h, int64_t distance, int16_t seekMethod)
{
    LARGE_INTEGER li;
    li.QuadPart = distance;

    if (!SetFilePointerEx(h, li, &li, seekMethod))
    {
        li.QuadPart = -1;
    }

    return li.QuadPart;
}
void PrintHex(const BYTE* data, int size)
{
    ios init(nullptr);
    init.copyfmt(cout);
    int pos = 0;
    int offset = 0;
    while (pos < size)
    {
        cout << dec << setw(8) << offset << "|";
        for (int j = 0; j < 16; ++j)
        {
            cout << hex << setw(2) << setfill('0') << uppercase << static_cast<int>(data[pos]) << ' ';
            ++pos;
        }
        cout << endl;
        offset += 16;
    }
    cout.copyfmt(init);
}
int ReadCluster(HANDLE h, BYTE* destination, uint64_t clusterNumber, uint16_t clusterSize)
{
    int64_t position = SetFileSeek(h, clusterNumber * clusterSize, FILE_BEGIN);
    if (position == -1)
    {
        cerr << "[ReadCluster] error SetFileSeek" << endl;
        return -1;
    }
    DWORD bytesToRead = clusterSize;
    DWORD bytesRead;
    BOOL readResult = ReadFile(
        h,
        destination,
        bytesToRead,
        &bytesRead,
        nullptr);
    if (!readResult || bytesRead != bytesToRead)
    {
        cerr << "[ReadCluster] error read file" << endl;
        return -1;
    }
    return 0;
}
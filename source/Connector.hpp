#ifndef CONNECTOR_H
#define CONNECTOR_H

#include <windows.h>
#include <cstdio>
#include <iostream>
#include <string>

// Các biến toàn cục cho kết nối với engine
STARTUPINFOA sti = { 0 };
SECURITY_ATTRIBUTES sats = { 0 };
PROCESS_INFORMATION pi = { 0 };
HANDLE pipin_w, pipin_r, pipout_w, pipout_r;
BYTE buffer[2048];
DWORD writ, excode, readBytes, available;

// Kết nối đến engine (ví dụ: Stockfish)
// Sử dụng CreateProcessA vì chúng ta truyền chuỗi char*
void ConnectToEngine(char* path)
{
    pipin_w = pipin_r = pipout_w = pipout_r = NULL;
    sats.nLength = sizeof(sats);
    sats.bInheritHandle = TRUE;
    sats.lpSecurityDescriptor = NULL;

    CreatePipe(&pipout_r, &pipout_w, &sats, 0);
    CreatePipe(&pipin_r, &pipin_w, &sats, 0);
         
    sti.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
    sti.wShowWindow = SW_HIDE;
    sti.hStdInput = pipin_r;
    sti.hStdOutput = pipout_w;
    sti.hStdError = pipout_w;

    CreateProcessA(NULL, path, NULL, NULL, TRUE, 0, NULL, NULL, &sti, &pi);
}

// Lấy nước đi tiếp theo từ engine dựa trên chuỗi position hiện tại
std::string getNextMove(std::string positionStr)
{     
    std::string outStr;
    std::string cmd = "position startpos moves " + positionStr + "\ngo\n";    

    WriteFile(pipin_w, cmd.c_str(), static_cast<DWORD>(cmd.length()), &writ, NULL);
    Sleep(500);
        
    PeekNamedPipe(pipout_r, buffer, sizeof(buffer), &readBytes, &available, NULL);   
    do
    {   
        ZeroMemory(buffer, sizeof(buffer));
        if (!ReadFile(pipout_r, buffer, sizeof(buffer), &readBytes, NULL) || readBytes == 0)
            break; 
        buffer[readBytes] = 0;    
        outStr += reinterpret_cast<char*>(buffer);
    }
    while (readBytes >= sizeof(buffer));

    size_t n = outStr.find("bestmove");
    if (n != std::string::npos)
        return outStr.substr(n + 9, 4);
             
    return "error";
}

// Đóng kết nối đến engine
void CloseConnection()
{
    WriteFile(pipin_w, "quit\n", 5, &writ, NULL);
    if (pipin_w != NULL) CloseHandle(pipin_w);
    if (pipin_r != NULL) CloseHandle(pipin_r);
    if (pipout_w != NULL) CloseHandle(pipout_w);
    if (pipout_r != NULL) CloseHandle(pipout_r);
    if (pi.hProcess != NULL) CloseHandle(pi.hProcess);
    if (pi.hThread != NULL) CloseHandle(pi.hThread);
}

#endif // CONNECTOR_H

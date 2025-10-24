#pragma once
#include <Windows.h>
#include <string>
#include <vector>
#include <memory>

#include "Vector.h"

namespace Utils
{
    class SafeMemory
    {
    public:
        SafeMemory();
        ~SafeMemory();
        
        bool AttachToProcess(const std::string& processName);
        void DetachFromProcess();
        
        template<typename T>
        bool Read(uintptr_t address, T& value) const
        {
            if (!m_isAttached || !address)
                return false;
                
            SIZE_T bytesRead;
            return ReadProcessMemory(m_processHandle, reinterpret_cast<LPCVOID>(address), &value, sizeof(T), &bytesRead) && bytesRead == sizeof(T);
        }
        
        template<typename T>
        bool Write(uintptr_t address, const T& value) const
        {
            if (!m_isAttached || !address)
                return false;
                
            SIZE_T bytesWritten;
            return WriteProcessMemory(m_processHandle, reinterpret_cast<LPVOID>(address), &value, sizeof(T), &bytesWritten) && bytesWritten == sizeof(T);
        }
        
        bool ReadBytes(uintptr_t address, void* buffer, size_t size);
        bool WriteBytes(uintptr_t address, const void* buffer, size_t size);
        
        uintptr_t GetBaseAddress() const { return m_baseAddress; }
        DWORD GetProcessId() const { return m_processId; }
        bool IsAttached() const { return m_isAttached; }
        
        // Pattern scanning
        uintptr_t PatternScan(const std::string& pattern, const std::string& mask);
        uintptr_t PatternScan(uintptr_t startAddress, size_t size, const std::string& pattern, const std::string& mask);
        
        // Module functions
        uintptr_t GetModuleBase(const std::string& moduleName);
        std::vector<uintptr_t> GetModuleBases();
        
        // Specialized read/write functions
        std::string ReadString(uintptr_t address, size_t maxLength = 256);
        Utils::Vector3 ReadVector3(uintptr_t address);
        void WriteVector3(uintptr_t address, const Utils::Vector3& vector);
        
        // Pointer operations
        uintptr_t ResolvePointerChain(uintptr_t baseAddress, const std::vector<uintptr_t>& offsets);
        
        // Validation functions
        bool IsValidAddress(uintptr_t address);
        bool IsValidPointer(uintptr_t address);
        bool ValidateAddress(uintptr_t address);
        
        // Memory patching
        bool PatchMemory(uintptr_t address, const std::vector<uint8_t>& newBytes);
        
        // Hash functions
        uint32_t CalculateBulletHash(const Utils::Vector3& position, const Utils::Vector3& direction);
        uint32_t CalculateBulletHashForPosition(const Utils::Vector3& position);
        // Internal hash calculation function
        uint32_t CalculateBulletHashInternal(const void* positionData, size_t size);
    private:
        
        // Bone operations
        Utils::Vector3 ReadSimpleBoneData(uintptr_t boneAddress);
        
    private:
        HANDLE m_processHandle;
        uintptr_t m_baseAddress;
        DWORD m_processId;
        bool m_isAttached;
        
        std::string PatternToBytes(const std::string& pattern);
    };
    
    // Hash calculation functions
    uint32_t CalculatePositionHash(const Utils::Vector3& position);
    uint32_t CalculateEntityHash(uintptr_t entityAddress, const Utils::Vector3& position);
}

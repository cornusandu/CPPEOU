#include "autoloaddll.hpp"

#include <unordered_map>
#include <iostream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <cstring>
#include <cstdint>

#if defined(_WIN32) || defined(WIN32)
    #pragma message("Compiling for Windows")
    #define isWINDOWS 1
#else
    #define isWINDOWS 0
#endif

#if defined(__posix__) || defined(__linux__) || defined(__unix__) || defined(linux)
    #define isLINUX 1
#else
    #define isLINUX 0
#endif

using DummyFuncPtr = int(*)(...);

#if isWINDOWS
#include <windows.h>
std::unordered_map<std::string, uintptr_t> GetExportedFunctions(const std::string& dllPath) {
    std::unordered_map<std::string, uintptr_t> exports;

    HMODULE hModule = LoadLibraryA(dllPath.c_str());
    if (!hModule) {
        std::cerr << "Failed to load DLL: " << dllPath << "\n";
        return exports;
    }

    auto base = reinterpret_cast<uint8_t*>(hModule);
    auto dosHeader = reinterpret_cast<IMAGE_DOS_HEADER*>(base);
    auto ntHeaders = reinterpret_cast<IMAGE_NT_HEADERS*>(base + dosHeader->e_lfanew);

    auto exportDirRVA = ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
    if (!exportDirRVA) return exports;

    auto exportDir = reinterpret_cast<IMAGE_EXPORT_DIRECTORY*>(base + exportDirRVA);
    auto namesRVA = reinterpret_cast<DWORD*>(base + exportDir->AddressOfNames);
    auto funcsRVA = reinterpret_cast<DWORD*>(base + exportDir->AddressOfFunctions);
    auto ordinalsRVA = reinterpret_cast<WORD*>(base + exportDir->AddressOfNameOrdinals);

    for (DWORD i = 0; i < exportDir->NumberOfNames; i++) {
        std::string funcName = reinterpret_cast<const char*>(base + namesRVA[i]);
        WORD ordinal = ordinalsRVA[i];
        DWORD funcRVA = funcsRVA[ordinal];
        uintptr_t funcAddr = reinterpret_cast<uintptr_t>(base + funcRVA);

        exports[funcName] = funcAddr;
    }

    return exports;
}
#endif

#if isLINUX
#include <dlfcn.h>
#include <link.h>
#include <elf.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

std::unordered_map<std::string, uintptr_t> GetExportedFunctions(const std::string& soPath) {
    std::unordered_map<std::string, uintptr_t> exports;

    // 1. Load library into memory
    void* handle = dlopen(soPath.c_str(), RTLD_LAZY);
    if (!handle) {
        std::cerr << "dlopen failed: " << dlerror() << "\n";
        return exports;
    }

    // 2. Get base address using dlinfo
    struct link_map* map;
    if (dlinfo(handle, RTLD_DI_LINKMAP, &map) != 0) {
        std::cerr << "dlinfo failed\n";
        dlclose(handle);
        return exports;
    }
    uintptr_t baseAddr = (uintptr_t)map->l_addr;

    // 3. Open file for ELF parsing
    int fd = open(soPath.c_str(), O_RDONLY);
    if (fd < 0) {
        std::perror("open");
        dlclose(handle);
        return exports;
    }

    struct stat st;
    fstat(fd, &st);
    void* mapped = mmap(nullptr, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mapped == MAP_FAILED) {
        std::perror("mmap");
        close(fd);
        dlclose(handle);
        return exports;
    }

    close(fd);
    uint8_t* base = reinterpret_cast<uint8_t*>(mapped);
    auto ehdr = reinterpret_cast<Elf64_Ehdr*>(base);
    auto shdr = reinterpret_cast<Elf64_Shdr*>(base + ehdr->e_shoff);
    const char* shstrtab = reinterpret_cast<const char*>(base + shdr[ehdr->e_shstrndx].sh_offset);

    Elf64_Shdr* dynsym = nullptr;
    Elf64_Shdr* dynstr = nullptr;

    // 4. Locate .dynsym and .dynstr sections
    for (int i = 0; i < ehdr->e_shnum; i++) {
        std::string sectionName = shstrtab + shdr[i].sh_name;
        if (sectionName == ".dynsym")
            dynsym = &shdr[i];
        else if (sectionName == ".dynstr")
            dynstr = &shdr[i];
    }

    if (!dynsym || !dynstr) {
        std::cerr << "No .dynsym/.dynstr found in " << soPath << "\n";
        munmap(mapped, st.st_size);
        dlclose(handle);
        return exports;
    }

    auto symtab = reinterpret_cast<Elf64_Sym*>(base + dynsym->sh_offset);
    const char* strtab = reinterpret_cast<const char*>(base + dynstr->sh_offset);
    size_t symcount = dynsym->sh_size / sizeof(Elf64_Sym);

    // 5. Iterate and collect function symbols
    for (size_t i = 0; i < symcount; i++) {
        const Elf64_Sym& sym = symtab[i];
        if (ELF64_ST_TYPE(sym.st_info) == STT_FUNC && sym.st_value != 0 && sym.st_name != 0) {
            std::string name = strtab + sym.st_name;
            uintptr_t addr = baseAddr + sym.st_value;
            exports[name] = addr;
        }
    }

    // Cleanup
    munmap(mapped, st.st_size);
    dlclose(handle);
    return exports;
}
#endif


eoudll::CallableFunc::CallableFunc(uintptr_t memaddr) : memaddr(memaddr) {}
template <typename T_Ret, typename... T_Args>
T_Ret eoudll::CallableFunc::operator()(T_Args&&... args) const {
    // 1. Define the necessary function pointer type
    using FuncPtrType = T_Ret(*)(T_Args...);
    // 2. The critical step: Cast the raw integer address to the typed pointer.
    auto func_ptr = reinterpret_cast<FuncPtrType>(memaddr);
    // 3. Call the function via the correctly typed pointer.
    return func_ptr(std::forward<T_Args>(args)...);
}

// DummyFuncPtr operator[](auto a) const
eoudll::DummyFuncPtr eoudll::CallableFunc::operator[](int a) const {
     return reinterpret_cast<DummyFuncPtr>(memaddr);
}

eoudll::DYNAMICLIB::DYNAMICLIB(const char* const dllpath) {
    this->functions = GetExportedFunctions(dllpath);
}

eoudll::DYNAMICLIB::DYNAMICLIB(const std::string dllpath) {
    this->functions = GetExportedFunctions(dllpath);
}

eoudll::DYNAMICLIB::DYNAMICLIB(const DYNAMICLIB& lib) {
    this->functions = lib.functions;
}

eoudll::CallableFunc eoudll::DYNAMICLIB::operator[](const std::string &function_name) const {
    return CallableFunc(functions.at(function_name));
}


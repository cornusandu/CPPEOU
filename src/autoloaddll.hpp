#include <unordered_map>
#include <iostream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <cstring>
#include <cstdint>

std::unordered_map<std::string, uintptr_t> GetExportedFunctions(const std::string& dllPath);

namespace eoudll {
    using DummyFuncPtr = int(*)(...);

    struct CallableFunc {
        private:
            uintptr_t memaddr;

        public:
           CallableFunc(uintptr_t memaddr);
        
            template <typename T_Ret, typename... T_Args>
            T_Ret operator()(T_Args&&... args) const;

            DummyFuncPtr operator[](int a) const;
    };

    struct DYNAMICLIB {
        private:
            std::unordered_map<std::string, uintptr_t> functions;
        public:
            DYNAMICLIB(const char* const dllpath);
            DYNAMICLIB(const std::string dllpath);
            DYNAMICLIB(const DYNAMICLIB& lib);

            CallableFunc operator[](const std::string &function_name) const;
    };
}

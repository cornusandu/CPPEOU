#include "src/autoloaddll.hpp"

int main() {
    eoudll::DYNAMICLIB kernel32("C:/Windows/System32/kernel32.dll");
    eoudll::DYNAMICLIB user32("C:/Windows/System32/user32.dll");

    user32["MessageBoxW"][0](NULL, L"Hello from user32.dll!", L"Info", 0x00000000L);
    kernel32["ExitProcess"][0](5);

    return 2; // Never reached since were calling ExitProcess early
}
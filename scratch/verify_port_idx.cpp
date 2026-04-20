#include <iostream>
#include <string_view>

int main() {
    std::string_view name = "PORTB";
    std::cout << "name[4]: '" << name[4] << "'" << std::endl;
    std::cout << "port_idx: " << (int)(name[4] - 'A') << std::endl;
    return 0;
}

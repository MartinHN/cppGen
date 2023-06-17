#include <array>
#include <string>
#include <vector>

struct IMember {
    int i;
};
struct RootAPI {
    int a          = 0;
    unsigned int b = 0;
    // uint16_t u16 = 0;
    // uint32_t u32 = 0;
    long long ll         = 0;
    float c              = 0;
    double d             = 0;
    short l              = 0;
    char ch              = 'c';
    std::string string   = {};
    std::vector<int> vec = {};
    // std::array<int, 1000> arr = {};
    std::vector<IMember> vecM = {};
    int lastInt               = 0;
};

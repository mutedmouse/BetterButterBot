#ifndef TEMPLATETYPES_H
#define TEMPLATETYPES_H

#include <source_location>
#include <string>
#include <vector>

template<typename... Args>
class TemplateTypesInfo {
public:
    static constexpr std::string TypeNames() {
        std::string name = std::source_location::current().function_name();
        const size_t first = name.find_first_of('{') + 1;
        const size_t last = name.find_last_of('}');
        name = name.substr(first, last - first);

        return name;
    }

    static constexpr uint32_t TypesHash() {
        std::vector<std::string> substrings;
        std::string names = TypeNames();

        while(names.size() > 32) {
            substrings.emplace_back(names.substr(0, 32));
            names.erase(0, 32);
        }

        substrings.emplace_back(names);

        std::vector<uint32_t> hashes;
        for(const std::string& substring : substrings) {
            hashes.emplace_back(STD_STRING_HASH(substring));
        }

        while(hashes.size() > 1) {
            std::string newHashStr;

            constexpr static size_t MagicHashNumber = 3; // This is the max number of hashes that can fit into a 32 character string by appending, that is then used to calculate a new hash.
            for(size_t i = 0; i < std::min(hashes.size(), static_cast<size_t>(MagicHashNumber)); ++i) {
                newHashStr += std::to_string(hashes[i]);
            }

            hashes.erase(hashes.begin(), hashes.begin() + std::min(hashes.size(), static_cast<size_t>(MagicHashNumber)));
            hashes.emplace_back(STD_STRING_HASH(newHashStr));
        }

        return hashes.front();
    }

private:
    /**
     * @brief Default constructor is deleted to prevent creating instances of this purely static class.
     */
    TemplateTypesInfo() = delete;
};

#endif //TEMPLATETYPES_H

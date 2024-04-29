#include <cstdint>
std::vector<std::string> get_song_name_list(const std::vector<std::string>& _songs)
{
    std::vector<std::string> names;
    names.reserve(_songs.size());
    for(const auto& tune : _songs) {
        const auto semicolon_index = std::find(std::begin(tune), std::end(tune), ':');
        names.push_back({std::begin(tune), semicolon_index});
    }
    return names;
}

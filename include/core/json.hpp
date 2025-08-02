#ifndef NILOTIC_JSON_HPP
#define NILOTIC_JSON_HPP

// This is a wrapper around nlohmann/json to make it easier to include
// and to potentially swap out for another JSON library if needed

#include "../lib/nlohmann_json/single_include/nlohmann/json.hpp"

// Re-export the nlohmann::json class in our own namespace
namespace json = nlohmann;

#endif // NILOTIC_JSON_HPP
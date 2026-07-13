#pragma once
#include <filesystem>

int tangle (
    std::filesystem::path web_file_name,
    std::filesystem::path change_file_name,
    std::filesystem::path pascal_file_name,
    std::filesystem::path pool_file_name);

int tangle_exceptions_handled (
    std::filesystem::path web_file_name,
    std::filesystem::path change_file_name,
    std::filesystem::path pascal_file_name,
    std::filesystem::path pool_file_name);

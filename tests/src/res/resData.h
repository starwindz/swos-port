#pragma once

using ResFilenameList = std::vector<std::string>;
ResFilenameList findResFiles(const char *extension);
std::pair<std::unique_ptr<const char>, size_t> loadResFile(const char *name);

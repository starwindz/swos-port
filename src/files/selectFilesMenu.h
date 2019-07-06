#pragma once

using FileList = std::vector<std::pair<std::string, size_t>>;
std::string showSelectFilesMenu(const char *menuTitle, bool saving, const FileList& filenames);

#pragma once
#include <string>
#include <vector>

// Remember that string_view reference a string. If we are storing the string_view and the original string goes out of scope, problems will arise
inline std::vector<std::string_view> SplitStringView(std::string_view str, std::string_view delimiter)
{
	std::vector<std::string_view> elements;
	size_t start = 0;
	size_t end = str.find(delimiter);

	while (end != std::string_view::npos)
	{
		elements.push_back(str.substr(start, end - start));
		start = end + delimiter.length();
		end = str.find(delimiter, start);
	}
	elements.push_back(str.substr(start));

	return elements;
}
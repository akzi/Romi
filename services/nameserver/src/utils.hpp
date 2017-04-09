#pragma once
#include <windows.h>
#include <string>

struct get_filename
{
	std::string operator ()(const std::string &filepath)
	{
		auto pos = filepath.find_last_of('\\');
		if (pos == std::string::npos)
			pos = filepath.find_last_of('/');
		if (pos == std::string::npos)
			return filepath;
		++pos;
		return filepath.substr(pos, filepath.size() - pos);
	}
};

struct get_extension_name
{
	std::string operator()(const std::string &file)
	{
		auto pos = file.find_last_of('.');
		if (pos == std::string::npos)
			return{};
		return file.substr(pos, file.size() - pos);
	}
};

#ifdef _MSC_VER
struct file_type
{
	enum type
	{
		e_file,
		e_dir,
		e_link,
		e_block,
		e_unknown,
	};

	type operator()(const std::string &path)
	{
		DWORD dwAttr;
		dwAttr = GetFileAttributes(path.c_str());
		if (dwAttr == INVALID_FILE_ATTRIBUTES)
		{
			return type::e_unknown;
		}
		if (dwAttr & (FILE_ATTRIBUTE_HIDDEN |
			FILE_ATTRIBUTE_NORMAL |
			FILE_ATTRIBUTE_ARCHIVE))
		{
			return type::e_file;
		}
		else if (dwAttr & FILE_ATTRIBUTE_DIRECTORY)
		{
			return type::e_dir;
		}
		else if (dwAttr & FILE_ATTRIBUTE_REPARSE_POINT)
		{
			return type::e_link;
		}
		else if (dwAttr & (FILE_ATTRIBUTE_DEVICE))
		{
			return type::e_block;
		}
		return type::e_unknown;
	}
};
struct ls_files
{
	std::vector<std::string> operator()(const std::string &path, std::size_t depth = 1)
	{
		std::vector<std::string> files;
		WIN32_FIND_DATA find_data;
		HANDLE handle = ::FindFirstFile((path + "*.*").c_str(), &find_data);
		if (INVALID_HANDLE_VALUE == handle)
			return{};
		while (TRUE)
		{
			std::string filename(find_data.cFileName);
			std::string realpath;
			if (path.size() && (path.back() == '\\' || path.back() == '/'))
				realpath = path + filename;
			else
				realpath = path + "/" + filename;

			auto type = file_type()(realpath);
			if (type == file_type::e_file)
			{
				files.emplace_back(realpath);
			}
			else if (type == file_type::e_dir)
			{
				if (filename != "." && filename != ".." && depth > 1)
				{
					auto tmp = ls_files()(realpath + "/", depth - 1);
					for (auto &itr : tmp)
						files.emplace_back(std::move(itr));
				}
			}
			if (!FindNextFile(handle, &find_data))
				break;
		}
		FindClose(handle);
		return files;
	}
};
#endif
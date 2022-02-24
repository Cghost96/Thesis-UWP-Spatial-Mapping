#pragma once

#include <vector>
#include <string>
#include <ostream>
#include <filesystem>

#include <debugapi.h>

namespace SpatialMapping {
	namespace Helper
	{
		using namespace Windows::Storage;

		template <typename T>
		void LogMessage(const T value)
		{
			std::ostringstream os;
			os << value << "\n";
			OutputDebugStringA(os.str().c_str());
		}

		template <typename T>
		void LogMessage(const std::vector<T> values, int const format = 1)
		{
			std::ostringstream os;

			for (int i = 0; i < values.size(); i++)
			{
				if (i % format == 0 and i > 0)
				{
					os << "\n";
				}
				os << values[i] << " ";
			}

			OutputDebugStringA(os.str().c_str());
		}

		void inline ClearMeshFolder() {
			Platform::String^ folder = ApplicationData::Current->LocalFolder->Path + "\\Meshes";
			std::wstring folderW(folder->Begin());
			std::string folderA(folderW.begin(), folderW.end());
			if (std::filesystem::exists(folderA)) {
				std::filesystem::remove_all(folderA);
			}
		}
	}
}



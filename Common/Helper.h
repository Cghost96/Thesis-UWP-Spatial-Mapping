#pragma once

#include <vector>
#include <string>
#include <ostream>

#include <debugapi.h>

namespace SpatialMapping {
	namespace Helper
	{
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
	}
}



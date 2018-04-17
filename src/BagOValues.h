/********************************************************************

   BagOValues.h

   Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

********************************************************************/
#pragma once

#include <map>
#include <vector>
#include <algorithm>

#include "spinlock.h"

// never do this in a global namespace --> using namespace std;

namespace winfile {

	using namespace std;

	namespace bagovalues_internal {

		using namespace std;

		inline void lowerize(wstring & key) {
			transform(key.begin(), key.end(), key.begin(), ::tolower);
		};
	}

	template <typename TValue>
	class BagOValues final
	{
		struct lock_unlock final {
			SpinLock spinlock_{};
			lock_unlock() { spinlock_.Lock(); }
			~lock_unlock() { spinlock_.Unlock(); }
		};

	public:
		using storage_type = std::multimap< std::wstring, TValue >;
	private:
		mutable storage_type key_value_storage_{};
	public:
		void swap(BagOValues & other_)	noexcept
		{	// swap contents with other_
			this->key_value_storage_ = other_.key_value_storage_;
		}

		// does copy the value argument on call
		void Add(std::wstring key, TValue value)
		{
			lock_unlock padlock{};
			bagovalues_internal::lowerize(key);
			key_value_storage_.insert(key, value);
		}

		void Sort() noexcept
		{
			// std::multimap is already sorted
		}

		// 
		// fPrefix = true means return values for the tree at the point of the query matched; 
		//      we must consume the whole query for anything to be returned
		// fPrefix = false means that we only return values when an entire key matches and we match substrings of the query
		//
		// NOTE: returns a newly allocated vector; must delete it

		// DBJ: return vector of values found
		// DBJ: alsways assume fPrefix is false
		std::vector<TValue>
			Retrieve(
				const std::wstring & query,
				bool fPrefix = true,
				unsigned maxResults = ULONG_MAX
			)
		{
			lock_unlock padlock{};
			bagovalues_internal::lowerize( const_cast<std::wstring &>(query));
			auto range = key_value_storage_.equal_range(query);
			return vector<TValue>{ range.first, range.second };
		}

	};

} // winfile namespace
/********************************************************************

   BagOValues.h

   Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License.

********************************************************************/
#pragma once

// avoid min/max macros 
#define NOMINMAX

#include <map>
#include <vector>
#include <algorithm>
#include <cwctype>

#include "spinlock.h"

// never do this in a global namespace --> using namespace std;

namespace winfile {

	// never (ever) do this in a global namespace
	using namespace std;

	namespace internal {

		using namespace std;

		struct lock_unlock final {
			SpinLock spinlock_{};
			lock_unlock() { spinlock_.Lock(); }
			~lock_unlock() { spinlock_.Unlock(); }
		};

		template <typename T>
		class guardian final {
		public:
			
			typedef T value_type;

			value_type load() {
				lock_unlock locker_;
				return treasure_;
			}
			T store (value_type new_value ) {
				lock_unlock locker_;
				return treasure_ = new_value;
			}
		private:
			value_type treasure_{};
		};


		inline void lowerize(wstring & key) {
			transform(key.begin(), key.end(), key.begin(), std::towlower);
		};

		/* std::equal is very choosy as to in which c++ language version  
		   is it gpoing to show what face of many .. sigh
		   so to avoid ambiguities we shall use this version from
		   http://en.cppreference.com/w/cpp/algorithm/equal
		 */
		template<class InputIt1, class InputIt2>
		bool equal_(InputIt1 first1, InputIt1 last1, InputIt2 first2)
		{
			for (; first1 != last1; ++first1, ++first2) {
				if (!(*first1 == *first2)) {
					return false;
				}
			}
			return true;
		}

		inline  bool  is_prefix( 
			const std::wstring & lhs, const std::wstring & rhs
		  )
		{
			_ASSERTE( lhs.size() > 0 );
			_ASSERTE( rhs.size() > 0 ) ;

			return equal_(
				lhs.begin(),
				lhs.begin() + std::min(lhs.size(), rhs.size()),
				rhs.begin());
		}

		inline auto interlocked_exchange(
			PVOID *	Destination,
			PVOID		Value
		)
		{
			_ASSERTE(Destination);
			_ASSERTE(Value);
			PVOID retval_ = reinterpret_cast<PVOID>(
				InterlockedExchangePointer(Destination, Value)
				);
			_ASSERTE(retval_ != nullptr);
			return retval_;
		};

	}

	template <typename TValue>
	class BagOValues final
	{
		using lock_unlock = internal::lock_unlock;
	public:
		using storage_type = std::multimap< std::wstring, TValue >;
	private:
		/*mutable*/ storage_type key_value_storage_{};
	public:
		/*
		the aim is to be able to use instances of this class as values
		c++ compiler will generate all the things necessary 
		we will privide the "swap" method to be used for move semantics

		thus this class must be trivially copyable
		http://en.cppreference.com/w/cpp/types/is_trivially_copyable
		*/
		void swap(BagOValues & other_)	noexcept
		{	// swap contents with other_
			this->key_value_storage_ = other_.key_value_storage_;
		}
		// DBJ added
		bool Empty() const noexcept {
			return this->key_value_storage_.size() < 1;
		}
		// does copy the value argument on call
		void Add(std::wstring key, TValue value)
		{
			lock_unlock padlock{};
			internal::lowerize(key);
			key_value_storage_.emplace(key, value);
			// key_value_storage_.insert( std::make_pair(key, value) );
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

		/*
		   DBJ logic deciphering aka translation of the above

		   It seems fBprefis had the meaning of "key starts with"
		   if it is true (it seems) all the keys that start with the query given should be taken into account

		   if fBprefix is false the exact key match should  be performed

		   right now we always assume fPrefix is false
		*/


		std::vector<TValue>
			Retrieve(
				const		std::wstring & query,
				bool		fPrefix = true,
				/* currently we ignore maxResults */
				unsigned	maxResults = ULONG_MAX
			)
		{
			lock_unlock padlock{};
			std::vector<TValue> retval_{};
				if (key_value_storage_.size() < 1) 
					return retval_;

			internal::lowerize(const_cast<std::wstring &>(query));
			
			if ( true == fPrefix) {
				retval_ = prefix_match_query(query);
			}
			
			if ( false == fPrefix) {
				retval_ = exact_match_query(query);
			}
			return retval_;
		}

	private: 
		/* do not use as public in this form */
		std::vector<TValue>
			exact_match_query(
				// must be lower case!
				const	std::wstring & query
			)
		{
			lock_unlock padlock{};
			std::vector<TValue> retvec{};

			auto range = key_value_storage_.equal_range(query);
			if (range.first == key_value_storage_.end()) return retvec;

			std::transform(
				range.first,
				range.second,
				std::back_inserter(retvec),
				[](storage_type::value_type element) { return element.second; }
			);
			return retvec;
		}

		/* do not use as public in this form */
		std::vector<TValue>
			prefix_match_query(
				// must be lower case!
				const	std::wstring & prefix_
			)
		{
			lock_unlock padlock{};
			std::vector<TValue> retvec{};
			auto walker_ = key_value_storage_.upper_bound (prefix_);

			while (walker_ != key_value_storage_.end())
			{
				// if prefix of the current key add its value to the result
				if (internal::is_prefix(prefix_, walker_->first)) {
					retvec.push_back( walker_->second );
				}
				// advance the iterator 
				walker_++;
			}

			return retvec;
		}

	}; // eof BagOValues 

} // winfile namespace
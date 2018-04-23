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
#include <atomic>

namespace winfile {

	using namespace std;

	namespace internal {

		struct lock_unlock final {
			std::atomic_flag atom_flag_ = ATOMIC_FLAG_INIT;

			lock_unlock() { atom_flag_.test_and_set(); }
			~lock_unlock() { atom_flag_.clear(); }
		};

		template <typename T>
		class guardian final {
		public:
				typedef T value_type;
			value_type load() {
				lock_unlock locker_;
				return treasure_;
			}
			value_type store(value_type new_value) {
				lock_unlock locker_;
				return treasure_ = new_value;
			}
		private:
			value_type treasure_{};
		};

		inline void lowerize(wstring & key) {
			transform(key.begin(), key.end(), key.begin(), std::towlower);
		};

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

		/// <summary>
		/// is Lhs prefix to Rhs
		/// </summary>
		inline  bool  is_prefix(
			const std::wstring & lhs, const std::wstring & rhs
		)
		{
			_ASSERTE(lhs.size() > 0);
			_ASSERTE(rhs.size() > 0);

			// "predefined" can not be a prefix to "pre" 
			// opposite is true
			if (lhs.size() > rhs.size()) return false;

			return equal_(
				lhs.begin(),
				lhs.end(), 
				rhs.begin());
		}

	}

	template <typename T>
	class BagOValues final
	{
		using lock_unlock = internal::lock_unlock;
	public:
		typedef T value_type;
		using storage_type = std::multimap< std::wstring, value_type >;
		using value_vector = std::vector< value_type >;
	private:
		mutable storage_type key_value_storage_{};
	public:
		/*
		the aim is to be able to use instances of this class as values
		c++ compiler will generate all the things necessary
		we will privide the "swap" method to be used for move semantics
		*/
		void swap(BagOValues & other_)	noexcept
		{	// swap contents with other_
			lock_unlock padlock{};
			this->key_value_storage_ = other_.key_value_storage_;
		}
		
		/// <summary>
		/// clear the storage held
		/// </summary>
		void Clear() const {
			lock_unlock padlock{};
			key_value_storage_.clear();
		}

		/// <summary>
		/// the argument of the callback is the 
		/// storage_type::value_type
		/// which in turn is pair 
		/// wstring is the key, T is the value
		/// </summary>
		template< typename F>
		const auto ForEach(F callback_) const noexcept {
			lock_unlock padlock{};
			return
				std::for_each(
					this->key_value_storage_.begin(),
					this->key_value_storage_.end(),
					callback_
				);
		}
		
		/// <summary>
		/// is the storage held empty?
		/// </summary>
		const bool Empty() const noexcept {
			lock_unlock padlock{};
			return this->key_value_storage_.size() < 1;
		}
		
		/// <summary>
		/// add's by enforcing the value semantics
		/// </summary>
		void Add(std::wstring key, value_type value)
		{
			lock_unlock padlock{};
			internal::lowerize(key);
			key_value_storage_.emplace(key, value);
		}

		/// <summary>
		/// std::multimap is already sorted
		/// </summary>
		void Sort() noexcept
		{
			// no op
		}

		/// <summary>
		/// if find_by_prefix is false the exact key match should  be performed
		/// if true all the keys matching are going into the result
		/// </summary>
		value_vector
			Retrieve(
				const		std::wstring & query,
				bool		find_by_prefix = true,
				/* currently we ignore maxResults */
				unsigned	maxResults = ULONG_MAX
			)
		{
			lock_unlock padlock{};
			value_vector retval_{};
			if (key_value_storage_.size() < 1)
				return retval_;

			internal::lowerize(const_cast<std::wstring &>(query));

			if (true == find_by_prefix) {
				retval_ = prefix_match_query(query);

				if (retval_.size() < 1)
					find_by_prefix = false;
			}

			if (false == find_by_prefix) {
				retval_ = exact_match_query(query);
			}
			return retval_;
		}

	private:
		/* do not use as public in this form */
		value_vector
			exact_match_query(
				// must be lower case!
				const	std::wstring & query
			)
		{
			value_vector retvec{};

			auto range = key_value_storage_.equal_range(query);
			if (range.first == key_value_storage_.end()) return retvec;

			std::transform(
				range.first,
				range.second,
				std::back_inserter(retvec),
				[](typename storage_type::value_type element) { return element.second; }
			);
			return retvec;
		}

		/* do not use as public in this form */
		value_vector
			prefix_match_query(
				// must be lower case!
				const	std::wstring & prefix_
			)
		{
			value_vector retvec{};
			auto walker_ = key_value_storage_.upper_bound(prefix_);

			while (walker_ != key_value_storage_.end())
			{
				// if prefix of the current key add its value to the result
				if (internal::is_prefix(prefix_, walker_->first)) {
					retvec.push_back(walker_->second);
					// advance the iterator 
					walker_++;
				}
				else {
					break;
				}
			}

			return retvec;
		}

	}; // eof BagOValues 

} // winfile namespace
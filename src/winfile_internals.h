#pragma once

/// <summary>
/// this is where we keep reusable 'things'
/// or atifacts one will use to sort out 
/// (for example) vs2017 code analyzer 
/// good advice and warnings
/// 
/// c++ stuff one can keep here for later or
/// never use it, it depends on the course 
/// of events
/// </summary>
#ifdef __cplusplus

#include <sysinfoapi.h>
// avoid min/max macros 
#define NOMINMAX
// mutex is (much) lighter on cpu vs std::atomic_flag
#define WINFILE_STD_MUTEX_USE 

#include <string>
#include <map>
#include <vector>
#include <algorithm>
#include <cwctype>
#ifndef WINFILE_STD_MUTEX_USE 
#include <atomic>
#else
#include <mutex>
#endif

namespace winfile {
	namespace internal {

		using namespace std;

#ifndef WINFILE_STD_MUTEX_USE 
		struct lock_unlock final {
			std::atomic_flag atom_flag_ = ATOMIC_FLAG_INIT;

			lock_unlock() { while (atom_flag_.test_and_set()); }
			~lock_unlock() { atom_flag_.clear(); }
		};
#else
		struct lock_unlock final {
			std::mutex mux_;

			lock_unlock() { mux_.lock(); }
			~lock_unlock() { mux_.unlock(); }
		};
#endif

		template <typename T>
		class guardian final {
		public:
			typedef T value_type;
			value_type load() const {
				lock_unlock locker_;
				return treasure_;
			}
			value_type store(value_type new_value) const {
				lock_unlock locker_;
				return treasure_ = new_value;
			}
		private:
			mutable value_type treasure_{};
		};

		/// <summary>
		/// argument is a reference 
		/// and is "lowerized" in place
		/// </summary>
		inline void lowerize(wstring & key) {
			transform(key.begin(), key.end(), key.begin(), std::towlower);
		};

		// std::equal has many overloads
		// it is less error prone to have here
		// and use this one we exactly need
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
		/// L must be shorter than R
		/// </summary>
		inline  bool  is_prefix(
			const std::wstring_view & lhs, const std::wstring_view & rhs
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

		// return e.g. L"C:\windows\system32"
		inline const std::wstring windir()
		{
			lock_unlock padlock_;
			static wchar_t  result_buf[BUFSIZ]{};
			static auto retval = ::GetSystemDirectoryW(
				result_buf,
				BUFSIZ
			);
			_ASSERTE(retval);
			return wstring{ result_buf };
		}

		// DBJ: what happens if there is no drive C: ?
		// we get the system drive letter
		// the first 3 chars that is, e.g. L"C:\\"
		inline const std::wstring windrive()
		{
			lock_unlock padlock_;
			static wstring result = windir().substr(0, 3);
			_ASSERTE(result.size() == 3);
			return result;
		}

		extern "C" inline auto
			add_backslash(wstring & path_)
		{
			static auto CHAR_BACKSLASH{ L'\\' };
			if (path_.back() != CHAR_BACKSLASH) {
				path_.append(wstring{ CHAR_BACKSLASH });
			}
			return path_.size();
		}

	} // internal
} // winfile

#endif // __cpluspus

  /*
  and here is the C interface to winfile internals
  */

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif /* __cplusplus */

	/*
	use string comparisons bellow to solve:
	warning C6400: Using 'lstrcmpiW' to perform a case-insensitive compare
	Yields unexpected results in non-English locales.
	*/
	int winfile_ordinal_string_compareA(const char * str1, const char * str2, unsigned char ignore_case);
	int winfile_ordinal_string_compareW(const wchar_t * str1, const wchar_t * str2, unsigned char ignore_case);

#if _UNICODE
#define winfile_ordinal_string_compare winfile_ordinal_string_compareW
#else
#define winfile_exact_string_compare winfile_ordinal_string_compareA
#endif

	int winfile_ui_string_compare(LPCTSTR str1, LPCTSTR str2, unsigned char ignore_case);

#ifdef __cplusplus
} // extern "C" 
#endif /* __cplusplus */
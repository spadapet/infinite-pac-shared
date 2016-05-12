#pragma once

namespace ff
{
	class String;
	typedef const String &StringRef;
	typedef String &StringOut;

	// Sometimes a function needs to return a string reference, but needs
	// something to return for failure cases. Use this.
	UTIL_API StringRef GetEmptyString();

	// Ref-counted string class that acts mostly like std::string (but with copy-on-write).
	//
	// The string can be read on multiple threads, as long as none of them modify it.
	// That's not safe, even with the copy-on-write design. Getting that to work isn't worth it.
	class UTIL_API String
	{
	public:
		static const size_t npos = INVALID_SIZE;

		String();
		String(StringRef rhs);
		String(StringRef rhs, size_t pos, size_t count = npos);
		String(String &&rhs);
		explicit String(const wchar_t *rhs, size_t count = npos);
		String(size_t count, wchar_t ch);
		String(const wchar_t *start, const wchar_t *end);
		~String();

		StringOut operator=(StringRef rhs);
		StringOut operator=(String &&rhs);
		StringOut operator=(const wchar_t *rhs);
		StringOut operator=(wchar_t ch);

		String operator+(StringRef rhs) const;
		String operator+(String &&rhs) const;
		String operator+(const wchar_t *rhs) const;
		String operator+(wchar_t ch) const;

		StringOut operator+=(StringRef rhs);
		StringOut operator+=(String &&rhs);
		StringOut operator+=(const wchar_t *rhs);
		StringOut operator+=(wchar_t ch);

		bool operator==(StringRef rhs) const;
		bool operator==(const wchar_t *rhs) const;
		bool operator==(wchar_t ch) const;

		bool operator!=(StringRef rhs) const;
		bool operator!=(const wchar_t *rhs) const;
		bool operator!=(wchar_t ch) const;

		bool operator<(StringRef rhs) const;
		bool operator<(const wchar_t *rhs) const;
		bool operator<(wchar_t ch) const;

		bool operator<=(StringRef rhs) const;
		bool operator<=(const wchar_t *rhs) const;
		bool operator<=(wchar_t ch) const;

		bool operator>(StringRef rhs) const;
		bool operator>(const wchar_t *rhs) const;
		bool operator>(wchar_t ch) const;

		bool operator>=(StringRef rhs) const;
		bool operator>=(const wchar_t *rhs) const;
		bool operator>=(wchar_t ch) const;

		StringOut assign(StringRef rhs, size_t pos = 0, size_t count = npos);
		StringOut assign(String &&rhs);
		StringOut assign(const wchar_t *rhs, size_t count = npos);
		StringOut assign(size_t count, wchar_t ch);
		StringOut assign(const wchar_t *start, const wchar_t *end);

		StringOut append(StringRef rhs, size_t pos = 0, size_t count = npos);
		StringOut append(const wchar_t *rhs, size_t count = npos);
		StringOut append(size_t count, wchar_t ch);
		StringOut append(const wchar_t *start, const wchar_t *end);

		StringOut insert(size_t pos, StringRef rhs, size_t rhs_pos = 0, size_t count = npos);
		StringOut insert(size_t pos, const wchar_t *rhs, size_t count = npos);
		StringOut insert(size_t pos, size_t count, wchar_t ch);
		const wchar_t *insert(const wchar_t *pos, wchar_t ch);
		const wchar_t *insert(const wchar_t *pos, const wchar_t *start, const wchar_t *end);
		const wchar_t *insert(const wchar_t *pos, size_t count, wchar_t ch);

		void push_back(wchar_t ch);
		void pop_back();

		const wchar_t *erase(const wchar_t *start, const wchar_t *end);
		const wchar_t *erase(const wchar_t *pos);
		StringOut erase(size_t pos = 0, size_t count = npos);

		StringOut replace(size_t pos, size_t count, const wchar_t *rhs, size_t rhs_count = npos);
		StringOut replace(size_t pos, size_t count, StringRef rhs, size_t rhs_pos = 0, size_t rhs_count = npos);
		StringOut replace(size_t pos, size_t count, size_t ch_count, wchar_t ch);
		StringOut replace(const wchar_t *start, const wchar_t *end, const wchar_t *rhs, size_t rhs_count = npos);
		StringOut replace(const wchar_t *start, const wchar_t *end, StringRef rhs);
		StringOut replace(const wchar_t *start, const wchar_t *end, size_t ch_count, wchar_t ch);
		StringOut replace(const wchar_t *start, const wchar_t *end, const wchar_t *start2, const wchar_t *end2);

		typedef SharedStringVectorAllocator::StringVector::iterator iterator;
		typedef SharedStringVectorAllocator::StringVector::const_iterator const_iterator;
		typedef SharedStringVectorAllocator::StringVector::reverse_iterator reverse_iterator;
		typedef SharedStringVectorAllocator::StringVector::const_reverse_iterator const_reverse_iterator;

		iterator begin();
		const_iterator begin() const;
		const_iterator cbegin() const;
		iterator end();
		const_iterator end() const;
		const_iterator cend() const;
		reverse_iterator rbegin();
		const_reverse_iterator rbegin() const;
		const_reverse_iterator crbegin() const;
		reverse_iterator rend();
		const_reverse_iterator rend() const;
		const_reverse_iterator crend() const;

		wchar_t &front();
		const wchar_t &front() const;
		wchar_t &back();
		const wchar_t &back() const;

		wchar_t &at(size_t pos);
		const wchar_t &at(size_t pos) const;

		wchar_t &operator[](size_t pos);
		const wchar_t &operator[](size_t pos) const;

		const wchar_t *c_str() const;
		const wchar_t *data() const;
		size_t length() const;
		size_t size() const;
		bool empty() const;
		size_t max_size() const;
		size_t capacity() const;

		void clear();
		void resize(size_t count);
		void resize(size_t count, wchar_t ch);
		void reserve(size_t alloc);
		void shrink_to_fit();

		size_t copy(wchar_t * out, size_t count, size_t pos = 0);
		void swap(StringOut rhs);
		String substr(size_t pos = 0, size_t count = npos) const;

		// Custom functions that aren't part of std::string
		bool format(const wchar_t *format, ...);
		bool format_v(const wchar_t *format, va_list args);
		static String format_new(const wchar_t *format, ...);
		static String format_new_v(const wchar_t *format, va_list args);
		static String from_acp(const char *str);
		static String from_static(const wchar_t *str, size_t len = npos);
#if METRO_APP
		Platform::String ^pstring() const;
		static String from_pstring(Platform::String ^str);
#endif

		size_t find(StringRef rhs, size_t pos = 0) const;
		size_t find(const wchar_t *rhs, size_t pos = 0, size_t count = npos) const;
		size_t find(wchar_t ch, size_t pos = 0) const;

		size_t rfind(StringRef rhs, size_t pos = npos) const;
		size_t rfind(const wchar_t *rhs, size_t pos = npos, size_t count = npos) const;
		size_t rfind(wchar_t ch, size_t pos = npos) const;

		size_t find_first_of(StringRef rhs, size_t pos = 0) const;
		size_t find_first_of(const wchar_t *rhs, size_t pos = 0, size_t count = npos) const;
		size_t find_first_of(wchar_t ch, size_t pos = 0) const;

		size_t find_last_of(StringRef rhs, size_t pos = npos) const;
		size_t find_last_of(const wchar_t *rhs, size_t pos = npos, size_t count = npos) const;
		size_t find_last_of(wchar_t ch, size_t pos = npos) const;

		size_t find_first_not_of(StringRef rhs, size_t pos = 0) const;
		size_t find_first_not_of(const wchar_t *rhs, size_t pos = 0, size_t count = npos) const;
		size_t find_first_not_of(wchar_t ch, size_t pos = 0) const;

		size_t find_last_not_of(StringRef rhs, size_t pos = npos) const;
		size_t find_last_not_of(const wchar_t *rhs, size_t pos = npos, size_t count = npos) const;
		size_t find_last_not_of(wchar_t ch, size_t pos = npos) const;

		int compare(StringRef rhs, size_t rhs_pos = 0, size_t rhs_count = npos) const;
		int compare(size_t pos, size_t count, StringRef rhs, size_t rhs_pos = 0, size_t rhs_count = npos) const;
		int compare(const wchar_t *rhs, size_t rhs_count = npos) const;
		int compare(size_t pos, size_t count, const wchar_t *rhs, size_t rhs_count = npos) const;

	private:
		SharedStringVectorAllocator::StringVector &Str();
		const SharedStringVectorAllocator::StringVector &StrConst() const;

		SharedStringVectorAllocator::SharedStringVector *_str;
	};

	class StaticString
	{
	public:
		UTIL_API StaticString(const wchar_t *sz, size_t len);

		template<size_t len>
		StaticString(const wchar_t (&sz)[len])
		{
			Initialize(sz, len);
		}

		UTIL_API StringRef GetString() const;
		UTIL_API operator StringRef() const;
		UTIL_API hash_t GetHash() const;

	private:
		UTIL_API void Initialize(const wchar_t *sz, size_t lenWithNull);

		SharedStringVectorAllocator::SharedStringVector *_str;
		SharedStringVectorAllocator::SharedStringVector _data;
		mutable hash_t _hash;
	};

	template<>
	inline hash_t HashFunc<String>(StringRef val)
	{
		return HashBytes(val.c_str(), val.size() * sizeof(wchar_t));
	}

	template<>
	inline hash_t HashFunc<StaticString>(const StaticString &val)
	{
		return val.GetHash();
	}
}

UTIL_API std::wostream &operator<<(std::wostream &output, ff::StringRef str);

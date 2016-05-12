#pragma once

namespace ff
{
	// Allocator for raw string buffers.
	//
	// Can be used by anything, but the main consumer is the Vector within the String class.
	// It is not safe to use this allocator until the program has initialized the ProcessGlobals.
	struct StringAllocator
	{
		wchar_t *New(size_t count)
		{
			return NewPoolString(count);
		}

		void Delete(wchar_t *str)
		{
			DeletePoolString(str);
		}

		wchar_t *Malloc(size_t count)
		{
			return New(count);
		}

		void Free(wchar_t *str)
		{
			return Delete(str);
		}

	private:
		static wchar_t *NewPoolString(size_t count);
		UTIL_API static void DeletePoolString(wchar_t *str);
	};

	// Allocator for a dynamic Vector of TCHARs for strings.
	//
	// The main consumer of this allocator is the String class.
	// It is not safe to use this allocator until the program has initialized the ProcessGlobals.
	struct SharedStringVectorAllocator
	{
		// Strings can store 15 characters (+null) before needing to allocate extra memory
		typedef Vector<wchar_t, 16, StringAllocator> StringVector;
		typedef SharedObject<StringVector, SharedStringVectorAllocator> SharedStringVector;

		SharedStringVector *NewOne()
		{
			return NewSharedStringVector();
		}

		SharedStringVector *NewOne(const SharedStringVector &rhs)
		{
			return NewSharedStringVector(rhs);
		}

		SharedStringVector *NewOne(SharedStringVector &&rhs)
		{
			return NewSharedStringVector(std::move(rhs));
		}

		void DeleteOne(SharedStringVector *str)
		{
			DeleteSharedStringVector(str);
		}

	private:
		static SharedStringVector *NewSharedStringVector();
		static SharedStringVector *NewSharedStringVector(const SharedStringVector &rhs);
		static SharedStringVector *NewSharedStringVector(SharedStringVector &&rhs);
		static void DeleteSharedStringVector(SharedStringVector *str);
	};
}

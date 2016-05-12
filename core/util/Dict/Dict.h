#pragma once

#include "Dict/Value.h"
#include "Dict/SmallDict.h"

namespace ff
{
	class Dict
	{
	public:
		UTIL_API Dict();
		UTIL_API Dict(const Dict &rhs);
		UTIL_API Dict(Dict &&rhs);
		UTIL_API Dict(const SmallDict &rhs);
		UTIL_API Dict(SmallDict &&rhs);
		UTIL_API ~Dict();

		// Copying
		UTIL_API const Dict &operator=(const Dict &rhs);
		UTIL_API const Dict &operator=(const SmallDict &rhs);
		UTIL_API void Add(const Dict &rhs);
		UTIL_API void Add(const SmallDict &rhs);
		UTIL_API void Merge(const Dict &rhs);
		UTIL_API void Reserve(size_t count);
		UTIL_API bool IsEmpty() const;
		UTIL_API size_t Size() const;

		// Operations
		UTIL_API void Clear();
		UTIL_API Vector<String> GetAllNames(bool sorted = false) const;

		// Generic set/get
		UTIL_API void SetValue(StringRef name, Value *value);
		UTIL_API Value *GetValue(StringRef name) const;

		// Option setters
		UTIL_API void SetInt(StringRef name, int value);
		UTIL_API void SetSize(StringRef name, size_t value);
		UTIL_API void SetBool(StringRef name, bool value);
		UTIL_API void SetRect(StringRef name, RectInt value);
		UTIL_API void SetRectF(StringRef name, RectFloat value);
		UTIL_API void SetFloat(StringRef name, float value);
		UTIL_API void SetDouble(StringRef name, double value);
		UTIL_API void SetPoint(StringRef name, PointInt value);
		UTIL_API void SetPointF(StringRef name, PointFloat value);
		UTIL_API void SetString(StringRef name, StringRef value);
		UTIL_API void SetGuid(StringRef name, REFGUID value);
		UTIL_API void SetData(StringRef name, IData *value);
		UTIL_API void SetData(StringRef name, const void *data, size_t size);
		UTIL_API void SetSavedData(StringRef name, ISavedData *value);
		UTIL_API void SetDict(StringRef name, const Dict &value);
		UTIL_API void SetResource(StringRef name, const SharedResourceValue &value);

		// Option getters
		UTIL_API int GetInt(StringRef name, int defaultValue = 0) const;
		UTIL_API size_t GetSize(StringRef name, size_t defaultValue = 0) const;
		UTIL_API bool GetBool(StringRef name, bool defaultValue = false) const;
		UTIL_API RectInt GetRect(StringRef name, RectInt defaultValue = RectInt(0, 0, 0, 0)) const;
		UTIL_API RectFloat GetRectF(StringRef name, RectFloat defaultValue = RectFloat(0, 0, 0, 0)) const;
		UTIL_API float GetFloat(StringRef name, float defaultValue = 0.0f) const;
		UTIL_API double GetDouble(StringRef name, double defaultValue = 0.0) const;
		UTIL_API PointInt GetPoint(StringRef name, PointInt defaultValue = PointInt(0, 0)) const;
		UTIL_API PointFloat GetPointF(StringRef name, PointFloat defaultValue = PointFloat(0, 0)) const;
		UTIL_API String GetString(StringRef name, String defaultValue = String()) const;
		UTIL_API GUID GetGuid(StringRef name, REFGUID defaultValue = GUID_NULL) const;
		UTIL_API ComPtr<IData> GetData(StringRef name) const;
		UTIL_API ComPtr<ISavedData> GetSavedData(StringRef name) const;
		UTIL_API Dict GetDict(StringRef name) const;
		UTIL_API SharedResourceValue GetResource(StringRef name) const;

		// Enum get/set helper

		template<typename E>
		void SetEnum(StringRef name, E value)
		{
			SetInt(name, (int)value);
		}

		template<typename E>
		E GetEnum(StringRef name, E defaultValue = (E)0) const
		{
			return (E)GetInt(name, (int)defaultValue);
		}

		UTIL_API void DebugDump() const;

	private:
		void InternalGetAllNames(Set<String> &names) const;
		void CheckSize();
		StringCache &GetAtomizer() const;

		typedef Map<hash_t, ValuePtr, NonHasher<hash_t>> PropsMap;

		StringCache *_atomizer;
		std::unique_ptr<PropsMap> _propsLarge;
		SmallDict _propsSmall;
	};
}

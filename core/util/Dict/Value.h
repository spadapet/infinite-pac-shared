#pragma once

namespace ff
{
	class Dict;
	class ResourceValue;
	class Value;
	class IData;
	class ISavedData;

	typedef std::shared_ptr<ResourceValue> SharedResourceValue;
	typedef SmartPtr<Value> ValuePtr;

	// a ref-counted variant type

	class UTIL_API Value
	{
	public:
		enum class Type : BYTE
		{
			// Existing values must not be removed or changed since they are persisted.

			Null,
			Bool,
			Double,
			Float,
			Int,
			Object,
			Data,
			Dict,
			String,
			Guid,
			Point,
			Rect,
			PointF,
			RectF,
			SavedData,
			SavedDict,
			DoubleVector,
			FloatVector,
			IntVector,
			DataVector,
			StringVector,
			ValueVector,
			Resource,
		};

		Type GetType() const;
		bool IsType(Type type) const;
		bool IsNumberType() const;

		// constructors
		static bool CreateNull(Value **ppValue);
		static bool CreateBool(bool val, Value **ppValue);
		static bool CreateDouble(double val, Value **ppValue);
		static bool CreateFloat(float val, Value **ppValue);
		static bool CreateInt(int val, Value **ppValue);
		static bool CreateObject(IUnknown *pObj, Value **ppValue);
		static bool CreateData(IData *pData, Value **ppValue);
		static bool CreateDict(Dict &&dict, Value **ppValue);
		static bool CreateString(StringRef str, Value **ppValue);
		static bool CreateGuid(REFGUID guid, Value **ppValue);
		static bool CreatePoint(const PointInt &point, Value **ppValue);
		static bool CreateRect(const RectInt &rect, Value **ppValue);
		static bool CreatePointF(const PointFloat &point, Value **ppValue);
		static bool CreateRectF(const RectFloat &rect, Value **ppValue);
		static bool CreateSavedData(ISavedData *data, Value **ppValue);
		static bool CreateSavedDict(ISavedData *data, Value **ppValue);
		static bool CreateSavedDict(const Dict &dict, bool compress, Value **ppValue);
		static bool CreateDoubleVector(Vector<double> &&vec, Value **ppValue);
		static bool CreateFloatVector(Vector<float> &&vec, Value **ppValue);
		static bool CreateIntVector(Vector<int> &&vec, Value **ppValue);
		static bool CreateDataVector(Vector<ComPtr<IData>> &&vec, Value **ppValue);
		static bool CreateStringVector(Vector<String> &&vec, Value **ppValue);
		static bool CreateValueVector(Vector<ValuePtr> &&vec, Value **ppValue);
		static bool CreateResource(const SharedResourceValue &res, Value **ppValue);

		// accessors
		bool AsBool() const;
		double AsDouble() const;
		float AsFloat() const;
		int AsInt() const;
		IUnknown *AsObject() const;
		IData *AsData() const;
		const Dict &AsDict() const;
		StringRef AsString() const;
		REFGUID AsGuid() const;
		const PointInt &AsPoint() const;
		const RectInt &AsRect() const;
		const PointFloat &AsPointF() const;
		const RectFloat &AsRectF() const;
		ISavedData *AsSavedData() const;
		const Vector<double> &AsDoubleVector() const;
		const Vector<float> &AsFloatVector() const;
		const Vector<int> &AsIntVector() const;
		const Vector<ComPtr<IData>> &AsDataVector() const;
		const Vector<String> &AsStringVector() const;
		const Vector<ValuePtr> &AsValueVector() const;
		const SharedResourceValue &AsResource() const;

		bool Convert(Type type, Value **ppValue) const;
		bool operator==(const Value &r) const;
		bool Compare(const Value *p) const;

		void AddRef();
		void Release();

		// Don't use these directly
		Value();
		~Value();

	private:
		// not implemented, cannot copy
		Value(const Value &r);
		const Value &operator=(const Value &r);

		static Value *NewValueOneRef();
		static void DeleteValue(Value *val);

		struct SPoint { int pt[2]; };
		struct SRect { int rect[4]; };
		struct SPointF { float pt[2]; };
		struct SRectF { float rect[4]; };
		struct SDict { size_t data[4]; Dict *AsDict() const; };
		struct SResource { size_t data[4]; SharedResourceValue *AsResource() const; };
		struct StaticValue { size_t data[6]; Value *AsValue() const; };

		void SetType(Type type);
		PointInt &InternalGetPoint() const;
		RectInt &InternalGetRect() const;
		PointFloat &InternalGetPointF() const;
		RectFloat &InternalGetRectF() const;
		StringOut InternalGetString() const;

		union
		{
			bool _bool;
			double _double;
			float _float;
			int _int;
			IUnknown *_object;
			IData *_data;
			ISavedData *_savedData;
			SDict _dict;
			void *_string;
			GUID _guid;
			SPoint _point;
			SRect _rect;
			SPointF _pointF;
			SRectF _rectF;
			SResource _resource;
			Vector<double> *_doubleVector;
			Vector<float> *_floatVector;
			Vector<int> *_intVector;
			Vector<ComPtr<IData>> *_dataVector;
			Vector<String> *_stringVector;
			Vector<ValuePtr> *_valueVector;
		};

		long _refCount;
		Type _type;
	};
}

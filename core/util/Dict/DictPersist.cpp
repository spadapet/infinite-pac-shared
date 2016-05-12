#include "pch.h"
#include "Data/Data.h"
#include "Data/DataPersist.h"
#include "Data/DataWriterReader.h"
#include "Data/SavedData.h"
#include "Dict/Dict.h"
#include "Dict/DictPersist.h"
#include "Dict/Value.h"
#include "Globals/Log.h"
#include "Module/Module.h"
#include "Module/ModuleFactory.h"
#include "Resource/ResourcePersist.h"
#include "Resource/ResourceValue.h"
#include "String/StringCache.h"
#include "String/StringUtil.h"

static DWORD DICT_HEADER = 'DICT';

static bool CanSaveValue(ff::Value *value)
{
	if (value)
	{
		switch (value->GetType())
		{
		case ff::Value::Type::Null:
		case ff::Value::Type::Bool:
		case ff::Value::Type::Double:
		case ff::Value::Type::Float:
		case ff::Value::Type::Int:
		case ff::Value::Type::Data:
		case ff::Value::Type::Dict:
		case ff::Value::Type::String:
		case ff::Value::Type::Guid:
		case ff::Value::Type::Point:
		case ff::Value::Type::Rect:
		case ff::Value::Type::PointF:
		case ff::Value::Type::RectF:
		case ff::Value::Type::SavedData:
		case ff::Value::Type::SavedDict:
		case ff::Value::Type::DoubleVector:
		case ff::Value::Type::FloatVector:
		case ff::Value::Type::IntVector:
		case ff::Value::Type::DataVector:
		case ff::Value::Type::StringVector:
		case ff::Value::Type::ValueVector:
		case ff::Value::Type::Resource:
			return true;

		case ff::Value::Type::Object:
			{
				ff::ComPtr<ff::IResourceSave> res;
				return res.QueryFrom(value->AsObject());
			}
		}
	}

	return false;
}

static bool InternalSaveDict(const ff::Dict &dict, ff::IData **data);
static bool InternalLoadDict(ff::IDataReader *reader, ff::Dict &dict);

static bool InternalSaveValue(ff::Value *value, ff::IDataWriter *writer)
{
	assertRetVal(value && writer, false);

	DWORD type = (DWORD)value->GetType();
	if (!CanSaveValue(value))
	{
		type = (DWORD)ff::Value::Type::Null;
	}

	assertRetVal(ff::SaveData(writer, type), false);

	switch (value->GetType())
	{
	default:
		assertRetVal(false, false);

	case ff::Value::Type::Null:
		break;

	case ff::Value::Type::Bool:
		assertRetVal(ff::SaveData(writer, value->AsBool()), false);
		break;

	case ff::Value::Type::Double:
		assertRetVal(ff::SaveData(writer, value->AsDouble()), false);
		break;

	case ff::Value::Type::Float:
		assertRetVal(ff::SaveData(writer, value->AsFloat()), false);
		break;

	case ff::Value::Type::Int:
		assertRetVal(ff::SaveData(writer, value->AsInt()), false);
		break;

	case ff::Value::Type::Data:
	case ff::Value::Type::Dict:
		{
			ff::ValuePtr valueAsData;
			assertRetVal(value->Convert(ff::Value::Type::Data, &valueAsData), false);

			ff::ComPtr<ff::IData> valueData = valueAsData->AsData();
			DWORD valueSavedSize = valueData ? (DWORD)valueData->GetSize() : 0;
			DWORD valueFullSize = ff::INVALID_DWORD;

			assertRetVal(ff::SaveData(writer, valueSavedSize), false);
			assertRetVal(ff::SaveData(writer, valueFullSize), false);

			if (valueSavedSize)
			{
				assertRetVal(ff::SaveBytes(writer, valueData), false);
			}
		}
		break;

	case ff::Value::Type::SavedData:
	case ff::Value::Type::SavedDict:
		{
			ff::ComPtr<ff::ISavedData> savedData;
			assertRetVal(value->AsSavedData()->Clone(&savedData), false);

			ff::ComPtr<ff::IData> data = savedData->SaveToMem();
			assertRetVal(data, false);

			DWORD valueSavedSize = (DWORD)data->GetSize();
			DWORD valueFullSize = savedData->IsCompressed() ? (DWORD)savedData->GetFullSize() : ff::INVALID_DWORD;

			assertRetVal(ff::SaveData(writer, valueSavedSize), false);
			assertRetVal(ff::SaveData(writer, valueFullSize), false);

			if (valueSavedSize)
			{
				assertRetVal(ff::SaveBytes(writer, data), false);
			}
		}
		break;

	case ff::Value::Type::String:
		assertRetVal(ff::SaveData(writer, value->AsString()), false);
		break;

	case ff::Value::Type::Guid:
		assertRetVal(ff::SaveData(writer, value->AsGuid()), false);
		break;

	case ff::Value::Type::Point:
		assertRetVal(ff::SaveData(writer, value->AsPoint()), false);
		break;

	case ff::Value::Type::Rect:
		assertRetVal(ff::SaveData(writer, value->AsRect()), false);
		break;

	case ff::Value::Type::PointF:
		assertRetVal(ff::SaveData(writer, value->AsPointF()), false);
		break;

	case ff::Value::Type::RectF:
		assertRetVal(ff::SaveData(writer, value->AsRectF()), false);
		break;

	case ff::Value::Type::DoubleVector:
		{
			DWORD doubleCount = (DWORD)value->AsDoubleVector().Size();
			assertRetVal(ff::SaveData(writer, doubleCount), false);
				
			for (size_t h = 0; h < value->AsDoubleVector().Size(); h++)
			{
				assertRetVal(ff::SaveData(writer, value->AsDoubleVector().GetAt(h)), false);
			}
		}
		break;

	case ff::Value::Type::FloatVector:
		{
			DWORD floatCount = (DWORD)value->AsFloatVector().Size();
			assertRetVal(ff::SaveData(writer, floatCount), false);
				
			for (size_t h = 0; h < value->AsFloatVector().Size(); h++)
			{
				assertRetVal(ff::SaveData(writer, value->AsFloatVector().GetAt(h)), false);
			}
		}
		break;

	case ff::Value::Type::IntVector:
		{
			DWORD intCount = (DWORD)value->AsIntVector().Size();
			assertRetVal(ff::SaveData(writer, intCount), false);
				
			for (size_t h = 0; h < value->AsIntVector().Size(); h++)
			{
				assertRetVal(ff::SaveData(writer, value->AsIntVector().GetAt(h)), false);
			}
		}
		break;

	case ff::Value::Type::DataVector:
		{
			DWORD valueSize = (DWORD)value->AsDataVector().Size();
			assertRetVal(ff::SaveData(writer, valueSize), false);

			for (size_t h = 0; h < value->AsDataVector().Size(); h++)
			{
				ff::IData *valueData = value->AsDataVector().GetAt(h);
				valueSize = (DWORD)valueData->GetSize();
				assertRetVal(ff::SaveData(writer, valueSize), false);

				if (valueSize)
				{
					assertRetVal(SaveBytes(writer, valueData), false);
				}
			}
		}
		break;

	case ff::Value::Type::StringVector:
		{
			DWORD stringCount = (DWORD)value->AsStringVector().Size();
			assertRetVal(ff::SaveData(writer, stringCount), false);
				
			for (size_t h = 0; h < value->AsStringVector().Size(); h++)
			{
				assertRetVal(ff::SaveData(writer, value->AsStringVector().GetAt(h)), false);
			}
		}
		break;

	case ff::Value::Type::ValueVector:
		{
			DWORD valueCount = (DWORD)value->AsValueVector().Size();
			assertRetVal(ff::SaveData(writer, valueCount), false);

			for (const ff::ValuePtr &nestedValue: value->AsValueVector())
			{
				assertRetVal(InternalSaveValue(nestedValue, writer), false);
			}
		}
		break;

	case ff::Value::Type::Resource:
		{
			ff::String resName = value->AsResource()->GetName();
			assertRetVal(ff::SaveData(writer, resName), false);
		}
		break;

	case ff::Value::Type::Object:
		{
			ff::Dict objDict;
			assertRetVal(ff::SaveResource(value->AsObject(), objDict), false);

			ff::ValuePtr nestedValue;
			assertRetVal(ff::Value::CreateDict(std::move(objDict), &nestedValue), false);
			assertRetVal(InternalSaveValue(nestedValue, writer), false);
		}
		break;
	}

	return true;
}

static bool InternalLoadValue(ff::IDataReader *reader, ff::Value **value)
{
	assertRetVal(reader && value, false);

	DWORD type = 0;
	assertRetVal(ff::LoadData(reader, type), false);

	ff::Value::Type valueType = (ff::Value::Type)type;
	switch (valueType)
	{
	default:
		assertRetVal(false, false);

	case ff::Value::Type::Null:
		assertRetVal(ff::Value::CreateNull(value), false);
		break;

	case ff::Value::Type::Bool:
		{
			bool val = false;
			assertRetVal(ff::LoadData(reader, val), false);
			assertRetVal(ff::Value::CreateBool(val, value), false);
		}
		break;

	case ff::Value::Type::Double:
		{
			double val = 0;
			assertRetVal(ff::LoadData(reader, val), false);
			assertRetVal(ff::Value::CreateDouble(val, value), false);
		}
		break;

	case ff::Value::Type::Float:
		{
			float val = 0;
			assertRetVal(ff::LoadData(reader, val), false);
			assertRetVal(ff::Value::CreateFloat(val, value), false);
		}
		break;

	case ff::Value::Type::Int:
		{
			int val = 0;
			assertRetVal(ff::LoadData(reader, val), false);
			assertRetVal(ff::Value::CreateInt(val, value), false);
		}
		break;

	case ff::Value::Type::Data:
	case ff::Value::Type::Dict:
	case ff::Value::Type::SavedData:
	case ff::Value::Type::SavedDict:
		{
			DWORD savedSize = 0;
			DWORD fullSize = 0;
			assertRetVal(ff::LoadData(reader, savedSize), false);
			assertRetVal(ff::LoadData(reader, fullSize), false);

			bool compressed = (fullSize != ff::INVALID_DWORD);
			fullSize = compressed ? fullSize : savedSize;

			ff::ComPtr<ff::ISavedData> savedData;
			assertRetVal(reader->CreateSavedData(reader->GetPos(), savedSize, fullSize, compressed, &savedData), false);
			assertRetVal(ff::LoadBytes(reader, savedSize, nullptr), false);

			if (valueType == ff::Value::Type::Dict || valueType == ff::Value::Type::SavedDict)
			{
				assertRetVal(ff::Value::CreateSavedDict(savedData, value), false);
			}
			else
			{
				assertRetVal(ff::Value::CreateSavedData(savedData, value), false);
			}
		}
		break;

	case ff::Value::Type::String:
		{
			ff::String val;
			assertRetVal(ff::LoadData(reader, val), false);
			assertRetVal(ff::Value::CreateString(val, value), false);
		}
		break;

	case ff::Value::Type::Guid:
		{
			GUID val = GUID_NULL;
			assertRetVal(ff::LoadData(reader, val), false);
			assertRetVal(ff::Value::CreateGuid(val, value), false);
		}
		break;

	case ff::Value::Type::Point:
		{
			ff::PointInt val(0, 0);
			assertRetVal(ff::LoadData(reader, val), false);
			assertRetVal(ff::Value::CreatePoint(val, value), false);
		}
		break;

	case ff::Value::Type::Rect:
		{
			ff::RectInt val(0, 0, 0, 0);
			assertRetVal(ff::LoadData(reader, val), false);
			assertRetVal(ff::Value::CreateRect(val, value), false);
		}
		break;

	case ff::Value::Type::PointF:
		{
			ff::PointFloat val(0, 0);
			assertRetVal(ff::LoadData(reader, val), false);
			assertRetVal(ff::Value::CreatePointF(val, value), false);
		}
		break;

	case ff::Value::Type::RectF:
		{
			ff::RectFloat val(0, 0, 0, 0);
			assertRetVal(ff::LoadData(reader, val), false);
			assertRetVal(ff::Value::CreateRectF(val, value), false);
		}
		break;

	case ff::Value::Type::DoubleVector:
		{
			DWORD count = 0;
			assertRetVal(ff::LoadData(reader, count), false);

			ff::Vector<double> vec;
			vec.Reserve(count);

			for (size_t h = 0; h < count; h++)
			{
				double val;
				assertRetVal(ff::LoadData(reader, val), false);
				vec.Push(val);
			}

			assertRetVal(ff::Value::CreateDoubleVector(std::move(vec), value), false);
		}
		break;

	case ff::Value::Type::FloatVector:
		{
			DWORD count = 0;
			assertRetVal(ff::LoadData(reader, count), false);

			ff::Vector<float> vec;
			vec.Reserve(count);

			for (size_t h = 0; h < count; h++)
			{
				float val;
				assertRetVal(ff::LoadData(reader, val), false);
				vec.Push(val);
			}

			assertRetVal(ff::Value::CreateFloatVector(std::move(vec), value), false);
		}
		break;

	case ff::Value::Type::IntVector:
		{
			DWORD count = 0;
			assertRetVal(ff::LoadData(reader, count), false);

			ff::Vector<int> vec;
			vec.Reserve(count);

			for (size_t h = 0; h < count; h++)
			{
				int val;
				assertRetVal(ff::LoadData(reader, val), false);
				vec.Push(val);
			}

			assertRetVal(ff::Value::CreateIntVector(std::move(vec), value), false);
		}
		break;

	case ff::Value::Type::DataVector:
		{
			DWORD dataCount = 0;
			assertRetVal(ff::LoadData(reader, dataCount), false);

			ff::Vector<ff::ComPtr<ff::IData>> vec;
			vec.Reserve(dataCount);

			for (size_t h = 0; h < dataCount; h++)
			{
				DWORD dataSize = 0;
				assertRetVal(ff::LoadData(reader, dataSize), false);

				ff::ComPtr<ff::IData> valueData;
				assertRetVal(ff::LoadBytes(reader, dataSize, &valueData), false);

				vec.Push(valueData);
			}

			assertRetVal(ff::Value::CreateDataVector(std::move(vec), value), false);
		}
		break;

	case ff::Value::Type::StringVector:
		{
			DWORD dataCount = 0;
			assertRetVal(ff::LoadData(reader, dataCount), false);

			ff::Vector<ff::String> vec;
			vec.Reserve(dataCount);

			for (size_t h = 0; h < dataCount; h++)
			{
				ff::String val;
				assertRetVal(ff::LoadData(reader, val), false);
				vec.Push(val);
			}

			assertRetVal(ff::Value::CreateStringVector(std::move(vec), value), false);
		}
		break;

	case ff::Value::Type::ValueVector:
		{
			DWORD dataCount = 0;
			assertRetVal(ff::LoadData(reader, dataCount), false);

			ff::Vector<ff::ValuePtr> vec;
			vec.Reserve(dataCount);

			for (size_t h = 0; h < dataCount; h++)
			{
				ff::ValuePtr nestedValue;
				assertRetVal(InternalLoadValue(reader, &nestedValue), false);
				vec.Push(nestedValue);
			}

			assertRetVal(ff::Value::CreateValueVector(std::move(vec), value), false);
		}
		break;

	case ff::Value::Type::Resource:
		{
			ff::String val;
			assertRetVal(ff::LoadData(reader, val), false);
			val.insert(0, L"ref:");
			assertRetVal(ff::Value::CreateString(val, value), false);
		}
		break;

	case ff::Value::Type::Object:
		// Just load the dict, don't convert it into an object
		assertRetVal(InternalLoadValue(reader, value), false);
		break;
	}

	return true;
}

static bool InternalSaveDict(const ff::Dict &dict, ff::IData **data)
{
	assertRetVal(data, false);
	*data = nullptr;

	ff::ComPtr<ff::IDataVector> dataVector;
	ff::ComPtr<ff::IDataWriter> writer;
	assertRetVal(CreateDataWriter(&dataVector, &writer), false);

	ff::Vector<ff::String> names = dict.GetAllNames();
	DWORD count = (DWORD)names.Size();
	DWORD version = 0;

	assertRetVal(ff::SaveData(writer, DICT_HEADER), false);
	assertRetVal(ff::SaveData(writer, version), false);
	assertRetVal(ff::SaveData(writer, count), false);

	for (ff::StringRef name: names)
	{
		assertRetVal(ff::SaveData(writer, name), false);

		ff::Value *value = dict.GetValue(name);
		assertRetVal(InternalSaveValue(value, writer), false);
	}

	*data = dataVector.Detach();
	return true;
}

bool ff::SaveDict(const ff::Dict &dict, ff::IData **data)
{
	return InternalSaveDict(dict, data);
}

static bool InternalLoadDict(ff::IDataReader *reader, ff::Dict &dict)
{
	assertRetVal(reader, false);

	DWORD header = 0;
	DWORD version = 0;
	DWORD count = 0;

	assertRetVal(ff::LoadData(reader, header) && header == DICT_HEADER, false);
	assertRetVal(ff::LoadData(reader, version) && version <= 1, false);
	assertRetVal(ff::LoadData(reader, count), false);
	dict.Reserve(count);

	ff::StringCache emptyCache;

	for (size_t i = 0; i < count; i++)
	{
		ff::String name;
		assertRetVal(ff::LoadData(reader, name), false);

		ff::ValuePtr value;
		assertRetVal(InternalLoadValue(reader, &value), false);

		dict.SetValue(name, value);
	}

	return true;
}

bool ff::LoadDict(IDataReader *reader, Dict &dict)
{
	return InternalLoadDict(reader, dict);
}

static ff::String SanitizeString(ff::StringRef str)
{
	ff::String cleanStr = str;

	ff::ReplaceAll(cleanStr, ff::String(L"\r"), ff::String(L"\\r"));
	ff::ReplaceAll(cleanStr, ff::String(L"\n"), ff::String(L"\\n"));

	if (cleanStr.size() > 50)
	{
		cleanStr = cleanStr.substr(0, 32) + ff::String(L"...");
	}

	return cleanStr;
}

static void InternalDumpDict(const ff::Dict &dict, ff::Log &log, size_t level);

static void InternalDumpValue(ff::StringRef name, ff::ValuePtr value, ff::Log &log, size_t level)
{
	ff::String spaces(level * 4, L' ');

	if (name.size())
	{
		log.TraceF(L"%s%s: ", spaces.c_str(), name.c_str());
	}

	switch (value->GetType())
	{
	default:
		{
			ff::ValuePtr strValue;
			if (!value->Convert(ff::Value::Type::String, &strValue))
			{
				ff::Value::CreateString(ff::String::format_new(L"<unknown type: %d>", (int)value->GetType()), &strValue);
			}

			ff::String str = SanitizeString(strValue->AsString());
			log.TraceF(L"%s\r\n", str.c_str());
		}
		break;

	case ff::Value::Type::Data:
		log.TraceF(L"<data: %lu>\r\n", value->AsData()->GetSize());
		break;

	case ff::Value::Type::DataVector:
		log.TraceF(L"<datas: %lu>\r\n", value->AsDataVector().Size());
		for (size_t i = 0; i < value->AsDataVector().Size(); i++)
		{
			log.TraceF(L"%s    [%lu] <data: %lu>\r\n", spaces.c_str(), i, value->AsDataVector().GetAt(i)->GetSize());
		}
		break;

	case ff::Value::Type::DoubleVector:
		log.TraceF(L"<doubles: %lu>\r\n", value->AsDoubleVector().Size());
		for (size_t i = 0; i < value->AsDoubleVector().Size(); i++)
		{
			log.TraceF(L"%s    [%lu] %g\r\n", spaces.c_str(), i, value->AsDoubleVector().GetAt(i));
		}
		break;

	case ff::Value::Type::FloatVector:
		log.TraceF(L"<floats: %lu>\r\n", value->AsFloatVector().Size());
		for (size_t i = 0; i < value->AsFloatVector().Size(); i++)
		{
			log.TraceF(L"%s    [%lu] %g\r\n", spaces.c_str(), i, value->AsFloatVector().GetAt(i));
		}
		break;

	case ff::Value::Type::IntVector:
		log.TraceF(L"<ints: %lu>\r\n", value->AsIntVector().Size());
		for (size_t i = 0; i < value->AsIntVector().Size(); i++)
		{
			log.TraceF(L"%s    [%lu] %d\r\n", spaces.c_str(), i, value->AsIntVector().GetAt(i));
		}
		break;

	case ff::Value::Type::StringVector:
		log.TraceF(L"<doubles: %lu>\r\n", value->AsStringVector().Size());
		for (size_t i = 0; i < value->AsStringVector().Size(); i++)
		{
			ff::String str = SanitizeString(value->AsStringVector().GetAt(i));
			log.TraceF(L"%s    [%lu] %s\r\n", spaces.c_str(), i, str.c_str());
		}
		break;

	case ff::Value::Type::ValueVector:
		log.TraceF(L"<values: %lu>\r\n", value->AsValueVector().Size());
		for (size_t i = 0; i < value->AsValueVector().Size(); i++)
		{
			log.TraceF(L"%s    [%lu] ", spaces.c_str(), i);
			InternalDumpValue(ff::GetEmptyString(), value->AsValueVector().GetAt(i), log, level + 2);
		}
		break;

	case ff::Value::Type::Dict:
		log.TraceF(L"<dict: %lu>\r\n", value->AsDict().Size());
		InternalDumpDict(value->AsDict(), log, level + 1);
		break;

	case ff::Value::Type::SavedData:
		{
			size_t savedSize = value->AsSavedData()->GetSavedSize();
			size_t fullSize = value->AsSavedData()->GetFullSize();

			if (value->AsSavedData()->IsCompressed() && fullSize)
			{
				log.TraceF(L"<saved data: %lu -> %lu (%.1f%%)>\r\n", savedSize, fullSize,
					(double)savedSize / (double)fullSize * 100.0);
			}
			else
			{
				log.TraceF(L"<saved data: %lu>\r\n", fullSize);
			}
		}
		break;

	case ff::Value::Type::SavedDict:
		{
			size_t savedSize = value->AsSavedData()->GetSavedSize();
			size_t fullSize = value->AsSavedData()->GetFullSize();

			if (value->AsSavedData()->IsCompressed() && fullSize)
			{
				log.TraceF(L"<saved dict: %lu -> %lu (%.1f%%)>\r\n", savedSize, fullSize,
					(double)savedSize / (double)fullSize * 100.0);
			}
			else
			{
				log.TraceF(L"<saved dict: %lu>\r\n", fullSize);
			}

			ff::ValuePtr dictValue;
			if (value->Convert(ff::Value::Type::Dict, &dictValue))
			{
				InternalDumpDict(dictValue->AsDict(), log, level + 1);
			}
		}
		break;

	case ff::Value::Type::Resource:
		log.TraceF(L"<resource: %s>\r\n", value->AsResource()->GetName().c_str());
		break;

	case ff::Value::Type::Object:
		{
			ff::ComPtr<ff::IComObject> obj;
			if (obj.QueryFrom(value->AsObject()))
			{
				log.TraceF(L"<object: %s>\r\n", obj->GetComClassName().c_str());
			}
			else
			{
				log.Trace(L"<object>\r\n");
			}

			ff::ComPtr<ff::IResourceSave> resObj;
			if (resObj.QueryFrom(value->AsObject()))
			{
				ff::Dict resDict;
				if (resObj->SaveResource(resDict))
				{
					InternalDumpDict(resDict, log, level + 1);
				}
			}
		}
		break;
	}
}

static void InternalDumpDict(const ff::Dict &dict, ff::Log &log, size_t level)
{
	ff::Vector<ff::String> names = dict.GetAllNames(true);
	for (ff::StringRef name : names)
	{
		ff::ValuePtr value = dict.GetValue(name);
		InternalDumpValue(name, value, log, level);
	}
}

void ff::DumpDict(StringRef name, const Dict &dict, Log *log, bool debugOnly)
{
	if (!debugOnly || GetThisModule().IsDebugBuild())
	{
		Log extraLog;
		Log &realLog = log ? *log : extraLog;

		realLog.TraceF(L"-- Dict %s --\r\n", name.c_str());

		InternalDumpDict(dict, realLog, 0);

		realLog.Trace(L"-- Done --\r\n");
	}
}

void ff::DebugDumpDict(const Dict &dict)
{
	ff::DumpDict(ff::GetEmptyString(), dict, nullptr, true);
}

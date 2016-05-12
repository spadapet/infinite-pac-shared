#include "pch.h"
#include "Dict/Dict.h"
#include "Dict/JsonPersist.h"
#include "Dict/JsonTokenizer.h"
#include "Dict/Value.h"

static const size_t INDENT_SPACES = 2;

static bool JsonWriteValue(ff::Value *value, size_t spaces, ff::StringOut output);
static void JsonWriteObject(const ff::Dict &dict, size_t spaces, ff::StringOut output);
static ff::Dict ParseObject(ff::JsonTokenizer &tokenizer, const wchar_t **errorPos);
static ff::Vector<ff::ValuePtr> ParseArray(ff::JsonTokenizer &tokenizer, const wchar_t **errorPos);

static ff::ValuePtr ParseValue(ff::JsonTokenizer &tokenizer, ff::JsonToken *firstToken, const wchar_t **errorPos)
{
	ff::ValuePtr value;
	ff::JsonToken token = firstToken ? *firstToken : tokenizer.NextToken();

	if (!token.GetValue(&value))
	{
		if (token._type == ff::JsonTokenType::OpenCurly)
		{
			// Nested object
			ff::Dict valueDict = ParseObject(tokenizer, errorPos);
			if (!*errorPos && !ff::Value::CreateDict(std::move(valueDict), &value))
			{
				*errorPos = token._start;
			}
		}
		else if (token._type == ff::JsonTokenType::OpenBracket)
		{
			// Array
			ff::Vector<ff::ValuePtr> valueVector = ParseArray(tokenizer, errorPos);
			if (!*errorPos && !ff::Value::CreateValueVector(std::move(valueVector), &value))
			{
				*errorPos = token._start;
			}
		}
	}

	if (!*errorPos && !value)
	{
		*errorPos = token._start;
	}

	return value;
}

static ff::Vector<ff::ValuePtr> ParseArray(ff::JsonTokenizer &tokenizer, const wchar_t **errorPos)
{
	ff::Vector<ff::ValuePtr> values;

	for (ff::JsonToken token = tokenizer.NextToken(); token._type != ff::JsonTokenType::CloseBracket; )
	{
		ff::ValuePtr value = ParseValue(tokenizer, &token, errorPos);
		if (*errorPos)
		{
			break;
		}

		values.Push(value);

		token = tokenizer.NextToken();
		if (token._type != ff::JsonTokenType::Comma &&
			token._type != ff::JsonTokenType::CloseBracket)
		{
			*errorPos = token._start;
			break;
		}

		if (token._type == ff::JsonTokenType::Comma)
		{
			token = tokenizer.NextToken();
		}
	}

	return values;
}

static ff::Dict ParseObject(ff::JsonTokenizer &tokenizer, const wchar_t **errorPos)
{
	ff::Dict dict;

	for (ff::JsonToken token = tokenizer.NextToken(); token._type != ff::JsonTokenType::CloseCurly; )
	{
		// Pair name is first
		if (token._type != ff::JsonTokenType::String)
		{
			*errorPos = token._start;
			break;
		}

		// Get string from token
		ff::ValuePtr key;
		if (!token.GetValue(&key))
		{
			*errorPos = token._start;
			break;
		}

		// Colon must be after name
		token = tokenizer.NextToken();
		if (token._type != ff::JsonTokenType::Colon)
		{
			*errorPos = token._start;
			break;
		}

		ff::ValuePtr value = ParseValue(tokenizer, nullptr, errorPos);
		if (*errorPos)
		{
			break;
		}

		dict.SetValue(key->AsString(), value);

		token = tokenizer.NextToken();
		if (token._type != ff::JsonTokenType::Comma &&
			token._type != ff::JsonTokenType::CloseCurly)
		{
			*errorPos = token._start;
			break;
		}

		if (token._type == ff::JsonTokenType::Comma)
		{
			token = tokenizer.NextToken();
		}
	}

	return dict;
}

static ff::Dict ParseRootObject(ff::JsonTokenizer &tokenizer, const wchar_t **errorPos)
{
	ff::JsonToken token = tokenizer.NextToken();
	if (token._type == ff::JsonTokenType::OpenCurly)
	{
		return ParseObject(tokenizer, errorPos);
	}

	*errorPos = token._start;
	return ff::Dict();
}

ff::Dict ff::JsonParse(StringRef text, size_t *errorPos)
{
	JsonTokenizer tokenizer(text);

	const wchar_t *myErrorPos = nullptr;
	Dict dict = ParseRootObject(tokenizer, &myErrorPos);

	if (errorPos)
	{
		*errorPos = myErrorPos ? (myErrorPos - text.c_str()) : ff::INVALID_SIZE;
	}

	return dict;
}

static ff::String JsonEncode(ff::StringRef value)
{
	ff::String output;
	output.reserve(value.size() + 2);
	output.append(1, '\"');

	for (const wchar_t *ch = value.c_str(); *ch; ch++)
	{
		switch (*ch)
		{
		case '\"':
			output.append(L"\\\"", 2);
			break;

		case '\\':
			output.append(L"\\\\", 2);
			break;

		case '\b':
			output.append(L"\\b", 2);
			break;

		case '\f':
			output.append(L"\\f", 2);
			break;

		case '\n':
			output.append(L"\\n", 2);
			break;

		case '\r':
			output.append(L"\\r", 2);
			break;

		case '\t':
			output.append(L"\\t", 2);
			break;

		default:
			if (*ch >= ' ')
			{
				output.append(1, *ch);
			}
			break;
		}
	}

	output.append(1, '\"');
	return output;
}

static void JsonWriteArray(const ff::Vector<ff::ValuePtr> &values, size_t spaces, ff::StringOut output)
{
	output.append(1, '[');
	
	size_t size = values.Size();
	if (size)
	{
		output.append(L"\r\n", 2);

		for (size_t i = 0; i < size; i++)
		{
			// Indent
			output.append(spaces + INDENT_SPACES, ' ');

			JsonWriteValue(values[i], spaces + INDENT_SPACES, output);

			if (i + 1 < size)
			{
				output.append(1, ',');
			}

			output.append(L"\r\n", 2);
		}

		output.append(spaces, ' ');
	}

	output.append(1, ']');
}

static bool JsonWriteValue(ff::Value *value, size_t spaces, ff::StringOut output)
{
	switch (value->GetType())
	{
	case ff::Value::Type::String:
		output.append(1, ' ');
		output += JsonEncode(value->AsString());
		break;

	case ff::Value::Type::Bool:
	case ff::Value::Type::Null:
	case ff::Value::Type::Double:
	case ff::Value::Type::Int:
		{
			ff::ValuePtr strValue;
			if (value->Convert(ff::Value::Type::String, &strValue))
			{
				output.append(1, ' ');
				output += strValue->AsString();
			}
		}
		break;

	case ff::Value::Type::Dict:
	case ff::Value::Type::SavedDict:
		{
			ff::ValuePtr dictValue;
			assertRetVal(value->Convert(ff::Value::Type::Dict, &dictValue), false);

			if (dictValue->AsDict().Size())
			{
				output.append(L"\r\n", 2);
				output.append(spaces, ' ');
			}
			else
			{
				output.append(1, ' ');
			}

			JsonWriteObject(dictValue->AsDict(), spaces, output);
		}
		break;

	case ff::Value::Type::ValueVector:
		if (value->AsValueVector().Size())
		{
			output.append(L"\r\n", 2);
			output.append(spaces, ' ');
		}
		else
		{
			output.append(1, ' ');
		}

		JsonWriteArray(value->AsValueVector(), spaces, output);
		break;

	default:
		output += L" null";
		assertRetVal(false, false);
	}

	return true;
}

static void JsonWriteObject(const ff::Dict &dict, size_t spaces, ff::StringOut output)
{
	output.append(1, '{');

	ff::Vector<ff::String> names = dict.GetAllNames(true);
	if (names.Size())
	{
		output.append(L"\r\n", 2);

		for (size_t i = 0; i < names.Size(); i++)
		{
			// Indent
			output.append(spaces + INDENT_SPACES, ' ');

			// "key": value,
			ff::StringRef name = names[i];
			output.append(JsonEncode(name));
			output.append(1, ':');
			JsonWriteValue(dict.GetValue(name), spaces + INDENT_SPACES, output);

			if (i + 1 < names.Size())
			{
				output.append(1, ',');
			}

			output.append(L"\r\n", 2);
		}

		output.append(spaces, ' ');
	}

	output.append(1, '}');
}

ff::String ff::JsonWrite(const Dict &dict)
{
	String output;
	JsonWriteObject(dict, 0, output);
	return output;
}

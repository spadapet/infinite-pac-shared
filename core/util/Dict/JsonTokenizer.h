#pragma once

namespace ff
{
	class Value;

	enum class JsonTokenType
	{
		None,
		Error,
		True,
		False,
		Null,
		String,
		Number,
		Comma,
		Colon,
		OpenCurly,
		CloseCurly,
		OpenBracket,
		CloseBracket,
	};

	struct JsonToken
	{
		UTIL_API bool GetValue(Value **value) const;

		JsonTokenType _type;
		const wchar_t *_start;
		size_t _length;
	};

	class JsonTokenizer
	{
	public:
		UTIL_API JsonTokenizer(StringRef text);

		UTIL_API JsonToken NextToken();

	private:
		bool SkipString(wchar_t &ch);
		bool SkipNumber(wchar_t &ch);
		bool SkipDigits(wchar_t &ch);
		bool SkipIdentifier(wchar_t &ch);
		wchar_t SkipSpacesAndComments(wchar_t ch);
		wchar_t CurrentChar() const;
		wchar_t NextChar();
		wchar_t PeekNextChar();

		String _text;
		const wchar_t *_pos;
		const wchar_t *_end;
	};
}

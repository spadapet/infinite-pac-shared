#include "pch.h"
#include "Dict/JsonTokenizer.h"
#include "Dict/Value.h"

bool ff::JsonToken::GetValue(Value **value) const
{
	assertRetVal(value, false);
	bool status = false;

	switch (_type)
	{
	case JsonTokenType::True:
		status = Value::CreateBool(true, value);
		break;

	case JsonTokenType::False:
		status = Value::CreateBool(false, value);
		break;

	case JsonTokenType::Null:
		status = Value::CreateNull(value);
		break;

	case JsonTokenType::Number:
		{
			wchar_t *end = nullptr;
			double val = wcstod(_start, &end);

			if (end == _start + _length)
			{
				if (std::floor(val) == val && val >= INT_MIN && val <= INT_MAX)
				{
					status = Value::CreateInt((int)val, value);
				}
				else
				{
					status = Value::CreateDouble(val, value);
				}
			}
		}
		break;

	case JsonTokenType::String:
		{
			String val;
			val.reserve(_length);

			const wchar_t *cur = _start + 1;
			for (const wchar_t *end = _start + _length - 1; cur && cur < end; )
			{
				if (*cur == '\\')
				{
					switch (cur[1])
					{
					case '\"':
					case '\\':
					case '/':
						val.append(1, cur[1]);
						cur += 2;
						break;

					case 'b':
						val.append(1, '\b');
						cur += 2;
						break;

					case 'f':
						val.append(1, '\f');
						cur += 2;
						break;

					case 'n':
						val.append(1, '\n');
						cur += 2;
						break;

					case 'r':
						val.append(1, '\r');
						cur += 2;
						break;

					case 't':
						val.append(1, '\t');
						cur += 2;
						break;

					case 'u':
						{
							wchar_t buffer[5] = { cur[2], cur[3], cur[4], cur[5], '\0' };
							wchar_t *stopped = nullptr;
							unsigned long decoded = wcstoul(buffer, &stopped, 16);
							if (!*stopped)
							{
								val.append(1, (wchar_t)(decoded & 0xFFFF));
								cur += 6;
							}
							else
							{
								cur = nullptr;
							}
						}
						break;

					default:
						cur = nullptr;
						break;
					}
				}
				else
				{
					val.append(1, *cur);
					cur++;
				}
			}

			status = cur && Value::CreateString(val, value);
		}
		break;
	}

	return status;
}

ff::JsonTokenizer::JsonTokenizer(StringRef text)
	: _text(text)
	, _pos(text.c_str())
	, _end(text.c_str() + text.size())
{
}

ff::JsonToken ff::JsonTokenizer::NextToken()
{
	wchar_t ch = SkipSpacesAndComments(CurrentChar());
	JsonTokenType type = JsonTokenType::Error;
	const wchar_t *start = _pos;

	switch (ch)
	{
	case '\0':
		type = JsonTokenType::None;
		break;

	case 't':
		if (SkipIdentifier(ch) &&
			_pos - start == 4 &&
			start[1] == 'r' &&
			start[2] == 'u' &&
			start[3] == 'e')
		{
			type = JsonTokenType::True;
		}
		break;

	case 'f':
		if (SkipIdentifier(ch) &&
			_pos - start == 5 &&
			start[1] == 'a' &&
			start[2] == 'l' &&
			start[3] == 's' &&
			start[4] == 'e')
		{
			type = JsonTokenType::False;
		}
		break;

	case 'n':
		if (SkipIdentifier(ch) &&
			_pos - start == 4 &&
			start[1] == 'u' &&
			start[2] == 'l' &&
			start[3] == 'l')
		{
			type = JsonTokenType::Null;
		}
		break;

	case '\"':
		if (SkipString(ch))
		{
			type = JsonTokenType::String;
		}
		break;

	case ',':
		_pos++;
		type = JsonTokenType::Comma;
		break;

	case ':':
		_pos++;
		type = JsonTokenType::Colon;
		break;

	case '{':
		_pos++;
		type = JsonTokenType::OpenCurly;
		break;

	case '}':
		_pos++;
		type = JsonTokenType::CloseCurly;
		break;

	case '[':
		_pos++;
		type = JsonTokenType::OpenBracket;
		break;

	case ']':
		_pos++;
		type = JsonTokenType::CloseBracket;
		break;

	case '-':
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		if (SkipNumber(ch))
		{
			type = JsonTokenType::Number;
		}
		break;
	}

	return JsonToken { type, start, (size_t)(_pos - start) };
}

bool ff::JsonTokenizer::SkipString(wchar_t &ch)
{
	if (ch != '\"')
	{
		return false;
	}

	ch = NextChar();

	while (true)
	{
		if (ch == '\"')
		{
			_pos++;
			break;
		}
		else if (ch == '\\')
		{
			ch = NextChar();

			switch (ch)
			{
			case '\"':
			case '\\':
			case '/':
			case 'b':
			case 'f':
			case 'n':
			case 'r':
			case 't':
				ch = NextChar();
				break;

			case 'u':
				if (_pos > _end - 5)
				{
					return false;
				}

				if (!iswxdigit(_pos[1]) ||
					!iswxdigit(_pos[2]) ||
					!iswxdigit(_pos[3]) ||
					!iswxdigit(_pos[4]))
				{
					return false;
				}

				_pos += 5;
				ch = CurrentChar();
				break;

			default:
				return false;
			}
		}
		else if (ch < ' ')
		{
			return false;
		}
		else
		{
			ch = NextChar();
		}
	}

	return true;
}

bool ff::JsonTokenizer::SkipNumber(wchar_t &ch)
{
	if (ch == '-')
	{
		ch = NextChar();
	}

	if (!SkipDigits(ch))
	{
		return false;
	}

	if (ch == '.')
	{
		ch = NextChar();

		if (!SkipDigits(ch))
		{
			return false;
		}
	}

	if (ch == 'e' || ch == 'E')
	{
		ch = NextChar();

		if (ch == '-' || ch == '+')
		{
			ch = NextChar();
		}

		if (!SkipDigits(ch))
		{
			return false;
		}
	}

	return true;
}

bool ff::JsonTokenizer::SkipDigits(wchar_t &ch)
{
	if (!iswdigit(ch))
	{
		return false;
	}

	do 
	{
		ch = NextChar();
	}
	while (iswdigit(ch));

	return true;
}

bool ff::JsonTokenizer::SkipIdentifier(wchar_t &ch)
{
	if (!iswalpha(ch))
	{
		return false;
	}

	do 
	{
		ch = NextChar();
	}
	while (iswalnum(ch));

	return true;
}

wchar_t ff::JsonTokenizer::SkipSpacesAndComments(wchar_t ch)
{
	while (true)
	{
		if (iswspace(ch))
		{
			ch = NextChar();
		}
		else if (ch == '/')
		{
			wchar_t ch2 = PeekNextChar();

			if (ch2 == '/')
			{
				_pos++;
				ch = NextChar();

				while (ch && ch != '\r' && ch != '\n')
				{
					ch = NextChar();
				}
			}
			else if (ch2 == '*')
			{
				const wchar_t *start = _pos++;
				ch = NextChar();

				while (ch && (ch != '*' || PeekNextChar() != '/'))
				{
					ch = NextChar();
				}

				if (!ch)
				{
					// No end for the comment
					_pos = start;
					ch = '/';
					break;
				}

				// Skip the end of comment
				_pos++;
				ch = NextChar();
			}
			else
			{
				break;
			}
		}
		else
		{
			break;
		}
	}

	return ch;
}

wchar_t ff::JsonTokenizer::CurrentChar() const
{
	return _pos < _end ? *_pos : '\0';
}

wchar_t ff::JsonTokenizer::NextChar()
{
	return ++_pos < _end ? *_pos : '\0';
}

wchar_t ff::JsonTokenizer::PeekNextChar()
{
	return _pos < _end - 1 ? _pos[1] : '\0';
}

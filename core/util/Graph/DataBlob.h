#pragma once

namespace DirectX
{
	class Blob;
};

namespace ff
{
	class IData;

	UTIL_API bool CreateDataFromBlob(ID3DBlob *blob, IData **obj);
	UTIL_API bool CreateDataFromBlob(DirectX::Blob &&blob, IData **obj);
}

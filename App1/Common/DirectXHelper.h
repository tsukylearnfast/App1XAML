#pragma once

#include <ppltasks.h>	// Pour create_task

namespace DX
{
	inline void ThrowIfFailed(HRESULT hr)
	{
		if (FAILED(hr))
		{
			// Définir un point d'arrêt sur cette ligne pour intercepter les erreurs d'API Win32.
			throw Platform::Exception::CreateException(hr);
		}
	}

	// Fonction qui lit les données d'un fichier binaire de manière asynchrone.
	inline Concurrency::task<std::vector<byte>> ReadDataAsync(const std::wstring& filename)
	{
		using namespace Windows::Storage;
		using namespace Concurrency;

		auto folder = Windows::ApplicationModel::Package::Current->InstalledLocation;

		return create_task(folder->GetFileAsync(Platform::StringReference(filename.c_str()))).then([] (StorageFile^ file) 
		{
			return FileIO::ReadBufferAsync(file);
		}).then([] (Streams::IBuffer^ fileBuffer) -> std::vector<byte> 
		{
			std::vector<byte> returnBuffer;
			returnBuffer.resize(fileBuffer->Length);
			Streams::DataReader::FromBuffer(fileBuffer)->ReadBytes(Platform::ArrayReference<byte>(returnBuffer.data(), fileBuffer->Length));
			return returnBuffer;
		});
	}

	// Convertit une longueur en pixels indépendants du périphérique (DIP) en longueur en pixels physiques.
	inline float ConvertDipsToPixels(float dips, float dpi)
	{
		static const float dipsPerInch = 96.0f;
		return floorf(dips * dpi / dipsPerInch + 0.5f); // Arrondir à l’entier le plus proche.
	}

#if defined(_DEBUG)
	// Vérifier la prise en charge de la couche SDK.
	inline bool SdkLayersAvailable()
	{
		HRESULT hr = D3D11CreateDevice(
			nullptr,
			D3D_DRIVER_TYPE_NULL,       // Il n’est pas nécessaire de créer un périphérique matériel réel.
			0,
			D3D11_CREATE_DEVICE_DEBUG,  // Vérifier les couches SDK.
			nullptr,                    // N’importe quel niveau de fonctionnalité fera l’affaire.
			0,
			D3D11_SDK_VERSION,          // Définissez toujours ce paramètre sur D3D11_SDK_VERSION pour les applications Microsoft Store.
			nullptr,                    // Inutile de conserver la référence du périphérique D3D.
			nullptr,                    // Il n'est pas nécessaire de connaître le niveau de fonctionnalité.
			nullptr                     // Inutile de conserver la référence de contexte du périphérique D3D.
			);

		return SUCCEEDED(hr);
	}
#endif
}

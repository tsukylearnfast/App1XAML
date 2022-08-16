#pragma once

#include <string>
#include "..\Common\DeviceResources.h"
#include "..\Common\StepTimer.h"

namespace App1
{
	// Affiche la valeur FPS actuelle dans l’angle inférieur droit de l’écran à l’aide de Direct2D et DirectWrite.
	class SampleFpsTextRenderer
	{
	public:
		SampleFpsTextRenderer(const std::shared_ptr<DX::DeviceResources>& deviceResources);
		void CreateDeviceDependentResources();
		void ReleaseDeviceDependentResources();
		void Update(DX::StepTimer const& timer);
		void Render();

	private:
		// Pointeur mis en cache vers les ressources du périphérique.
		std::shared_ptr<DX::DeviceResources> m_deviceResources;

		// Ressources liées au rendu du texte.
		std::wstring                                    m_text;
		DWRITE_TEXT_METRICS	                            m_textMetrics;
		Microsoft::WRL::ComPtr<ID2D1SolidColorBrush>    m_whiteBrush;
		Microsoft::WRL::ComPtr<ID2D1DrawingStateBlock1> m_stateBlock;
		Microsoft::WRL::ComPtr<IDWriteTextLayout3>      m_textLayout;
		Microsoft::WRL::ComPtr<IDWriteTextFormat2>      m_textFormat;
	};
}
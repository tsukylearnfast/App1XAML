//
// DirectXPage.xaml.h
// Déclaration de la classe DirectXPage.
//

#pragma once

#include "DirectXPage.g.h"

#include "Common\DeviceResources.h"
#include "App1Main.h"

namespace App1
{
	/// <summary>
	/// Page qui héberge un DirectX SwapChainPanel.
	/// </summary>
	public ref class DirectXPage sealed
	{
	public:
		DirectXPage();
		virtual ~DirectXPage();

		void SaveInternalState(Windows::Foundation::Collections::IPropertySet^ state);
		void LoadInternalState(Windows::Foundation::Collections::IPropertySet^ state);

	private:
		// Gestionnaire d'événements de rendu de bas niveau XAML.
		void OnRendering(Platform::Object^ sender, Platform::Object^ args);

		// Gestionnaires d’événements de fenêtre.
		void OnVisibilityChanged(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::VisibilityChangedEventArgs^ args);

		// Gestionnaires d’événements DisplayInformation.
		void OnDpiChanged(Windows::Graphics::Display::DisplayInformation^ sender, Platform::Object^ args);
		void OnOrientationChanged(Windows::Graphics::Display::DisplayInformation^ sender, Platform::Object^ args);
		void OnDisplayContentsInvalidated(Windows::Graphics::Display::DisplayInformation^ sender, Platform::Object^ args);

		// Autres gestionnaires d'événements.
		void AppBarButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void OnCompositionScaleChanged(Windows::UI::Xaml::Controls::SwapChainPanel^ sender, Object^ args);
		void OnSwapChainPanelSizeChanged(Platform::Object^ sender, Windows::UI::Xaml::SizeChangedEventArgs^ e);

		// Suivez notre entrée indépendante sur un thread de processus de travail d'arrière-plan.
		Windows::Foundation::IAsyncAction^ m_inputLoopWorker;
		Windows::UI::Core::CoreIndependentInputSource^ m_coreInput;

		// Fonctions de gestion des entrées indépendantes.
		void OnPointerPressed(Platform::Object^ sender, Windows::UI::Core::PointerEventArgs^ e);
		void OnPointerMoved(Platform::Object^ sender, Windows::UI::Core::PointerEventArgs^ e);
		void OnPointerReleased(Platform::Object^ sender, Windows::UI::Core::PointerEventArgs^ e);

		// Ressources utilisées pour afficher le contenu DirectX dans l'arrière-plan de la page XAML.
		std::shared_ptr<DX::DeviceResources> m_deviceResources;
		std::unique_ptr<App1Main> m_main; 
		bool m_windowVisible;
	};
}


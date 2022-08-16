﻿//
// App.xaml.h
// Déclaration de la classe App.
//

#pragma once

#include "App.g.h"
#include "DirectXPage.xaml.h"

namespace App1
{
		/// <summary>
	/// Fournit un comportement spécifique à l'application afin de compléter la classe Application par défaut.
	/// </summary>
	ref class App sealed
	{
	public:
		App();
		virtual void OnLaunched(Windows::ApplicationModel::Activation::LaunchActivatedEventArgs^ e) override;

	private:
		void OnSuspending(Platform::Object^ sender, Windows::ApplicationModel::SuspendingEventArgs^ e);
		void OnResuming(Platform::Object ^sender, Platform::Object ^args);
		void OnNavigationFailed(Platform::Object ^sender, Windows::UI::Xaml::Navigation::NavigationFailedEventArgs ^e);
		DirectXPage^ m_directXPage;
	};
}

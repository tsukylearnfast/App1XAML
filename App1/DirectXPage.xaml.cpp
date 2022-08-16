//
// DirectXPage.xaml.cpp
// Implémentation de la classe DirectXPage.
//

#include "pch.h"
#include "DirectXPage.xaml.h"

using namespace App1;

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Graphics::Display;
using namespace Windows::System::Threading;
using namespace Windows::UI::Core;
using namespace Windows::UI::Input;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;
using namespace concurrency;

DirectXPage::DirectXPage():
	m_windowVisible(true),
	m_coreInput(nullptr)
{
	InitializeComponent();

	// Inscrivez les gestionnaires d'événements pour le cycle de vie de la page.
	CoreWindow^ window = Window::Current->CoreWindow;

	window->VisibilityChanged +=
		ref new TypedEventHandler<CoreWindow^, VisibilityChangedEventArgs^>(this, &DirectXPage::OnVisibilityChanged);

	DisplayInformation^ currentDisplayInformation = DisplayInformation::GetForCurrentView();

	currentDisplayInformation->DpiChanged +=
		ref new TypedEventHandler<DisplayInformation^, Object^>(this, &DirectXPage::OnDpiChanged);

	currentDisplayInformation->OrientationChanged +=
		ref new TypedEventHandler<DisplayInformation^, Object^>(this, &DirectXPage::OnOrientationChanged);

	DisplayInformation::DisplayContentsInvalidated +=
		ref new TypedEventHandler<DisplayInformation^, Object^>(this, &DirectXPage::OnDisplayContentsInvalidated);

	swapChainPanel->CompositionScaleChanged += 
		ref new TypedEventHandler<SwapChainPanel^, Object^>(this, &DirectXPage::OnCompositionScaleChanged);

	swapChainPanel->SizeChanged +=
		ref new SizeChangedEventHandler(this, &DirectXPage::OnSwapChainPanelSizeChanged);

	// À ce stade, nous avons accès au périphérique. 
	// Nous pouvons créer les ressources dépendantes du périphérique.
	m_deviceResources = std::make_shared<DX::DeviceResources>();
	m_deviceResources->SetSwapChainPanel(swapChainPanel);

	// Inscrire notre SwapChainPanel pour obtenir les événements de pointeur d'entrée indépendante
	auto workItemHandler = ref new WorkItemHandler([this] (IAsyncAction ^)
	{
		// Le CoreIndependentInputSource déclenchera des événements de pointeur pour les types de périphériques spécifiés sur lesquels le thread est créé.
		m_coreInput = swapChainPanel->CreateCoreIndependentInputSource(
			Windows::UI::Core::CoreInputDeviceTypes::Mouse |
			Windows::UI::Core::CoreInputDeviceTypes::Touch |
			Windows::UI::Core::CoreInputDeviceTypes::Pen
			);

		// S'abonner aux événements de pointeur, qui seront déclenchés sur le thread d'arrière-plan.
		m_coreInput->PointerPressed += ref new TypedEventHandler<Object^, PointerEventArgs^>(this, &DirectXPage::OnPointerPressed);
		m_coreInput->PointerMoved += ref new TypedEventHandler<Object^, PointerEventArgs^>(this, &DirectXPage::OnPointerMoved);
		m_coreInput->PointerReleased += ref new TypedEventHandler<Object^, PointerEventArgs^>(this, &DirectXPage::OnPointerReleased);

		// Commencer à traiter les messages d'entrée lors de leur remise.
		m_coreInput->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessUntilQuit);
	});

	// Exécuter la tâche sur un thread d'arrière-plan de haute priorité dédié.
	m_inputLoopWorker = ThreadPool::RunAsync(workItemHandler, WorkItemPriority::High, WorkItemOptions::TimeSliced);

	m_main = std::unique_ptr<App1Main>(new App1Main(m_deviceResources));
	m_main->StartRenderLoop();
}

DirectXPage::~DirectXPage()
{
	// Arrêter le rendu et le traitement des événements lors de la destruction.
	m_main->StopRenderLoop();
	m_coreInput->Dispatcher->StopProcessEvents();
}

// Enregistre l'état actuel de l'application pour les événements de suspension et de fin.
void DirectXPage::SaveInternalState(IPropertySet^ state)
{
	critical_section::scoped_lock lock(m_main->GetCriticalSection());
	m_deviceResources->Trim();

	// Arrêter le rendu lors de l'interruption de l'application.
	m_main->StopRenderLoop();

	// Ajoutez du code pour enregistrer l'état de l'application ici.
}

// Charge l'état actuel de l'application pour les événements de reprise.
void DirectXPage::LoadInternalState(IPropertySet^ state)
{
	// Ajoutez du code pour charger l'état de l'application ici.

	// Démarrer le rendu lors de la reprise de l'application.
	m_main->StartRenderLoop();
}

// Gestionnaires d’événements de fenêtre.

void DirectXPage::OnVisibilityChanged(CoreWindow^ sender, VisibilityChangedEventArgs^ args)
{
	m_windowVisible = args->Visible;
	if (m_windowVisible)
	{
		m_main->StartRenderLoop();
	}
	else
	{
		m_main->StopRenderLoop();
	}
}

// Gestionnaires d’événements DisplayInformation.

void DirectXPage::OnDpiChanged(DisplayInformation^ sender, Object^ args)
{
	critical_section::scoped_lock lock(m_main->GetCriticalSection());
	// Remarque : la valeur de LogicalDpi récupérée ici peut ne pas correspondre à la résolution effective de l'application
	// si elle est mise à l'échelle pour les appareils haute résolution. Une fois la valeur PPP définie sur DeviceResources,
	// vous devez toujours la récupérer à l'aide de la méthode GetDpi.
	// Consultez DeviceResources.cpp pour plus d'informations.
	m_deviceResources->SetDpi(sender->LogicalDpi);
	m_main->CreateWindowSizeDependentResources();
}

void DirectXPage::OnOrientationChanged(DisplayInformation^ sender, Object^ args)
{
	critical_section::scoped_lock lock(m_main->GetCriticalSection());
	m_deviceResources->SetCurrentOrientation(sender->CurrentOrientation);
	m_main->CreateWindowSizeDependentResources();
}

void DirectXPage::OnDisplayContentsInvalidated(DisplayInformation^ sender, Object^ args)
{
	critical_section::scoped_lock lock(m_main->GetCriticalSection());
	m_deviceResources->ValidateDevice();
}

// Appelé lors d'un clic sur la barre d'applications.
void DirectXPage::AppBarButton_Click(Object^ sender, RoutedEventArgs^ e)
{
	// Utilisez la barre d'applications si c'est approprié pour votre application. Concevez la barre d'applications, 
	// puis indiquez les gestionnaires d'événements (comme celui-ci).
}

void DirectXPage::OnPointerPressed(Object^ sender, PointerEventArgs^ e)
{
	// Lors de l'appui sur le pointeur, commencer le suivi du mouvement du pointeur.
	m_main->StartTracking();
}

void DirectXPage::OnPointerMoved(Object^ sender, PointerEventArgs^ e)
{
	// Mettre à jour le code de suivi du pointeur.
	if (m_main->IsTracking())
	{
		m_main->TrackingUpdate(e->CurrentPoint->Position.X);
	}
}

void DirectXPage::OnPointerReleased(Object^ sender, PointerEventArgs^ e)
{
	// Arrêter le suivi du mouvement du pointeur lorsque le pointeur est relâché.
	m_main->StopTracking();
}

void DirectXPage::OnCompositionScaleChanged(SwapChainPanel^ sender, Object^ args)
{
	critical_section::scoped_lock lock(m_main->GetCriticalSection());
	m_deviceResources->SetCompositionScale(sender->CompositionScaleX, sender->CompositionScaleY);
	m_main->CreateWindowSizeDependentResources();
}

void DirectXPage::OnSwapChainPanelSizeChanged(Object^ sender, SizeChangedEventArgs^ e)
{
	critical_section::scoped_lock lock(m_main->GetCriticalSection());
	m_deviceResources->SetLogicalSize(e->NewSize);
	m_main->CreateWindowSizeDependentResources();
}

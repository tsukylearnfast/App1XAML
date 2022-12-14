//
// App.xaml.cpp
// Implémentation de la classe App.
//

#include "pch.h"
#include "DirectXPage.xaml.h"

using namespace App1;

using namespace Platform;
using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Storage;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Interop;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;
/// <summary>
/// Initialise l'objet d'application de singleton.  Il s'agit de la première ligne du code créé
/// à être exécutée. Elle correspond donc à l'équivalent logique de main() ou WinMain().
/// </summary>
App::App()
{
	InitializeComponent();
	Suspending += ref new SuspendingEventHandler(this, &App::OnSuspending);
	Resuming += ref new EventHandler<Object^>(this, &App::OnResuming);
}

/// <summary>
/// Invoqué lorsque l'application est lancée normalement par l'utilisateur final.  D'autres points d'entrée
/// sont utilisés lorsque l'application est lancée pour ouvrir un fichier spécifique, pour afficher
/// des résultats de recherche, etc.
/// </summary>
/// <param name="e">Détails concernant la requête et le processus de lancement.</param>
void App::OnLaunched(Windows::ApplicationModel::Activation::LaunchActivatedEventArgs^ e)
{
#if _DEBUG
	if (IsDebuggerPresent())
	{
		DebugSettings->EnableFrameRateCounter = true;
	}
#endif

	auto rootFrame = dynamic_cast<Frame^>(Window::Current->Content);

	// Ne répétez pas l'initialisation de l'application lorsque la fenêtre comporte déjà du contenu,
	// assurez-vous juste que la fenêtre est active
	if (rootFrame == nullptr)
	{
		// Crée un frame qui joue le rôle de contexte de navigation et l'associe à
		// une clé SuspensionManager
		rootFrame = ref new Frame();

		rootFrame->NavigationFailed += ref new Windows::UI::Xaml::Navigation::NavigationFailedEventHandler(this, &App::OnNavigationFailed);

		// Placez le frame dans la fenêtre active
		Window::Current->Content = rootFrame;
	}

	if (rootFrame->Content == nullptr)
	{
		// Quand la pile de navigation n'est pas restaurée, accédez à la première page,
		// puis configurez la nouvelle page en transmettant les informations requises en tant que
		// paramètre
		rootFrame->Navigate(TypeName(DirectXPage::typeid), e->Arguments);
	}

	if (m_directXPage == nullptr)
	{
		m_directXPage = dynamic_cast<DirectXPage^>(rootFrame->Content);
	}

	if (e->PreviousExecutionState == ApplicationExecutionState::Terminated)
	{
		m_directXPage->LoadInternalState(ApplicationData::Current->LocalSettings->Values);
	}
	
	// Vérifiez que la fenêtre actuelle est active
	Window::Current->Activate();
}
/// <summary>
/// Appelé lorsque l'exécution de l'application est suspendue.  L'état de l'application est enregistré
/// sans savoir si l'application pourra se fermer ou reprendre sans endommager
/// le contenu de la mémoire.
/// </summary>
/// <param name="sender">Source de la requête de suspension.</param>
/// <param name="e">Détails de la requête de suspension.</param>
void App::OnSuspending(Object^ sender, SuspendingEventArgs^ e)
{
	(void) sender;	// Paramètre non utilisé
	(void) e;	// Paramètre non utilisé

	m_directXPage->SaveInternalState(ApplicationData::Current->LocalSettings->Values);
}

/// <summary>
/// Appelé lors de la reprise de l'exécution de l'application.
/// </summary>
/// <param name="sender">Source de la requête de reprise.</param>
/// <param name="args">Détails relatifs à la requête de reprise.</param>
void App::OnResuming(Object ^sender, Object ^args)
{
	(void) sender; // Paramètre non utilisé
	(void) args; // Paramètre non utilisé

	m_directXPage->LoadInternalState(ApplicationData::Current->LocalSettings->Values);
}

/// <summary>
/// Appelé lorsque la navigation vers une page donnée échoue
/// </summary>
/// <param name="sender">Frame à l'origine de l'échec de navigation.</param>
/// <param name="e">Détails relatifs à l'échec de navigation</param>
void App::OnNavigationFailed(Platform::Object ^sender, Windows::UI::Xaml::Navigation::NavigationFailedEventArgs ^e)
{
	throw ref new FailureException("Failed to load Page " + e->SourcePageType.Name);
}


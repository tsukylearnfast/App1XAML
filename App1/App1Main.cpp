#include "pch.h"
#include "App1Main.h"
#include "Common\DirectXHelper.h"

using namespace App1;
using namespace Windows::Foundation;
using namespace Windows::System::Threading;
using namespace Concurrency;

// Charge et initialise les actifs de l'application lors de son chargement.
App1Main::App1Main(const std::shared_ptr<DX::DeviceResources>& deviceResources) :
	m_deviceResources(deviceResources), m_pointerLocationX(0.0f)
{
	// S'inscrire pour recevoir une notification si le périphérique est perdu ou recréé
	m_deviceResources->RegisterDeviceNotify(this);

	// TODO: remplacez ceci par l'initialisation du contenu de votre application.
	m_sceneRenderer = std::unique_ptr<Sample3DSceneRenderer>(new Sample3DSceneRenderer(m_deviceResources));

	m_fpsTextRenderer = std::unique_ptr<SampleFpsTextRenderer>(new SampleFpsTextRenderer(m_deviceResources));

	// TODO: modifiez les paramètres de minuterie si vous voulez utiliser autre chose que le mode timestep variable par défaut.
	// par exemple, pour une logique de mise à jour timestep fixe de 60 FPS, appelez :
	/*
	m_timer.SetFixedTimeStep(true) ;
	m_timer.SetTargetElapsedSeconds(1.0 / 60) ;
	*/
}

App1Main::~App1Main()
{
	// Annuler l'inscription de la notification de périphériques
	m_deviceResources->RegisterDeviceNotify(nullptr);
}

// Met à jour l'état de l'application lorsque la taille de la fenêtre change (par exemple modification de l'orientation du périphérique)
void App1Main::CreateWindowSizeDependentResources() 
{
	// TODO: remplacez ceci par l'initialisation du contenu de votre application en fonction de la taille.
	m_sceneRenderer->CreateWindowSizeDependentResources();
}

void App1Main::StartRenderLoop()
{
	// Si la boucle de rendu de l'animation est déjà exécutée, ne lancez pas un autre thread.
	if (m_renderLoopWorker != nullptr && m_renderLoopWorker->Status == AsyncStatus::Started)
	{
		return;
	}

	// Crée une tâche qui sera exécutée sur un thread d'arrière-plan.
	auto workItemHandler = ref new WorkItemHandler([this](IAsyncAction ^ action)
	{
		// Calcule le frame et le rendu mis à jour une fois par intervalle de suppression vertical.
		while (action->Status == AsyncStatus::Started)
		{
			critical_section::scoped_lock lock(m_criticalSection);
			Update();
			if (Render())
			{
				m_deviceResources->Present();
			}
		}
	});

	// Exécuter la tâche sur un thread d'arrière-plan de haute priorité dédié.
	m_renderLoopWorker = ThreadPool::RunAsync(workItemHandler, WorkItemPriority::High, WorkItemOptions::TimeSliced);
}

void App1Main::StopRenderLoop()
{
	m_renderLoopWorker->Cancel();
}

// Met à jour l'état de l'application une fois par frame.
void App1Main::Update() 
{
	ProcessInput();

	// Mettre à jour les objets de scène.
	m_timer.Tick([&]()
	{
		// TODO: remplacez ceci par vos fonctions de mise à jour de contenu d'application.
		m_sceneRenderer->Update(m_timer);
		m_fpsTextRenderer->Update(m_timer);
	});
}

// Traiter toutes les saisies utilisateur avant de mettre à jour l'état du jeu
void App1Main::ProcessInput()
{
	// TODO: Ajouter la gestion de saisie par frame ici.
	m_sceneRenderer->TrackingUpdate(m_pointerLocationX);
}

// Affiche le frame actuel en fonction de l'état actuel de l'application.
// Retourne true si le frame a été rendu et est prêt à être affiché.
bool App1Main::Render() 
{
	// N'essayez d'afficher aucun élément avant la première mise à jour.
	if (m_timer.GetFrameCount() == 0)
	{
		return false;
	}

	auto context = m_deviceResources->GetD3DDeviceContext();

	// Réinitialisez la fenêtre d'affichage de manière à ce qu'elle cible la totalité de l'écran.
	auto viewport = m_deviceResources->GetScreenViewport();
	context->RSSetViewports(1, &viewport);

	// Réinitialiser les cibles de rendu à l’écran.
	ID3D11RenderTargetView *const targets[1] = { m_deviceResources->GetBackBufferRenderTargetView() };
	context->OMSetRenderTargets(1, targets, m_deviceResources->GetDepthStencilView());

	// Effacer la mémoire tampon d’arrière-plan et la vue du stencil de profondeur.
	context->ClearRenderTargetView(m_deviceResources->GetBackBufferRenderTargetView(), DirectX::Colors::CornflowerBlue);
	context->ClearDepthStencilView(m_deviceResources->GetDepthStencilView(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	// Afficher les objets de scène.
	// TODO: remplacez ceci par vos fonctions de rendu de contenu d'application.
	m_sceneRenderer->Render();
	m_fpsTextRenderer->Render();

	return true;
}

// Avertit les convertisseurs que les ressources du périphérique doivent être libérées.
void App1Main::OnDeviceLost()
{
	m_sceneRenderer->ReleaseDeviceDependentResources();
	m_fpsTextRenderer->ReleaseDeviceDependentResources();
}

// Avertit les convertisseurs que les ressources du périphérique seront peut-être recréées.
void App1Main::OnDeviceRestored()
{
	m_sceneRenderer->CreateDeviceDependentResources();
	m_fpsTextRenderer->CreateDeviceDependentResources();
	CreateWindowSizeDependentResources();
}

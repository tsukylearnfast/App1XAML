#include "pch.h"
#include "DeviceResources.h"
#include "DirectXHelper.h"
#include <windows.ui.xaml.media.dxinterop.h>

using namespace D2D1;
using namespace DirectX;
using namespace Microsoft::WRL;
using namespace Windows::Foundation;
using namespace Windows::Graphics::Display;
using namespace Windows::UI::Core;
using namespace Windows::UI::Xaml::Controls;
using namespace Platform;

namespace DisplayMetrics
{
	// Les affichages haute résolution peuvent demander une grande puissance de GPU et de batterie pour le rendu.
	// Les téléphones haute résolution, par exemple, peuvent être affectés par une faible autonomie de la batterie si
	// les jeux tentent de restituer 60 frames par seconde avec une fidélité optimale.
	// La décision de restituer avec une fidélité optimale pour l'ensemble des plateformes et des formats
	// doit être délibérée.
	static const bool SupportHighResolutions = false;

	// Seuils par défaut qui définissent un affichage "haute résolution". Si les seuils
	// sont dépassés et que SupportHighResolutions a la valeur false, les dimensions sont réduites
	// de 50 %.
	static const float DpiThreshold = 192.0f;		// 200 % de l'affichage standard d'un ordinateur de bureau.
	static const float WidthThreshold = 1920.0f;	// 1080p de largeur.
	static const float HeightThreshold = 1080.0f;	// 1080p de hauteur.
};

// Constantes utilisées pour calculer les rotations d'écran.
namespace ScreenRotation
{
	// Rotation Z - 0 degré
	static const XMFLOAT4X4 Rotation0( 
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
		);

	// Rotation Z - 90 degrés
	static const XMFLOAT4X4 Rotation90(
		0.0f, 1.0f, 0.0f, 0.0f,
		-1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
		);

	// Rotation Z - 180 degrés
	static const XMFLOAT4X4 Rotation180(
		-1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, -1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
		);

	// Rotation Z - 270 degrés
	static const XMFLOAT4X4 Rotation270( 
		0.0f, -1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
		);
};

// Constructeur pour DeviceResources.
DX::DeviceResources::DeviceResources() : 
	m_screenViewport(),
	m_d3dFeatureLevel(D3D_FEATURE_LEVEL_9_1),
	m_d3dRenderTargetSize(),
	m_outputSize(),
	m_logicalSize(),
	m_nativeOrientation(DisplayOrientations::None),
	m_currentOrientation(DisplayOrientations::None),
	m_dpi(-1.0f),
	m_effectiveDpi(-1.0f),
	m_compositionScaleX(1.0f),
	m_compositionScaleY(1.0f),
	m_deviceNotify(nullptr)
{
	CreateDeviceIndependentResources();
	CreateDeviceResources();
}

// Configure les ressources qui ne dépendent pas du périphérique Direct3D.
void DX::DeviceResources::CreateDeviceIndependentResources()
{
	// Initialiser les ressources Direct2D.
	D2D1_FACTORY_OPTIONS options;
	ZeroMemory(&options, sizeof(D2D1_FACTORY_OPTIONS));

#if defined(_DEBUG)
	// Si le projet se trouve dans une version Debug, activer le débogage Direct2D via les couches SDK.
	options.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#endif

	// Initialiser la fabrique Direct2D.
	DX::ThrowIfFailed(
		D2D1CreateFactory(
			D2D1_FACTORY_TYPE_SINGLE_THREADED,
			__uuidof(ID2D1Factory3),
			&options,
			&m_d2dFactory
			)
		);

	// Initialiser la fabrique DirectWrite.
	DX::ThrowIfFailed(
		DWriteCreateFactory(
			DWRITE_FACTORY_TYPE_SHARED,
			__uuidof(IDWriteFactory3),
			&m_dwriteFactory
			)
		);

	// Initialiser la fabrique Windows Imaging Component (WIC).
	DX::ThrowIfFailed(
		CoCreateInstance(
			CLSID_WICImagingFactory2,
			nullptr,
			CLSCTX_INPROC_SERVER,
			IID_PPV_ARGS(&m_wicFactory)
			)
		);
}

// Configure le périphérique Direct3D et stocke les handles dans ce périphérique et son contexte.
void DX::DeviceResources::CreateDeviceResources() 
{
	// Cet indicateur ajoute la prise en charge des surfaces ayant un classement de canaux de couleurs différent de
	// celui de l'API par défaut. Il est requis à des fins de compatibilité avec Direct2D.
	UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

#if defined(_DEBUG)
	if (DX::SdkLayersAvailable())
	{
		// Si le projet se trouve dans une version Debug, activer le débogage via les couches SDK à l'aide de cet indicateur.
		creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
	}
#endif

	// Ce tableau définit l'ensemble des niveaux de fonctionnalité matérielle DirectX pris en charge par cette application.
	// Noter que le classement doit être conservé.
	// Ne pas oublier de déclarer le niveau de fonctionnalité minimum de votre application dans sa
	// description.  Toutes les applications sont supposées prendre en charge 9.1, sauf indication contraire.
	D3D_FEATURE_LEVEL featureLevels[] = 
	{
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3,
		D3D_FEATURE_LEVEL_9_2,
		D3D_FEATURE_LEVEL_9_1
	};

	// Créer l'objet périphérique de l'API Direct3D 11 et un contexte correspondant.
	ComPtr<ID3D11Device> device;
	ComPtr<ID3D11DeviceContext> context;

	HRESULT hr = D3D11CreateDevice(
		nullptr,					// Spécifier nullptr pour utiliser l'adaptateur par défaut.
		D3D_DRIVER_TYPE_HARDWARE,	// Créez un périphérique à l'aide du pilote graphique matériel.
		0,							// Doit avoir la valeur 0, sauf si le pilote est D3D_DRIVER_TYPE_SOFTWARE.
		creationFlags,				// Définir les indicateurs de débogage et de compatibilité avec Direct2D.
		featureLevels,				// Liste des niveaux de fonctionnalité pris en charge par cette application.
		ARRAYSIZE(featureLevels),	// Taille de la liste ci-dessus.
		D3D11_SDK_VERSION,			// Définissez toujours ce paramètre sur D3D11_SDK_VERSION pour les applications Microsoft Store.
		&device,					// Retourne le périphérique Direct3D créé.
		&m_d3dFeatureLevel,			// Retourne le niveau de fonctionnalité du périphérique créé.
		&context					// Retourne le contexte immédiat du périphérique.
		);

	if (FAILED(hr))
	{
		// Sil l'initialisation échoue, utilisez le périphérique WARP.
		// Pour plus d'informations WARP, consultez : 
		// https://go.microsoft.com/fwlink/?LinkId=286690
		DX::ThrowIfFailed(
			D3D11CreateDevice(
				nullptr,
				D3D_DRIVER_TYPE_WARP, // Créez un périphérique WARP au lieu d'un périphérique matériel.
				0,
				creationFlags,
				featureLevels,
				ARRAYSIZE(featureLevels),
				D3D11_SDK_VERSION,
				&device,
				&m_d3dFeatureLevel,
				&context
				)
			);
	}

	// Stocker les pointeurs vers le périphérique de l'API Direct3D 11.3 et le contexte immédiat.
	DX::ThrowIfFailed(
		device.As(&m_d3dDevice)
		);

	DX::ThrowIfFailed(
		context.As(&m_d3dContext)
		);

	// Créer l'objet périphérique Direct2D et un contexte correspondant.
	ComPtr<IDXGIDevice3> dxgiDevice;
	DX::ThrowIfFailed(
		m_d3dDevice.As(&dxgiDevice)
		);

	DX::ThrowIfFailed(
		m_d2dFactory->CreateDevice(dxgiDevice.Get(), &m_d2dDevice)
		);

	DX::ThrowIfFailed(
		m_d2dDevice->CreateDeviceContext(
			D2D1_DEVICE_CONTEXT_OPTIONS_NONE,
			&m_d2dContext
			)
		);
}

// Ces ressources doivent être recréées chaque fois que la taille de la fenêtre est modifiée.
void DX::DeviceResources::CreateWindowSizeDependentResources() 
{
	// Effacer le précédent contexte spécifique à la taille de la fenêtre.
	ID3D11RenderTargetView* nullViews[] = {nullptr};
	m_d3dContext->OMSetRenderTargets(ARRAYSIZE(nullViews), nullViews, nullptr);
	m_d3dRenderTargetView = nullptr;
	m_d2dContext->SetTarget(nullptr);
	m_d2dTargetBitmap = nullptr;
	m_d3dDepthStencilView = nullptr;
	m_d3dContext->Flush1(D3D11_CONTEXT_TYPE_ALL, nullptr);

	UpdateRenderTargetSize();

	// La largeur et la hauteur de la chaîne de permutation doivent être basées sur la
	// largeur et hauteur en orientation native. Si la fenêtre n'est pas en orientation native
	// l'orientation portrait, les dimensions doivent être inversées.
	DXGI_MODE_ROTATION displayRotation = ComputeDisplayRotation();

	bool swapDimensions = displayRotation == DXGI_MODE_ROTATION_ROTATE90 || displayRotation == DXGI_MODE_ROTATION_ROTATE270;
	m_d3dRenderTargetSize.Width = swapDimensions ? m_outputSize.Height : m_outputSize.Width;
	m_d3dRenderTargetSize.Height = swapDimensions ? m_outputSize.Width : m_outputSize.Height;

	if (m_swapChain != nullptr)
	{
		// Si la chaîne de permutation existe déjà, la redimensionner.
		HRESULT hr = m_swapChain->ResizeBuffers(
			2, // Chaîne de permutation mise deux fois en mémoire tampon.
			lround(m_d3dRenderTargetSize.Width),
			lround(m_d3dRenderTargetSize.Height),
			DXGI_FORMAT_B8G8R8A8_UNORM,
			0
			);

		if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
		{
			// Si le périphérique a été supprimé pour une raison quelconque, un périphérique et une chaîne de permutation doivent être créés.
			HandleDeviceLost();

			// Tout est maintenant configuré. Ne poursuivez pas l'exécution de cette méthode. HandleDeviceLost entrera à nouveau cette méthode 
			// et configurer correctement le nouveau périphérique.
			return;
		}
		else
		{
			DX::ThrowIfFailed(hr);
		}
	}
	else
	{
		// Sinon, en créer une nouvelle en utilisant le même adaptateur que le périphérique Direct3D existant.
		DXGI_SCALING scaling = DisplayMetrics::SupportHighResolutions ? DXGI_SCALING_NONE : DXGI_SCALING_STRETCH;
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {0};

		swapChainDesc.Width = lround(m_d3dRenderTargetSize.Width);		// Faire correspondre avec la taille de la fenêtre.
		swapChainDesc.Height = lround(m_d3dRenderTargetSize.Height);
		swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;				// Il s'agit du format de chaîne de permutation le plus courant.
		swapChainDesc.Stereo = false;
		swapChainDesc.SampleDesc.Count = 1;								// Ne pas utiliser l'échantillonnage multiple.
		swapChainDesc.SampleDesc.Quality = 0;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.BufferCount = 2;									// Utiliser la double mise en mémoire tampon pour réduire la latence.
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;	// Toutes les applications Microsoft Store doivent utiliser _FLIP_ SwapEffects.
		swapChainDesc.Flags = 0;
		swapChainDesc.Scaling = scaling;
		swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

		// Cette séquence obtient la fabrique DXGI utilisée pour créer le périphérique Direct3D ci-dessus.
		ComPtr<IDXGIDevice3> dxgiDevice;
		DX::ThrowIfFailed(
			m_d3dDevice.As(&dxgiDevice)
			);

		ComPtr<IDXGIAdapter> dxgiAdapter;
		DX::ThrowIfFailed(
			dxgiDevice->GetAdapter(&dxgiAdapter)
			);

		ComPtr<IDXGIFactory4> dxgiFactory;
		DX::ThrowIfFailed(
			dxgiAdapter->GetParent(IID_PPV_ARGS(&dxgiFactory))
			);

		// Lorsque vous utilisez l'interopérabilité XAML, la chaîne de permutation doit être créée pour la composition.
		ComPtr<IDXGISwapChain1> swapChain;
		DX::ThrowIfFailed(
			dxgiFactory->CreateSwapChainForComposition(
				m_d3dDevice.Get(),
				&swapChainDesc,
				nullptr,
				&swapChain
				)
			);

		DX::ThrowIfFailed(
			swapChain.As(&m_swapChain)
			);

		// Associer la chaîne de permutation à SwapChainPanel
		// Les changements d'interface utilisateur doivent être retransmises au thread d'interface utilisateur
		m_swapChainPanel->Dispatcher->RunAsync(CoreDispatcherPriority::High, ref new DispatchedHandler([=]()
		{
			// Obtenir l'interface native de stockage pour SwapChainPanel
			ComPtr<ISwapChainPanelNative> panelNative;
			DX::ThrowIfFailed(
				reinterpret_cast<IUnknown*>(m_swapChainPanel)->QueryInterface(IID_PPV_ARGS(&panelNative))
				);

			DX::ThrowIfFailed(
				panelNative->SetSwapChain(m_swapChain.Get())
				);
		}, CallbackContext::Any));

		// Vérifier que DXGI ne met pas plus d'un frame à la fois en file d'attente. Cela permet de réduire la latence et
		// de veiller à ce que l'application effectue le rendu uniquement après chaque VSync, réduisant de ce fait la consommation d'énergie.
		DX::ThrowIfFailed(
			dxgiDevice->SetMaximumFrameLatency(1)
			);
	}

	// Définir l'orientation appropriée pour la chaîne de permutation et générer des
	// transformations de matrices 2D et 3D pour effectuer le rendu vers la chaîne de permutation pivotée.
	// Noter que l’angle de rotation des transformations 2D et 3D est différent.
	// Ceci est dû à la différence entre les espaces de coordonnées.  En outre,
	// la matrice 3D est spécifiée de manière explicite pour éviter les erreurs d'arrondi.

	switch (displayRotation)
	{
	case DXGI_MODE_ROTATION_IDENTITY:
		m_orientationTransform2D = Matrix3x2F::Identity();
		m_orientationTransform3D = ScreenRotation::Rotation0;
		break;

	case DXGI_MODE_ROTATION_ROTATE90:
		m_orientationTransform2D = 
			Matrix3x2F::Rotation(90.0f) *
			Matrix3x2F::Translation(m_logicalSize.Height, 0.0f);
		m_orientationTransform3D = ScreenRotation::Rotation270;
		break;

	case DXGI_MODE_ROTATION_ROTATE180:
		m_orientationTransform2D = 
			Matrix3x2F::Rotation(180.0f) *
			Matrix3x2F::Translation(m_logicalSize.Width, m_logicalSize.Height);
		m_orientationTransform3D = ScreenRotation::Rotation180;
		break;

	case DXGI_MODE_ROTATION_ROTATE270:
		m_orientationTransform2D = 
			Matrix3x2F::Rotation(270.0f) *
			Matrix3x2F::Translation(0.0f, m_logicalSize.Width);
		m_orientationTransform3D = ScreenRotation::Rotation90;
		break;

	default:
		throw ref new FailureException();
	}

	DX::ThrowIfFailed(
		m_swapChain->SetRotation(displayRotation)
		);

	// Configurer l'échelle inverse sur la chaîne de permutation
	DXGI_MATRIX_3X2_F inverseScale = { 0 };
	inverseScale._11 = 1.0f / m_effectiveCompositionScaleX;
	inverseScale._22 = 1.0f / m_effectiveCompositionScaleY;
	ComPtr<IDXGISwapChain2> spSwapChain2;
	DX::ThrowIfFailed(
		m_swapChain.As<IDXGISwapChain2>(&spSwapChain2)
		);

	DX::ThrowIfFailed(
		spSwapChain2->SetMatrixTransform(&inverseScale)
		);

	// Créer un affichage cible de rendu de la mémoire tampon d'arrière-plan de la chaîne de permutation.
	ComPtr<ID3D11Texture2D1> backBuffer;
	DX::ThrowIfFailed(
		m_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer))
		);

	DX::ThrowIfFailed(
		m_d3dDevice->CreateRenderTargetView1(
			backBuffer.Get(),
			nullptr,
			&m_d3dRenderTargetView
			)
		);

	// Créer un affichage gabarit à utiliser avec le rendu 3D si nécessaire.
	CD3D11_TEXTURE2D_DESC1 depthStencilDesc(
		DXGI_FORMAT_D24_UNORM_S8_UINT, 
		lround(m_d3dRenderTargetSize.Width),
		lround(m_d3dRenderTargetSize.Height),
		1, // Cette vue du stencil de profondeur n’a qu’une texture.
		1, // Utiliser un niveau mipmap unique.
		D3D11_BIND_DEPTH_STENCIL
		);

	ComPtr<ID3D11Texture2D1> depthStencil;
	DX::ThrowIfFailed(
		m_d3dDevice->CreateTexture2D1(
			&depthStencilDesc,
			nullptr,
			&depthStencil
			)
		);

	CD3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc(D3D11_DSV_DIMENSION_TEXTURE2D);
	DX::ThrowIfFailed(
		m_d3dDevice->CreateDepthStencilView(
			depthStencil.Get(),
			&depthStencilViewDesc,
			&m_d3dDepthStencilView
			)
		);

	// Définir la fenêtre d'affichage de rendu 3D de manière à ce qu'elle cible la totalité de la fenêtre.
	m_screenViewport = CD3D11_VIEWPORT(
		0.0f,
		0.0f,
		m_d3dRenderTargetSize.Width,
		m_d3dRenderTargetSize.Height
		);

	m_d3dContext->RSSetViewports(1, &m_screenViewport);

	// Créer une image bitmap cible Direct2D associée à la
	// mémoire tampon d'arrière-plan de la chaîne de permutation et la définir en tant que cible actuelle.
	D2D1_BITMAP_PROPERTIES1 bitmapProperties = 
		D2D1::BitmapProperties1(
			D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
			D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
			m_dpi,
			m_dpi
			);

	ComPtr<IDXGISurface2> dxgiBackBuffer;
	DX::ThrowIfFailed(
		m_swapChain->GetBuffer(0, IID_PPV_ARGS(&dxgiBackBuffer))
		);

	DX::ThrowIfFailed(
		m_d2dContext->CreateBitmapFromDxgiSurface(
			dxgiBackBuffer.Get(),
			&bitmapProperties,
			&m_d2dTargetBitmap
			)
		);

	m_d2dContext->SetTarget(m_d2dTargetBitmap.Get());
	m_d2dContext->SetDpi(m_effectiveDpi, m_effectiveDpi);

	// L'anticrénelage du texte en nuances de gris est recommandé pour toutes les applications Microsoft Store.
	m_d2dContext->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
}

// Déterminez les dimensions de la cible de rendu et si elle doit être réduite.
void DX::DeviceResources::UpdateRenderTargetSize()
{
	m_effectiveDpi = m_dpi;
	m_effectiveCompositionScaleX = m_compositionScaleX;
	m_effectiveCompositionScaleY = m_compositionScaleY;

	// Pour améliorer l'autonomie de la batterie sur les appareils haute résolution, définissez une cible de rendu inférieure
	// et autorisez le GPU à mettre à l'échelle la sortie quand elle est présentée.
	if (!DisplayMetrics::SupportHighResolutions && m_dpi > DisplayMetrics::DpiThreshold)
	{
		float width = DX::ConvertDipsToPixels(m_logicalSize.Width, m_dpi);
		float height = DX::ConvertDipsToPixels(m_logicalSize.Height, m_dpi);

		// Quand l'appareil est en orientation portrait, la hauteur > la largeur. Compare
		// la plus grande dimension au seuil de largeur, et la plus petite dimension
		// au seuil de hauteur.
		if (max(width, height) > DisplayMetrics::WidthThreshold && min(width, height) > DisplayMetrics::HeightThreshold)
		{
			// Pour mettre à l'échelle l'application, nous modifions la valeur PPP effective. La taille logique ne change pas.
			m_effectiveDpi /= 2.0f;
			m_effectiveCompositionScaleX /= 2.0f;
			m_effectiveCompositionScaleY /= 2.0f;
		}
	}

	// Calculez la taille nécessaire de la cible de rendu en pixels.
	m_outputSize.Width = DX::ConvertDipsToPixels(m_logicalSize.Width, m_effectiveDpi);
	m_outputSize.Height = DX::ConvertDipsToPixels(m_logicalSize.Height, m_effectiveDpi);

	// Empêcher la création de contenu DirectX de taille nulle.
	m_outputSize.Width = max(m_outputSize.Width, 1);
	m_outputSize.Height = max(m_outputSize.Height, 1);
}

// Cette méthode est appelée lorsque le contrôle XAML est créé (ou recréé).
void DX::DeviceResources::SetSwapChainPanel(SwapChainPanel^ panel)
{
	DisplayInformation^ currentDisplayInformation = DisplayInformation::GetForCurrentView();

	m_swapChainPanel = panel;
	m_logicalSize = Windows::Foundation::Size(static_cast<float>(panel->ActualWidth), static_cast<float>(panel->ActualHeight));
	m_nativeOrientation = currentDisplayInformation->NativeOrientation;
	m_currentOrientation = currentDisplayInformation->CurrentOrientation;
	m_compositionScaleX = panel->CompositionScaleX;
	m_compositionScaleY = panel->CompositionScaleY;
	m_dpi = currentDisplayInformation->LogicalDpi;
	m_d2dContext->SetDpi(m_dpi, m_dpi);

	CreateWindowSizeDependentResources();
}

// Cette méthode est appelée dans le gestionnaire d’événements pour l’événement SizeChanged.
void DX::DeviceResources::SetLogicalSize(Windows::Foundation::Size logicalSize)
{
	if (m_logicalSize != logicalSize)
	{
		m_logicalSize = logicalSize;
		CreateWindowSizeDependentResources();
	}
}

// Cette méthode est appelée dans le gestionnaire d’événements pour l’événement DpiChanged.
void DX::DeviceResources::SetDpi(float dpi)
{
	if (dpi != m_dpi)
	{
		m_dpi = dpi;
		m_d2dContext->SetDpi(m_dpi, m_dpi);
		CreateWindowSizeDependentResources();
	}
}

// Cette méthode est appelée dans le gestionnaire d’événements pour l’événement OrientationChanged.
void DX::DeviceResources::SetCurrentOrientation(DisplayOrientations currentOrientation)
{
	if (m_currentOrientation != currentOrientation)
	{
		m_currentOrientation = currentOrientation;
		CreateWindowSizeDependentResources();
	}
}

// Cette méthode est appelée dans le gestionnaire d'événements pour l'événement CompositionScaleChanged.
void DX::DeviceResources::SetCompositionScale(float compositionScaleX, float compositionScaleY)
{
	if (m_compositionScaleX != compositionScaleX ||
		m_compositionScaleY != compositionScaleY)
	{
		m_compositionScaleX = compositionScaleX;
		m_compositionScaleY = compositionScaleY;
		CreateWindowSizeDependentResources();
	}
}

// Cette méthode est appelée dans le gestionnaire d'événements pour l'événement DisplayContentsInvalidated.
void DX::DeviceResources::ValidateDevice()
{
	// Le périphérique D3D n'est plus valide si l'adaptateur par défaut a été modifié depuis que le périphérique
	// a été créé ou si le périphérique a été supprimé.

	// Tout d'abord, obtenez les informations sur l'adaptateur par défaut lors de la création du périphérique.

	ComPtr<IDXGIDevice3> dxgiDevice;
	DX::ThrowIfFailed(m_d3dDevice.As(&dxgiDevice));

	ComPtr<IDXGIAdapter> deviceAdapter;
	DX::ThrowIfFailed(dxgiDevice->GetAdapter(&deviceAdapter));

	ComPtr<IDXGIFactory2> deviceFactory;
	DX::ThrowIfFailed(deviceAdapter->GetParent(IID_PPV_ARGS(&deviceFactory)));

	ComPtr<IDXGIAdapter1> previousDefaultAdapter;
	DX::ThrowIfFailed(deviceFactory->EnumAdapters1(0, &previousDefaultAdapter));

	DXGI_ADAPTER_DESC1 previousDesc;
	DX::ThrowIfFailed(previousDefaultAdapter->GetDesc1(&previousDesc));

	// Ensuite, obtenez les informations sur l'adaptateur par défaut actuel.

	ComPtr<IDXGIFactory4> currentFactory;
	DX::ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&currentFactory)));

	ComPtr<IDXGIAdapter1> currentDefaultAdapter;
	DX::ThrowIfFailed(currentFactory->EnumAdapters1(0, &currentDefaultAdapter));

	DXGI_ADAPTER_DESC1 currentDesc;
	DX::ThrowIfFailed(currentDefaultAdapter->GetDesc1(&currentDesc));

	// Si les LUID de l'adaptateur ne correspondent pas ou si le périphérique indique qu'il a été supprimé,
	// un nouveau périphérique D3D doit être créé.

	if (previousDesc.AdapterLuid.LowPart != currentDesc.AdapterLuid.LowPart ||
		previousDesc.AdapterLuid.HighPart != currentDesc.AdapterLuid.HighPart ||
		FAILED(m_d3dDevice->GetDeviceRemovedReason()))
	{
		// Libérer les références aux ressources associées à l'ancien périphérique.
		dxgiDevice = nullptr;
		deviceAdapter = nullptr;
		deviceFactory = nullptr;
		previousDefaultAdapter = nullptr;

		// Créez un périphérique et une chaîne de permutation.
		HandleDeviceLost();
	}
}

// Recréer toutes les ressources du périphérique et les remettre à leur état actuel.
void DX::DeviceResources::HandleDeviceLost()
{
	m_swapChain = nullptr;

	if (m_deviceNotify != nullptr)
	{
		m_deviceNotify->OnDeviceLost();
	}

	CreateDeviceResources();
	m_d2dContext->SetDpi(m_dpi, m_dpi);
	CreateWindowSizeDependentResources();

	if (m_deviceNotify != nullptr)
	{
		m_deviceNotify->OnDeviceRestored();
	}
}

// Inscrire notre DeviceNotify pour être informé de la perte et de la création de périphérique.
void DX::DeviceResources::RegisterDeviceNotify(DX::IDeviceNotify* deviceNotify)
{
	m_deviceNotify = deviceNotify;
}

// Appelez cette méthode lorsque l'application s'interrompt. Cela indique au pilote que l'application 
// devient inactive et que les mémoires tampons temporaires peuvent être récupérées et utilisées par d'autres applications.
void DX::DeviceResources::Trim()
{
	ComPtr<IDXGIDevice3> dxgiDevice;
	m_d3dDevice.As(&dxgiDevice);

	dxgiDevice->Trim();
}

// Affichez le contenu de la chaîne de permutation à l'écran.
void DX::DeviceResources::Present() 
{
	// Le premier argument prescrit à DXGI de bloquer jusqu'au VSync, mettant l'application
	// en veille jusqu'au prochain VSync. Cela évite des rendus inutiles de
	// frames qui ne seront jamais affichés à l’écran.
	DXGI_PRESENT_PARAMETERS parameters = { 0 };
	HRESULT hr = m_swapChain->Present1(1, 0, &parameters);

	// Supprimer le contenu de la cible de rendu.
	// Cette opération est valide uniquement si l'intégralité du contenu existant est
	// remplacé(e). Si dirty ou scroll est utilisé, cet appel doit être modifié.
	m_d3dContext->DiscardView1(m_d3dRenderTargetView.Get(), nullptr, 0);

	// Supprimer le contenu du stencil de profondeur.
	m_d3dContext->DiscardView1(m_d3dDepthStencilView.Get(), nullptr, 0);

	// Si le périphérique est supprimé suite à une déconnexion ou une mise à niveau du pilote, 
	// toutes les ressources du périphérique doivent être recréées.
	if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
	{
		HandleDeviceLost();
	}
	else
	{
		DX::ThrowIfFailed(hr);
	}
}

// Cette méthode détermine la rotation entre l'orientation native du périphérique d'affichage et
// orientation actuelle de l'affichage.
DXGI_MODE_ROTATION DX::DeviceResources::ComputeDisplayRotation()
{
	DXGI_MODE_ROTATION rotation = DXGI_MODE_ROTATION_UNSPECIFIED;

	// Remarque : NativeOrientation peut uniquement être Landscape ou Portrait même si
	// l'enum DisplayOrientations a d'autres valeurs.
	switch (m_nativeOrientation)
	{
	case DisplayOrientations::Landscape:
		switch (m_currentOrientation)
		{
		case DisplayOrientations::Landscape:
			rotation = DXGI_MODE_ROTATION_IDENTITY;
			break;

		case DisplayOrientations::Portrait:
			rotation = DXGI_MODE_ROTATION_ROTATE270;
			break;

		case DisplayOrientations::LandscapeFlipped:
			rotation = DXGI_MODE_ROTATION_ROTATE180;
			break;

		case DisplayOrientations::PortraitFlipped:
			rotation = DXGI_MODE_ROTATION_ROTATE90;
			break;
		}
		break;

	case DisplayOrientations::Portrait:
		switch (m_currentOrientation)
		{
		case DisplayOrientations::Landscape:
			rotation = DXGI_MODE_ROTATION_ROTATE90;
			break;

		case DisplayOrientations::Portrait:
			rotation = DXGI_MODE_ROTATION_IDENTITY;
			break;

		case DisplayOrientations::LandscapeFlipped:
			rotation = DXGI_MODE_ROTATION_ROTATE270;
			break;

		case DisplayOrientations::PortraitFlipped:
			rotation = DXGI_MODE_ROTATION_ROTATE180;
			break;
		}
		break;
	}
	return rotation;
}
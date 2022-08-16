#pragma once

namespace App1
{
	// Mémoire tampon constante utilisée pour envoyer les matrices MVP au nuanceur de sommets.
	struct ModelViewProjectionConstantBuffer
	{
		DirectX::XMFLOAT4X4 model;
		DirectX::XMFLOAT4X4 view;
		DirectX::XMFLOAT4X4 projection;
	};

	// Utilisé pour envoyer les données par sommet au nuanceur de sommets.
	struct VertexPositionColor
	{
		DirectX::XMFLOAT3 pos;
		DirectX::XMFLOAT3 color;
	};
}
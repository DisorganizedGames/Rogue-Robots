#include "LayerStack.h"
namespace DOG
{
	LayerStack LayerStack::s_instance;

	LayerStack::LayerStack() noexcept
		:m_layers{}, 
		 m_layerIt{ m_layers.begin() }, 
		 m_overlayIt{ m_layers.begin() },
		 m_nrOfRegularLayers{ 0u }
	{
	}

	void LayerStack::PushLayer(Layer* layer) noexcept
	{
		layer->OnAttach();
		m_layers.insert(m_layers.begin() + m_nrOfRegularLayers, layer);
		m_nrOfRegularLayers++;
	}

	void LayerStack::PushOverlay(Layer* layer) noexcept
	{
		layer->OnAttach();
		m_layers.emplace_back(layer);
	}

	void LayerStack::PopLayer(Layer* layer) noexcept
	{
		auto it = std::find(m_layers.begin(), m_layers.end(), layer);
		ASSERT(it < m_layers.begin() + m_nrOfRegularLayers, "Layer does not exist in stack.");
		layer->OnDetach();
		m_layers.erase(it);
		m_nrOfRegularLayers--;
	}

	void LayerStack::PopOverlay(Layer* layer) noexcept
	{
		auto it = std::find(m_layers.begin(), m_layers.end(), layer);
		ASSERT(it >= m_layers.begin() + m_nrOfRegularLayers, "Overlay does not exist in stack.");
		layer->OnDetach();
		m_layers.erase(it);
	}

}
#pragma once
#include "Layer.h"
namespace DOG
{
	class LayerStack
	{
	public:
		LayerStack() noexcept;
		~LayerStack() noexcept = default;
		void PushLayer(Layer* layer) noexcept;
		void PushOverlay(Layer* layer) noexcept;
		void PopLayer(Layer* layer) noexcept;
		void PopOverlay(Layer* layer) noexcept;

		[[nodiscard]] std::vector<Layer*>::const_iterator begin() { return m_layers.begin(); }
		[[nodiscard]] std::vector<Layer*>::const_iterator end() { return m_layers.end(); }
	private:
		DELETE_COPY_MOVE_CONSTRUCTOR(LayerStack);
		std::vector<Layer*> m_layers;
		std::vector<Layer*>::iterator m_layerIt;
		std::vector<Layer*>::iterator m_overlayIt;
		u32 m_nrOfRegularLayers;
	};
}
#pragma once
#include "Layer.h"
namespace DOG
{
	class LayerStack
	{
	public:
		[[nodiscard]] constexpr static LayerStack& Get() noexcept { return s_instance; }
		void PushLayer(Layer* layer) noexcept;
		void PushOverlay(Layer* layer) noexcept;
		void PopLayer(Layer* layer) noexcept;
		void PopOverlay(Layer* layer) noexcept;

		[[nodiscard]] std::vector<Layer*>::const_iterator begin() { return m_layers.begin(); }
		[[nodiscard]] std::vector<Layer*>::const_iterator end() { return m_layers.end(); }
	private:
		LayerStack() noexcept;
		~LayerStack() noexcept = default;
		DELETE_COPY_MOVE_CONSTRUCTOR(LayerStack);
	private:
		static LayerStack s_instance;
		std::vector<Layer*> m_layers;
		std::vector<Layer*>::iterator m_layerIt;
		std::vector<Layer*>::iterator m_overlayIt;
		u32 m_nrOfRegularLayers;
	};
}
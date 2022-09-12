
namespace DOG::gfx
{
	class RenderDevice;

	class RenderBackend
	{
	public:
		virtual RenderDevice* CreateDevice() = 0;

		virtual ~RenderBackend() {}
	};
}

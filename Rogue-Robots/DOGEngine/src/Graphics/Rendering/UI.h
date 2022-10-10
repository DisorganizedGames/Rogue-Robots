#include <string>
#include <time.h>
#include <functional>
#include "../RHI/DX12/D2DBackend_DX12.h"
#include "../RHI/RenderDevice.h"

class UIelement;
class RenderDevice;
class Swapchain;

enum MenuUI
{
   bplay,
   boptions,
   bexit
};

enum GameUI
{
   inventory
};

enum PauseUI
{

};

enum Uiscene
{
   menu,
   game
};

class UI
{
public:
   std::vector<std::unique_ptr<UIelement>> m_elements;
   std::unique_ptr<DOG::gfx::D2DBackend_DX12> m_d2d; //The thing that renders everything
   UINT m_width, m_height;
   UI(DOG::gfx::RenderDevice* m_rd, DOG::gfx::Swapchain* sc, u_int maxFramesInFlight, HWND hwnd);
   ~UI();
   void DrawUI();
   void ChangeUIscene(Uiscene scene);
private:
   Uiscene m_scene;
   bool m_visible;
   void BuildMenuUI();
   void BuildGameUI();
   ComPtr<IDWriteTextFormat> m_btextformat;
};

class UIelement
{
public:
   UIelement();
   UIelement(D2D_POINT_2F pos);
   virtual void Draw(DOG::gfx::D2DBackend_DX12 &m_d2d) = 0;
   virtual void Update(DOG::gfx::D2DBackend_DX12& m_d2d);
   
   virtual ~UIelement();
private:
   u32 id;
   D2D_POINT_2F pos;
};

class UIButton : public UIelement
{
public:
   bool pressed;
   UIButton(D2D_POINT_2F pos,D2D_VECTOR_2F size, std::wstring text, std::function<void(void)> callback);
   void Draw(DOG::gfx::D2DBackend_DX12 &m_d2d) override;
   void Update(DOG::gfx::D2DBackend_DX12& m_d2d) override;
   ~UIButton();
private:
   D2D_POINT_2F m_pos;
   D2D1_RECT_F m_textRect;
   D2D_VECTOR_2F m_size;
   std::wstring m_text;
   std::function<void(void)> m_callback;
};


class UISplashScreen : public UIelement
{
public:
   UISplashScreen(DOG::gfx::D2DBackend_DX12& m_d2d, float width, float height);
   void Draw(DOG::gfx::D2DBackend_DX12 &m_d2d) override;
   void Update(DOG::gfx::D2DBackend_DX12& m_d2d) override;
   ~UISplashScreen();
private:
   D2D1_RECT_F m_background;
   clock_t m_timer;
   ComPtr<ID2D1SolidColorBrush> m_splashBrush, m_textBrush;
   std::wstring m_text;
   float m_textOp, m_backOp;
};

class UIHealthBar : public UIelement
{
   private:
   D2D1_RECT_F m_background;

};
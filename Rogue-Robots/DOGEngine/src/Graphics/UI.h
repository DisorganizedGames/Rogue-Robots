#include <memory>
#include <vector>
#include <string>
#include <time.h>
#include "RHI/DX12/d2dBackend_DX12.h"
#include "RHI/RenderDevice.h"
#include "../Core/Time.h"

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
private:
   Uiscene m_scene;
   bool m_visible;
   void BuildMenuUI();
   void BuildGameUI();
   ComPtr<IDWriteTextFormat> m_btextformat;
public:
   std::vector<UIelement*> m_elements;
   UINT m_width, m_height;
   DOG::gfx::D2DBackend_DX12 m_d2d; //The thing that renders everything
   UI(DOG::gfx::RenderDevice* m_rd, DOG::gfx::Swapchain* sc, u_int maxFramesInFlight, HWND hwnd);
   ~UI();
   void DrawUI();
   void ChangeUIscene(Uiscene scene);
};

class UIelement
{
private:
   D2D_POINT_2F pos;
public:
   UIelement();
   UIelement(D2D_POINT_2F pos);
   virtual void Draw(DOG::gfx::D2DBackend_DX12 &m_d2d) = 0;
   virtual void Update(DOG::gfx::D2DBackend_DX12& m_d2d);
   
   virtual ~UIelement();
};

class UIButton : public UIelement
{
private:
   D2D_POINT_2F m_pos;
   D2D1_RECT_F m_textRect;
   D2D_VECTOR_2F m_size;
   std::wstring m_text;
public:
   bool pressed;
   UIButton(D2D_POINT_2F pos,D2D_VECTOR_2F size, std::wstring text);
   void Draw(DOG::gfx::D2DBackend_DX12 &m_d2d) override;
   void Update(DOG::gfx::D2DBackend_DX12& m_d2d) override;
   ~UIButton();
};


class UISplashScreen : public UIelement
{
private:
   D2D1_RECT_F m_background;
   clock_t m_timer;
   ID2D1SolidColorBrush* m_splashBrush, *m_textBrush;
   std::wstring m_text;
   float m_textOp, m_backOp;
public:
   UISplashScreen(DOG::gfx::D2DBackend_DX12& m_d2d, float width, float height);
   void Draw(DOG::gfx::D2DBackend_DX12 &m_d2d) override;
   void Update(DOG::gfx::D2DBackend_DX12& m_d2d) override;
   ~UISplashScreen();
};
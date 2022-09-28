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

enum menuUI
{
   bplay = 1,
   bexit
};

enum Uiscene
{
   menu,
   game
};

class UI
{
private:
public:
   std::vector<std::unique_ptr<UIelement>> elements;
   DOG::gfx::d2dBackend_DX12 m_d2d; //The thing that renders everything
   UI(DOG::gfx::RenderDevice* rd, DOG::gfx::Swapchain* sc, u_int maxFramesInFlight, HWND hwnd);
   ~UI();
   void drawUI();
   void changeUIscene(Uiscene scene);
};

class UIelement
{
private:
   D2D_POINT_2F pos;
public:
   UIelement();
   UIelement(D2D_POINT_2F pos);
   virtual void draw(DOG::gfx::d2dBackend_DX12 &m_d2d) = 0;
   virtual void update(DOG::gfx::d2dBackend_DX12& m_d2d);
   
   virtual ~UIelement();
};

class UIButton : public UIelement
{
private:
   D2D_POINT_2F pos;
   D2D1_RECT_F textRect;
   D2D_VECTOR_2F size;
   std::wstring text;
public:
   bool pressed;
   UIButton(D2D_POINT_2F pos,D2D_VECTOR_2F size, std::wstring text);
   void draw(DOG::gfx::d2dBackend_DX12 &m_d2d) override;
   void update(DOG::gfx::d2dBackend_DX12& m_d2d) override;
   ~UIButton();
};


class UISplashScreen : public UIelement
{
private:
   D2D1_RECT_F background;
   clock_t timer;
   ID2D1SolidColorBrush* splashBrush, *textBrush;
   std::wstring text;
   float textOp, backOp;
public:
   UISplashScreen(DOG::gfx::d2dBackend_DX12& m_d2d);
   void draw(DOG::gfx::d2dBackend_DX12 &m_d2d) override;
   void update(DOG::gfx::d2dBackend_DX12& m_d2d) override;
   ~UISplashScreen();
};
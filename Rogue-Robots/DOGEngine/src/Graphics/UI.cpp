#include "UI.h"
#include "../Input/Mouse.h"
#include <assert.h>

UI::UI(DOG::gfx::RenderDevice* rd, DOG::gfx::Swapchain* sc, u_int numBuffers, HWND hwnd) : m_d2d(rd, sc, numBuffers, hwnd)
{
   
   D2D_POINT_2F p = { 100.f, 100.f };
   D2D_VECTOR_2F s = {400.f, 100.f};
   //std::unique_ptr<UIelement> b = std::make_unique<UIButton>(p, "play");
   //elements.push_back(std::move(std::make_unique<UIButton>(p,s, L"ROGUE ROBOTS")));
   elements.push_back(std::move(std::make_unique<UISplashScreen>( m_d2d)));


   
   

   //p.x = 500;
   //p.y = 500;
   //elements.push_back(std::move(std::make_unique<UIButton>(p,s, L"Button2")));
}

UI::~UI()
{
}


void UI::drawUI()
{
   for (auto&& e : elements)
   {
      e->update(m_d2d);
      e->draw(m_d2d);
   }

}

UIelement::UIelement()
{
   
}

UIelement::UIelement(D2D_POINT_2F pos)
{
   this->pos = pos;
}

UIelement::~UIelement()
{
}

void UIelement::update(DOG::gfx::d2dBackend_DX12& m_d2d)
{
   return;
}

UIButton::UIButton(D2D_POINT_2F pos, D2D_VECTOR_2F size, std::wstring text) : pos(pos)
{
   this->size = size;
   
   textRect = D2D1::RectF(pos.x, pos.y, pos.x + size.x, pos.y + size.y);
   this->text = text;
}

UIButton::~UIButton()
{
}

void UIButton::draw(DOG::gfx::d2dBackend_DX12& m_d2d)
{
   m_d2d.m_2ddc->DrawRectangle(textRect, m_d2d.brush.Get());
   m_d2d.m_2ddc->DrawTextW(
      text.c_str(),
      text.length(),
      m_d2d.format.Get(),
      &textRect,
      m_d2d.brush.Get()
   );
}

void UIButton::update(DOG::gfx::d2dBackend_DX12& m_d2d)
{
   // static float angle = 0;
   // auto t = D2D1::Matrix3x2F::Rotation( angle += 0.1f, D2D1::Point2F(468.0f, 331.5f));
   // m_d2d.m_2ddc->SetTransform(t);
   auto m = DOG::Mouse::GetCoordinates();
   // if(m.first >= textRect.left && m.first <= textRect.right && m.second >= textRect.top && m.second <= textRect.bottom)
   //    m_d2d.brush.Get()->SetOpacity(1.0f);
   // else
   //    m_d2d.brush.Get()->SetOpacity(0.5f);

}

UISplashScreen::UISplashScreen(DOG::gfx::d2dBackend_DX12& m_d2d)
{
   timer = clock();
   background = D2D1::RectF(0.0f, 0.0f, 1280.f, 720.f);
   text = L"Disorganized Games";

   HRESULT hr = m_d2d.m_2ddc->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black, 1.0f), &splashBrush);
   HR_VFY(hr);
   hr = m_d2d.m_2ddc->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White, 1.0f), &textBrush);
   HR_VFY(hr);
   backOp = 1.0f;
   textOp = 0.0f;
}

void UISplashScreen::draw(DOG::gfx::d2dBackend_DX12 &m_d2d)
{
   m_d2d.m_2ddc->FillRectangle(background, splashBrush);
   m_d2d.m_2ddc->DrawTextW(
      text.c_str(),
      text.length(),
      m_d2d.format.Get(),
      &background,
      textBrush);
}
void UISplashScreen::update(DOG::gfx::d2dBackend_DX12& m_d2d)
{
   float time = clock() / CLOCKS_PER_SEC;
   if (time <= 13 && time >= 3)
   {
      if(textOp <= 1.0f)
         textBrush->SetOpacity(textOp += 0.01f);
   }
   else
      textBrush->SetOpacity(textOp -= 0.01f);

   if (time >= 14.8f)
      splashBrush->SetOpacity(backOp -= 0.01f);
   
}

UISplashScreen::~UISplashScreen()
{

}

float easeOutCubic(float x) {
   return 1 - powf(1 - x, 3);
}
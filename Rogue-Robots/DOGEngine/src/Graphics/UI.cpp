#include "UI.h"
#include "../Input/Mouse.h"
#include "../Input/Keyboard.h"
#include <assert.h>

UI::UI(DOG::gfx::RenderDevice* rd, DOG::gfx::Swapchain* sc, u_int numBuffers, HWND hwnd) : m_d2d(rd, sc, numBuffers, hwnd)
{
   RECT wrect;
   if (!GetClientRect(hwnd, &wrect))
      OutputDebugString(L"Error retreiving client rect in UI creation\n");
   m_width = wrect.right;
   m_height = wrect.bottom;
   this->scene = menu;
   BuildMenuUI();
   m_elements.push_back(new UISplashScreen(m_d2d, m_width, m_height));
}

UI::~UI()
{

}

void UI::DrawUI()
{

   for (auto&& e : m_elements)
   {
      e->Update(m_d2d);
      e->Draw(m_d2d);
   }
   if (scene == menu && dynamic_cast<UIButton*>(m_elements[menuUI::bplay])->pressed )
      ChangeUIscene(game);
   else if (scene == game && dynamic_cast<UIButton*>(m_elements[gameUI::inventory])->pressed )
      ChangeUIscene(menu);
   else if(scene == menu && dynamic_cast<UIButton*>(m_elements[menuUI::bexit])->pressed)
      ;
}

void UI::BuildMenuUI()
{
   D2D_VECTOR_2F s = { 150.f, 60.f };
   D2D_POINT_2F p = { m_width / 2.f - s.x / 2, m_height / 2 - s.y / 2 };
   m_elements.push_back(new UIButton(p, s, L"Play"));
   p.y += s.y + 50;
   m_elements.push_back(new UIButton(p, s, L"Options"));
   p.y += s.y + 50;
   m_elements.push_back(new UIButton(p, s, L"Exit"));
}

void UI::BuildGameUI()
{
   D2D_POINT_2F p = { 50.f, 50.f };
   D2D_VECTOR_2F s = { 250.f, 100.f };
   m_elements.push_back(new UIButton(p, s, L"Menu"));
}

void UI::ChangeUIscene(Uiscene scene)
{
   switch (scene)
   {
   case menu:
      m_elements.clear();
      BuildMenuUI();
      this->scene = menu;
      break;
   case game:
      m_elements.clear();
      BuildGameUI();
      this->scene = game;
      break;
   default:
      break;
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

void UIelement::Update(DOG::gfx::d2dBackend_DX12& m_d2d)
{
   return;
}

UIButton::UIButton(D2D_POINT_2F pos, D2D_VECTOR_2F size, std::wstring text) : m_pos(pos), pressed(false)
{
   this->m_size = size;
   m_textRect = D2D1::RectF(pos.x, pos.y, pos.x + size.x, pos.y + size.y);
   this->m_text = text;
}

UIButton::~UIButton()
{

}

void UIButton::Draw(DOG::gfx::d2dBackend_DX12& m_d2d)
{
   m_d2d.m_2ddc->DrawRectangle(m_textRect, m_d2d.brush.Get());
   m_d2d.m_2ddc->DrawTextW(
      m_text.c_str(),
      m_text.length(),
      m_d2d.bformat.Get(),
      &m_textRect,
      m_d2d.brush.Get()
   );
}

void UIButton::Update(DOG::gfx::d2dBackend_DX12& m_d2d)
{

   auto m = DOG::Mouse::GetCoordinates();
   if (m.first >= m_textRect.left && m.first <= m_textRect.right && m.second >= m_textRect.top && m.second <= m_textRect.bottom)
   {
      m_d2d.brush.Get()->SetOpacity(1.0f);
      if (DOG::Mouse::IsButtonPressed(DOG::Button::Left))
         pressed = true;
      else
         pressed = false;
   }
   else
      m_d2d.brush.Get()->SetOpacity(0.5f);

}

UISplashScreen::UISplashScreen(DOG::gfx::d2dBackend_DX12& m_d2d, float width, float height)
{
   m_timer = clock();
   m_background = D2D1::RectF(0.0f, 0.0f, width, height);
   m_text = L"Disorganized Games";

   HRESULT hr = m_d2d.m_2ddc->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black, 1.0f), &m_splashBrush);
   HR_VFY(hr);
   hr = m_d2d.m_2ddc->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White, 1.0f), &m_textBrush);
   HR_VFY(hr);
   m_backOp = 1.0f;
   m_textOp = 0.0f;
}

void UISplashScreen::Draw(DOG::gfx::d2dBackend_DX12& m_d2d)
{
   m_d2d.m_2ddc->FillRectangle(m_background, m_splashBrush);
   m_d2d.m_2ddc->DrawTextW(
      m_text.c_str(),
      m_text.length(),
      m_d2d.format.Get(),
      &m_background,
      m_textBrush);
}
void UISplashScreen::Update(DOG::gfx::d2dBackend_DX12& m_d2d)
{
   float time = clock() / CLOCKS_PER_SEC;
   if (time <= 4 && time >= 0)
   {
      if (m_textOp <= 1.0f)
         m_textBrush->SetOpacity(m_textOp += 0.01f);
   }
   else
      m_textBrush->SetOpacity(m_textOp -= 0.01f);

   if (time >= 6.f)
      m_splashBrush->SetOpacity(m_backOp -= 0.01f);

}

UISplashScreen::~UISplashScreen()
{
   m_textBrush->Release();
   m_splashBrush->Release();
}

float easeOutCubic(float x)
{
   return 1 - powf(1 - x, 3);
}
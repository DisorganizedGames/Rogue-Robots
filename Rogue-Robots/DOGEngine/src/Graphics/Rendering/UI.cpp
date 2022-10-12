#include "UI.h"
#include "../../Input/Mouse.h"
#include "../../Input/Keyboard.h"
#include <assert.h>
#include <Intsafe.h>
#include <algorithm>

void buttonfunc()
{
   OutputDebugString(L"Button pressed");
}

UI::UI(DOG::gfx::RenderDevice* rd, DOG::gfx::Swapchain* sc, u_int numBuffers, HWND hwnd) : m_visible(true)
{
   srand((UINT)time(NULL));
   RECT wrect;
   if (!GetClientRect(hwnd, &wrect))
      OutputDebugString(L"Error retreiving client rect in UI creation\n");
   m_width = wrect.right;
   m_height = wrect.bottom;
   m_d2d = std::make_unique<DOG::gfx::D2DBackend_DX12>(rd, sc, numBuffers, hwnd);
   UINT menuID = AddScene();
   UINT gameID = AddScene();
   ChangeUIscene(menuID);
   auto hID = GenerateUID();
   auto h = std::make_unique<UIHealthBar>(40.f, m_height - 60.f, 250.f, 30.f, *m_d2d, hID);
   AddUIlEmentToScene(gameID, std::move(h));
   auto bID = GenerateUID();
   auto b = std::make_unique<UIButton>(m_width / 2.f - 150.f / 2, m_height / 2 - 60.f / 2, 150.f, 60.f, std::wstring(L"Play"), buttonfunc, bID);
   AddUIlEmentToScene(menuID, std::move(b));
   auto sID = GenerateUID();
   auto s = std::make_unique<UISplashScreen>(*m_d2d, (float)m_width, (float)m_height, sID);
   AddUIlEmentToScene(menuID, std::move(s));

}

UI::~UI()
{

}


void UI::DrawUI()
{
   if (DOG::Keyboard::IsKeyPressed(DOG::Key::G))
   {
      m_visible = false;
   }
   if (m_visible)
   {
      for (auto&& e : m_scenes[m_currsceneIndex]->m_scene)
      {
         e->Update(*m_d2d);
         e->Draw(*m_d2d);
      }
   }
}

/// @brief Generates a unique ID
/// @return Unique ID
UINT UI::GenerateUID()
{
   UINT uid;
   do
      uid = rand();
   while (std::find(m_generatedIDs.begin(), m_generatedIDs.end(), uid) != m_generatedIDs.end());
   m_generatedIDs.push_back(uid);
   return uid;
}

/// @brief Querrys a scene from a specific sceneID
/// @param sceneID The ID of the scene
/// @return Returns the index of the scene witrh the specific sceneID. If scene is not found UINT_MAX is returned;
UINT UI::QuerryScene(UINT sceneID)
{
   UINT index;
   auto res = std::find_if(m_scenes.begin(), m_scenes.end(), [&](std::unique_ptr<UIScene> const& s) { return s->m_ID == sceneID; });
   if (res == m_scenes.end())
      return UINT_MAX;
   else
   {
      ptrdiff_t ptrdiff = std::distance(m_scenes.begin(), res);
      HR_VFY(PtrdiffTToUInt(ptrdiff, &index));
      return index;
   }
}

/// @brief Adds a ui element to a specific UIscene
/// @param sceneID The ID of the scene
/// @param element The element to be added
/// @return The unique ID of the element. If the scene does not exist UINT_MAX is returned
UINT UI::AddUIlEmentToScene(UINT sceneID, std::unique_ptr<UIElement> element)
{
   UINT index = QuerryScene(sceneID);
   UINT id;
   if (index == UINT_MAX)
      return UINT_MAX;
   else
   {
      id = element->m_ID;
      m_scenes[index]->m_scene.push_back(std::move(element));
   }
   return id;
}

/// @brief Adds a scene to the UI manager
/// @return A unique ID for the newly created scene
UINT UI::AddScene()
{
   auto scene = std::make_unique<UIScene>(GenerateUID());
   UINT id = scene->m_ID;
   m_scenes.push_back(std::move(scene));
   return id;
}

/// @brief Removes a scene with a specific ID
/// @param sceneID The ID of the scene to be removed
void UI::RemoveScene(UINT sceneID)
{
   UINT index = QuerryScene(sceneID);
   if (index == UINT_MAX)
      return;
   else
      m_scenes.erase(m_scenes.begin() + index);
}


/// @brief Changes the current scene to a scene with a specific ID
/// @param sceneID The ID of the scene to switch to
void UI::ChangeUIscene(UINT sceneID)
{
   UINT index = QuerryScene(sceneID);
   if (index == UINT_MAX)
      return;
   else
   {
      m_currsceneIndex = index;
      m_currsceneID = sceneID;
   }
}

UIElement::UIElement(UINT id) : m_ID(id)
{

}

UIElement::~UIElement()
{
}

void UIElement::Update(DOG::gfx::D2DBackend_DX12& m_d2d)
{
   UNREFERENCED_PARAMETER(m_d2d);
   return;
}

UIButton::UIButton(float x, float y, float width, float height, std::wstring text, std::function<void(void)> callback, UINT id) : pressed(false), m_callback(callback), UIElement(id)
{
   this->m_size = D2D1::Vector2F(width, height);
   m_textRect = D2D1::RectF(x, y, x + width, y + height);
   this->m_text = text;
}

UIButton::~UIButton()
{

}

void UIButton::Draw(DOG::gfx::D2DBackend_DX12& m_d2d)
{
   m_d2d.m_2ddc->DrawRectangle(m_textRect, m_d2d.brush.Get());
   m_d2d.m_2ddc->DrawTextW(
      m_text.c_str(),
      (UINT32)m_text.length(),
      m_d2d.bformat.Get(),
      &m_textRect,
      m_d2d.brush.Get()
   );
}

void UIButton::Update(DOG::gfx::D2DBackend_DX12& m_d2d)
{

   auto m = DOG::Mouse::GetCoordinates();
   if (m.first >= m_textRect.left && m.first <= m_textRect.right && m.second >= m_textRect.top && m.second <= m_textRect.bottom)
   {
      m_d2d.brush.Get()->SetOpacity(1.0f);
      if (DOG::Mouse::IsButtonPressed(DOG::Button::Left))
         m_callback();
   }
   else
      m_d2d.brush.Get()->SetOpacity(0.5f);

}

UISplashScreen::UISplashScreen(DOG::gfx::D2DBackend_DX12& m_d2d, float width, float height, UINT id) : UIElement(id)
{
   UNREFERENCED_PARAMETER(m_d2d);
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

void UISplashScreen::Draw(DOG::gfx::D2DBackend_DX12& m_d2d)
{
   m_d2d.m_2ddc->FillRectangle(m_background, m_splashBrush.Get());
   m_d2d.m_2ddc->DrawTextW(
      m_text.c_str(),
      (UINT32)m_text.length(),
      m_d2d.format.Get(),
      &m_background,
      m_textBrush.Get());
}
void UISplashScreen::Update(DOG::gfx::D2DBackend_DX12& m_d2d)
{
   UNREFERENCED_PARAMETER(m_d2d);
   float time = (float)(clock() / CLOCKS_PER_SEC);
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

}

float easeOutCubic(float x)
{
   return 1 - powf(1 - x, 3);
}

UIScene::UIScene(UINT id) : m_ID(id)
{

}

UIHealthBar::UIHealthBar(float x, float y, float width, float height, DOG::gfx::D2DBackend_DX12& m_d2d, UINT id) : UIElement(id)
{
   m_text = L"100%";
   m_value = m_test = 1.0f;
   m_barWidth = width - 2.f;
   m_border = D2D1::RectF(x, y, x + width, y + height);
   m_bar = D2D1::RectF(x + 2.0f, y + 2.0f, x + width - 2.f, y + height - 2.f);
   HRESULT hr = m_d2d.m_2ddc->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White, 1.0f), &m_borderBrush);
   HR_VFY(hr);
   hr = m_d2d.m_2ddc->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::GreenYellow, 0.5f), &m_barBrush);
   HR_VFY(hr);
   hr = m_d2d.m_dwritwf->CreateTextFormat(
      L"Robot Radicals",
      NULL,
      DWRITE_FONT_WEIGHT_NORMAL,
      DWRITE_FONT_STYLE_NORMAL,
      DWRITE_FONT_STRETCH_NORMAL,
      12,
      L"en-us",
      &m_textFormat
   );
   HR_VFY(hr);
   hr = m_textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
   HR_VFY(hr);
   hr = m_textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
   HR_VFY(hr);
}

UIHealthBar::~UIHealthBar()
{

}

void UIHealthBar::Draw(DOG::gfx::D2DBackend_DX12& m_d2d)
{
   m_d2d.m_2ddc->FillRectangle(m_bar, m_barBrush.Get());
   m_d2d.m_2ddc->DrawRectangle(m_border, m_barBrush.Get());
   m_d2d.m_2ddc->DrawTextW(
      m_text.c_str(),
      (UINT32)m_text.length(),
      m_textFormat.Get(),
      &m_border,
      m_borderBrush.Get());

}
void UIHealthBar::Update(DOG::gfx::D2DBackend_DX12& m_d2d)
{
   auto val = abs(sinf(m_value += 0.01f));
   m_bar.right = val * (m_barWidth)+m_bar.left - 1.0f;
   m_text = std::to_wstring((UINT)(val * 100.f + 1.f)) + L'%';
}
void UIHealthBar::SetBarValue(float value)
{
   m_value = value;
   m_text = std::to_wstring(value) + L'%';
}
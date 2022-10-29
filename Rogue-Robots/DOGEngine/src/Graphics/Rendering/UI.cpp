#include "UI.h"
#include "../../Input/Mouse.h"
#include "../../Input/Keyboard.h"
//#include "../../EventSystem/IEvent.h"
#include "../../EventSystem/KeyboardEvents.h"
#include "../../EventSystem/MouseEvents.h"


DOG::UI* DOG::UI::s_instance = nullptr;

UINT menuID, gameID, optionsID, multiID;
UINT menuBackID, optionsBackID, multiBackID;
UINT bpID, bmID, boID, beID, optbackID, mulbackID, bhID, bjID;
UINT cID, tID, hID;

void PlayButtonFunc(void)
{
   DOG::UI::Get()->ChangeUIscene(gameID);
}

void OptionsButtonFunc(void)
{
   DOG::UI::Get()->ChangeUIscene(optionsID);
}

void MultiplayerButtonFunc(void)
{
   DOG::UI::Get()->ChangeUIscene(multiID);
}

void ToMenuButtonFunc(void)
{
   DOG::UI::Get()->ChangeUIscene(menuID);
}

void ExitButtonFunc(void)
{
   //Exit game
}

DOG::UI::UI(DOG::gfx::RenderDevice* rd, DOG::gfx::Swapchain* sc, UINT numBuffers, UINT clientWidth, UINT clientHeight) : m_visible(true), Layer("UILayer")
{
   srand((UINT)time(NULL));
   m_width = clientWidth;
   m_height = clientHeight;
   m_d2d = std::make_unique<DOG::gfx::D2DBackend_DX12>(rd, sc, numBuffers);
}

DOG::UI::~UI()
{

}

DOG::gfx::D2DBackend_DX12* DOG::UI::GetBackend()
{
   return m_d2d.get();
}

void DOG::UI::DrawUI()
{
   if (m_visible && m_scenes.size() > 0)
   {
      for (auto&& e : m_scenes[m_currsceneIndex]->GetScene())
      {
         e->Update(*m_d2d);
         e->Draw(*m_d2d);
      }
   }
}

void DOG::UI::OnEvent(IEvent& event)
{
   for (auto&& e : m_scenes[m_currsceneIndex]->GetScene())
      e->OnEvent(event);

}

void DOG::UI::Resize(UINT clientWidth, UINT clientHeight)
{
   m_width = clientWidth;
   m_height = clientHeight;
   m_d2d->OnResize();
}

void DOG::UI::FreeResize()
{
   for (auto&& e : m_scenes)
      e->GetScene().clear();

   m_d2d->FreeResize();
}

DOG::UI* DOG::UI::Get()
{
   assert(s_instance);
   return s_instance;
}

void DOG::UI::Initialize(DOG::gfx::RenderDevice* rd, DOG::gfx::Swapchain* sc, UINT numBuffers, UINT clientWidth, UINT clientHeight)
{
   if (!s_instance)
      s_instance = new UI(rd, sc, numBuffers, clientWidth, clientHeight);
}

void DOG::UI::Destroy()
{
   if (s_instance)
   {
      delete s_instance;
      s_instance = nullptr;
   }
}

/// @brief Generates a unique ID
/// @return Unique ID
UINT DOG::UI::GenerateUID()
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
UINT DOG::UI::QueryScene(UINT sceneID)
{
   UINT index;
   auto res = std::find_if(m_scenes.begin(), m_scenes.end(), [&](std::unique_ptr<UIScene> const& s) { return s->GetID() == sceneID; });
   if (res == m_scenes.end())
      return UINT_MAX;
   else
   {
      ptrdiff_t ptrdiff = std::distance(m_scenes.begin(), res);
      HRESULT hr = S_OK;
      hr = PtrdiffTToUInt(ptrdiff, &index);
      HR_VFY(hr);
      return index;
   }
}

/// @brief Adds a ui element to a specific UIscene
/// @param sceneID The ID of the scene
/// @param element The element to be added
/// @return The unique ID of the element. If the scene does not exist UINT_MAX is returned
UINT DOG::UI::AddUIElementToScene(UINT sceneID, std::unique_ptr<UIElement> element)
{
   UINT index = QueryScene(sceneID);
   UINT id;
   if (index == UINT_MAX)
      return UINT_MAX;
   else
   {
      id = element->GetID();
      m_scenes[index]->GetScene().push_back(std::move(element));
   }
   return id;
}

/// @brief Removes an element with a specific UID
/// @param elementID The ID of the element to be removed.
/// @return returns the ID of the scene in which the element was found. If the element is non existant UINT_MAX is returned
UINT DOG::UI::RemoveUIElement(UINT elementID)
{
   for (auto&& s : m_scenes)
   {
      auto res = std::find_if(s->GetScene().begin(), s->GetScene().end(), [&](std::unique_ptr<UIElement> const& e) { return e->GetID() == elementID; });
      if (res == s->GetScene().end())
         continue;
      else
      {
         s->GetScene().erase(res);
         return s->GetID();
      }
   }
   return UINT_MAX;
}

/// @brief Adds a scene to the UI manager
/// @return A unique ID for the newly created scene
UINT DOG::UI::AddScene()
{
   auto scene = std::make_unique<UIScene>(GenerateUID());
   UINT id = scene->GetID();
   m_scenes.push_back(std::move(scene));
   return id;
}

/// @brief Removes a scene with a specific ID
/// @param sceneID The ID of the scene to be removed
void DOG::UI::RemoveScene(UINT sceneID)
{
   UINT index = QueryScene(sceneID);
   if (index == UINT_MAX)
      return;
   else
      m_scenes.erase(m_scenes.begin() + index);
}

DOG::UIScene* DOG::UI::GetScene(UINT sceneID)
{
   for (auto&& s : m_scenes)
   {
      if (s->GetID() == sceneID)
         return s.get();
   }
   return nullptr;
}

/// @brief Changes the current scene to a scene with a specific ID
/// @param sceneID The ID of the scene to switch to
void DOG::UI::ChangeUIscene(UINT sceneID)
{
   UINT index = QueryScene(sceneID);
   if (index == UINT_MAX)
      return;
   else
   {
      m_currsceneIndex = index;
      m_currsceneID = sceneID;
   }
}

UINT DOG::UIScene::GetID()
{
   return m_ID;
}

void DOG::UIScene::OnEvent(IEvent& event)
{
   for (auto&& e : m_scene)
      e->OnEvent(event);

}

std::vector<std::unique_ptr<DOG::UIElement>>& DOG::UIScene::GetScene()
{
   return m_scene;
}

DOG::UIElement::UIElement(UINT id) : m_ID(id)
{

}



DOG::UIElement::~UIElement()
{
}

void DOG::UIElement::Update(DOG::gfx::D2DBackend_DX12& d2d)
{
   UNREFERENCED_PARAMETER(d2d);
}

UINT DOG::UIElement::GetID()
{
   return m_ID;
}

void DOG::UIElement::OnEvent(IEvent& event)
{
   UNREFERENCED_PARAMETER(event);
   return;
}

DOG::UIButton::UIButton(DOG::gfx::D2DBackend_DX12& d2d, UINT id, float x, float y, float width, float height, float fontSize, float r, float g, float b, const std::wstring& text, std::function<void(void)> callback) : pressed(false), m_callback(callback), UIElement(id)
{
   this->m_size = D2D1::Vector2F(width, height);
   m_textRect = D2D1::RectF(x, y, x + width, y + height);
   this->m_text = text;
   HRESULT hr = d2d.Get2DDeviceContext()->CreateSolidColorBrush(D2D1::ColorF(r,g,b, 1.0f), &m_brush);
   HR_VFY(hr);
   hr = d2d.GetDWriteFactory()->CreateTextFormat(
      L"Robot Radicals",
      NULL,
      DWRITE_FONT_WEIGHT_NORMAL,
      DWRITE_FONT_STYLE_NORMAL,
      DWRITE_FONT_STRETCH_NORMAL,
      fontSize,
      L"en-us",
      &m_format
   );
   HR_VFY(hr);
   hr = m_format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
   HR_VFY(hr);
   hr = m_format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
   HR_VFY(hr);
}

DOG::UIButton::~UIButton()
{

}

void DOG::UIButton::Draw(DOG::gfx::D2DBackend_DX12& d2d)
{
   d2d.Get2DDeviceContext()->DrawRectangle(m_textRect, m_brush.Get());
   d2d.Get2DDeviceContext()->DrawTextW(
      m_text.c_str(),
      (UINT32)m_text.length(),
      m_format.Get(),
      &m_textRect,
      m_brush.Get()
   );
}

void DOG::UIButton::OnEvent(IEvent& event)
{
   using namespace DOG;
   if (event.GetEventCategory() == EventCategory::MouseEventCategory)
   {
      if (event.GetEventType() == EventType::LeftMouseButtonPressedEvent)
      {
         auto mevent = EVENT(DOG::LeftMouseButtonPressedEvent);
         auto mpos = mevent.coordinates;
         if (mpos.x >= m_textRect.left && mpos.x <= m_textRect.right && mpos.y >= m_textRect.top && mpos.y <= m_textRect.bottom)
         {
            m_callback();
         }
      }
      else if (event.GetEventType() == EventType::MouseMovedEvent)
      {
         auto mevent = EVENT(DOG::MouseMovedEvent);
         auto mpos = mevent.coordinates;
         if (!(mpos.x >= m_textRect.left && mpos.x <= m_textRect.right && mpos.y >= m_textRect.top && mpos.y <= m_textRect.bottom))
            m_brush.Get()->SetOpacity(0.5f);
         else
            m_brush.Get()->SetOpacity(1.0f);
      }
   }
}

DOG::UIScene::UIScene(UINT id) : m_ID(id)
{

}


DOG::UISplashScreen::UISplashScreen(DOG::gfx::D2DBackend_DX12& d2d, UINT id, float width, float height) : UIElement(id)
{
   UNREFERENCED_PARAMETER(d2d);
   m_timer = clock();
   m_background = D2D1::RectF(0.0f, 0.0f, width, height);
   m_text = L"Disorganized Games";

   HRESULT hr = d2d.Get2DDeviceContext()->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black, 1.0f), &m_splashBrush);
   HR_VFY(hr);
   hr = d2d.Get2DDeviceContext()->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White, 1.0f), &m_textBrush);
   HR_VFY(hr);
   hr = d2d.GetDWriteFactory()->CreateTextFormat(
      L"Robot Radicals",
      NULL,
      DWRITE_FONT_WEIGHT_NORMAL,
      DWRITE_FONT_STYLE_NORMAL,
      DWRITE_FONT_STRETCH_NORMAL,
      50,
      L"en-us",
      &m_format
   );
   hr = m_format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
   HR_VFY(hr);
   hr = m_format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
   HR_VFY(hr);
   m_backOp = 1.0f;
   m_textOp = 0.0f;
}

void DOG::UISplashScreen::Draw(DOG::gfx::D2DBackend_DX12& d2d)
{
   d2d.Get2DDeviceContext()->FillRectangle(m_background, m_splashBrush.Get());
   d2d.Get2DDeviceContext()->DrawTextW(
      m_text.c_str(),
      (UINT32)m_text.length(),
      m_format.Get(),
      &m_background,
      m_textBrush.Get());
}
void DOG::UISplashScreen::Update(DOG::gfx::D2DBackend_DX12& d2d)
{
   UNREFERENCED_PARAMETER(d2d);
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

DOG::UISplashScreen::~UISplashScreen()
{

}


DOG::UIHealthBar::UIHealthBar(DOG::gfx::D2DBackend_DX12& d2d, UINT id, float x, float y, float width, float height) : UIElement(id)
{
   m_text = L"100%";
   m_value = m_test = 1.0f;
   m_barWidth = width - 2.f;
   m_border = D2D1::RectF(x, y, x + width, y + height);
   m_bar = D2D1::RectF(x + 2.0f, y + 2.0f, x + width - 2.f, y + height - 2.f);
   HRESULT hr = d2d.Get2DDeviceContext()->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White, 1.0f), &m_borderBrush);
   HR_VFY(hr);
   hr = d2d.Get2DDeviceContext()->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::GreenYellow, 0.5f), &m_barBrush);
   HR_VFY(hr);
   hr = d2d.GetDWriteFactory()->CreateTextFormat(
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

DOG::UIHealthBar::~UIHealthBar()
{

}

void DOG::UIHealthBar::Draw(DOG::gfx::D2DBackend_DX12& d2d)
{
   if (m_value > 0.0f)
      d2d.Get2DDeviceContext()->FillRectangle(m_bar, m_barBrush.Get());
   d2d.Get2DDeviceContext()->DrawRectangle(m_border, m_barBrush.Get());
   d2d.Get2DDeviceContext()->DrawTextW(
      m_text.c_str(),
      (UINT32)m_text.length(),
      m_textFormat.Get(),
      &m_border,
      m_borderBrush.Get());

}
void DOG::UIHealthBar::Update(DOG::gfx::D2DBackend_DX12& d2d)
{
   UNREFERENCED_PARAMETER(d2d);
   m_bar.right = m_value * m_barWidth + m_bar.left - 2.0f;
   m_text = std::to_wstring((UINT)(m_value * 100.f)) + L'%';
}
void DOG::UIHealthBar::SetBarValue(float value)
{
   m_value = value;
}

DOG::UIBackground::UIBackground(DOG::gfx::D2DBackend_DX12& d2d, UINT id, float width, float heigt, const std::wstring& title) : UIElement(id)
{
   m_title = title;
   m_background = D2D1::RectF(0.0f, 0.0f, width, heigt);
   m_textRect = D2D1::RectF(width / 2 - 350.f / 2, heigt / 2 - 200.f, width / 2 + 300.f, heigt / 2 - 50.f);
   HRESULT hr = d2d.Get2DDeviceContext()->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &m_textBrush);
   HR_VFY(hr);
   hr = d2d.Get2DDeviceContext()->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &m_backBrush);
   HR_VFY(hr);
   hr = d2d.GetDWriteFactory()->CreateTextFormat(
      L"Robot Radicals",
      NULL,
      DWRITE_FONT_WEIGHT_NORMAL,
      DWRITE_FONT_STYLE_NORMAL,
      DWRITE_FONT_STRETCH_NORMAL,
      50,
      L"en-us",
      &m_textFormat
   );
   HR_VFY(hr);
   hr = m_textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
   HR_VFY(hr);
}

DOG::UIBackground::~UIBackground()
{

}

void DOG::UIBackground::Draw(DOG::gfx::D2DBackend_DX12& d2d)
{
   d2d.Get2DDeviceContext()->FillRectangle(m_background, m_backBrush.Get());
   d2d.Get2DDeviceContext()->DrawTextW(
      m_title.c_str(),
      (UINT32)m_title.length(),
      m_textFormat.Get(),
      &m_textRect,
      m_textBrush.Get());
}
void DOG::UIBackground::Update(DOG::gfx::D2DBackend_DX12& d2d)
{
   UNREFERENCED_PARAMETER(d2d);

}

DOG::UICrosshair::UICrosshair(DOG::gfx::D2DBackend_DX12& d2d, UINT id) : UIElement(id)
{
   HRESULT hr = d2d.Get2DDeviceContext()->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White, 0.3f), &m_brush);
   HR_VFY(hr);
   hr = d2d.GetDWriteFactory()->CreateTextFormat(
      L"Arial",
      NULL,
      DWRITE_FONT_WEIGHT_ULTRA_LIGHT,
      DWRITE_FONT_STYLE_NORMAL,
      DWRITE_FONT_STRETCH_NORMAL,
      30,
      L"en-us",
      &m_textFormat
   );
   HR_VFY(hr);
   hr = m_textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
   HR_VFY(hr);
   hr = m_textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
   HR_VFY(hr);
   auto pixsize = d2d.GetRTPixelSize();
   m_screenSize = D2D1::RectF(0.0f, 0.0f, (FLOAT)pixsize.width, (FLOAT)pixsize.height);

}

DOG::UICrosshair::~UICrosshair()
{

}

void DOG::UICrosshair::Draw(DOG::gfx::D2DBackend_DX12& d2d)
{
   d2d.Get2DDeviceContext()->DrawTextW(
      L"+",
      1u,
      m_textFormat.Get(),
      m_screenSize,
      m_brush.Get());
}

DOG::UITextField::UITextField(DOG::gfx::D2DBackend_DX12& d2d, UINT id, float x, float y, float width, float height) : UIElement(id)
{
   m_displayText = L"  IP";
   m_active = false;
   m_border = m_background = D2D1::RectF(x, y, x + width, y + height);
   //m_bar = D2D1::RectF(x + 2.0f, y + 2.0f, x + width - 2.f, y + height - 2.f);
   HRESULT hr = d2d.Get2DDeviceContext()->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White, 1.0f), &m_borderBrush);
   HR_VFY(hr);
   hr = d2d.Get2DDeviceContext()->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Gray, 0.3f), &m_backBrush);
   HR_VFY(hr);
   hr = d2d.Get2DDeviceContext()->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::GhostWhite, 0.7f), &m_textBrush);
   HR_VFY(hr);
   hr = d2d.GetDWriteFactory()->CreateTextFormat(
      L"Robot Radicals",
      NULL,
      DWRITE_FONT_WEIGHT_NORMAL,
      DWRITE_FONT_STYLE_NORMAL,
      DWRITE_FONT_STRETCH_NORMAL,
      14,
      L"en-us",
      &m_textFormat
   );
   HR_VFY(hr);
   hr = m_textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
   HR_VFY(hr);
}

DOG::UITextField::~UITextField()
{

}

void DOG::UITextField::Draw(DOG::gfx::D2DBackend_DX12& d2d)
{
   d2d.Get2DDeviceContext()->DrawRectangle(m_border, m_borderBrush.Get());
   d2d.Get2DDeviceContext()->FillRectangle(m_background, m_backBrush.Get());
   if (m_active)
   {
      m_textBrush->SetOpacity(0.7f);
      d2d.Get2DDeviceContext()->DrawTextW(
         m_text.c_str(),
         (UINT32)m_text.length(),
         m_textFormat.Get(),
         &m_border,
         m_textBrush.Get());

   }
   else if (!m_active && m_text.length() == 0)
   {
      m_textBrush->SetOpacity(0.3f);
      d2d.Get2DDeviceContext()->DrawTextW(
         m_displayText.c_str(),
         (UINT32)m_displayText.length(),
         m_textFormat.Get(),
         &m_border,
         m_textBrush.Get());
   }
   else
   {
      m_textBrush->SetOpacity(0.3f);
      d2d.Get2DDeviceContext()->DrawTextW(
         m_text.c_str(),
         (UINT32)m_text.length(),
         m_textFormat.Get(),
         &m_border,
         m_textBrush.Get());
   }
}

void DOG::UITextField::OnEvent(DOG::IEvent& event)
{
   using namespace DOG;
   if (event.GetEventCategory() == EventCategory::KeyboardEventCategory || event.GetEventCategory() == EventCategory::MouseEventCategory)
   {
      switch (event.GetEventType())
      {
      case EventType::KeyPressedEvent:
         if (m_active)
         {
            int c = static_cast<int>(EVENT(KeyPressedEvent).key);
            if (c == 190)
               m_text += L'.';
            else if (isgraph(c) != 0)
            {
               wchar_t character = static_cast<wchar_t>(c);
               m_text += character;
            }
            else if (EVENT(KeyPressedEvent).key == DOG::Key::BackSpace && m_text.length() > 0)
               m_text.pop_back();
         }
         break;
      case EventType::LeftMouseButtonPressedEvent:
         {
            auto mevent = EVENT(DOG::LeftMouseButtonPressedEvent);
            auto mpos = mevent.coordinates;
            if (mpos.x >= m_border.left && mpos.x <= m_border.right && mpos.y >= m_border.top && mpos.y <= m_border.bottom)
               m_active = true;
            if (!(mpos.x >= m_border.left && mpos.x <= m_border.right && mpos.y >= m_border.top && mpos.y <= m_border.bottom) && m_active)
               m_active = false;
            break;

         }
      default:
         break;
      }
   }

}

/// @brief Getter for UITextfield.
/// @return The text currently in the text field.
std::wstring DOG::UITextField::GetText()
{
   return m_text;
}


void UIRebuild(UINT clientHeight, UINT clientWidth)
{
   auto instance = DOG::UI::Get();

   //HealthBar
   auto h = instance->Create<DOG::UIHealthBar, float, float, float, float>(hID, 40.f, clientHeight - 60.f, 250.f, 30.f);
   instance->AddUIElementToScene(gameID, std::move(h));

   //Crosshair
   auto c = instance->Create<DOG::UICrosshair>(cID);
   instance->AddUIElementToScene(gameID, std::move(c));


   //Menu backgrounds

   auto menuBack = instance->Create<DOG::UIBackground, float, float, std::wstring>(menuBackID, (FLOAT)clientWidth, (FLOAT)clientHeight, std::wstring(L"Rogue Robots"));
   instance->AddUIElementToScene(menuID, std::move(menuBack));
   auto optionsBack = instance->Create<DOG::UIBackground, float, float, std::wstring>(optionsBackID, (FLOAT)clientWidth, (FLOAT)clientHeight, std::wstring(L"Options"));
   instance->AddUIElementToScene(optionsID, std::move(optionsBack));
   auto multiBack = instance->Create<DOG::UIBackground, float, float, std::wstring>(multiBackID, (FLOAT)clientWidth, (FLOAT)clientHeight, std::wstring(L"Multiplayer"));
   instance->AddUIElementToScene(multiID, std::move(multiBack));

   auto t = instance->Create<DOG::UITextField, float, float, float, float>(tID, (FLOAT)clientWidth / 2.f - 250.f / 2, (FLOAT)clientHeight / 2.f, 250.f, 30.f);
   instance->AddUIElementToScene(multiID, std::move(t));

   //Menu buttons
   auto bp = instance->Create<DOG::UIButton, float, float, float, float, float,float, float, float, std::wstring>(bpID, (FLOAT)clientWidth / 2.f - 150.f / 2, (FLOAT)clientHeight / 2.f, 150.f, 60.f, 20.f,0.0f,1.0f,0.0f, std::wstring(L"Play"), std::function<void()>(PlayButtonFunc));
   auto bm = instance->Create<DOG::UIButton, float, float, float, float, float,float, float, float, std::wstring>(bmID, (FLOAT)clientWidth / 2.f - 150.f / 2, (FLOAT)clientHeight / 2.f + 70.f, 150.f, 60.f, 20.f,1.0f,1.0f,1.0f, std::wstring(L"Multiplayer"), std::function<void()>(MultiplayerButtonFunc));
   auto bo = instance->Create<DOG::UIButton, float, float, float, float, float,float, float, float, std::wstring>(boID, (FLOAT)clientWidth / 2.f - 150.f / 2, (FLOAT)clientHeight / 2.f + 140.f, 150.f, 60.f, 20.f,1.0f,1.0f,1.0f, std::wstring(L"Options"), std::function<void()>(OptionsButtonFunc));
   auto be = instance->Create<DOG::UIButton, float, float, float, float, float,float, float, float, std::wstring>(beID, (FLOAT)clientWidth / 2.f - 150.f / 2, (FLOAT)clientHeight / 2.f + 210.f, 150.f, 60.f, 20.f,1.0f,1.0f,1.0f, std::wstring(L"Exit"), std::function<void()>(ExitButtonFunc));
   auto optback = instance->Create<DOG::UIButton, float, float, float, float, float,float, float, float, std::wstring>(optbackID, (FLOAT)clientWidth / 2.f - 150.f / 2, (FLOAT)clientHeight / 2.f + 210.f, 150.f, 60.f, 20.f,1.0f,1.0f,1.0f, std::wstring(L"Back"), std::function<void()>(ToMenuButtonFunc));
   auto mulback = instance->Create<DOG::UIButton, float, float, float, float, float,float, float, float, std::wstring>(mulbackID, (FLOAT)clientWidth / 2.f - 150.f / 2, (FLOAT)clientHeight / 2.f + 250.f, 150.f, 60.f, 20.f,1.0f,1.0f,1.0f, std::wstring(L"Back"), std::function<void()>(ToMenuButtonFunc));

   auto bh = instance->Create<DOG::UIButton, float, float, float, float, float,float, float, float, std::wstring>(bhID, (FLOAT)clientWidth / 2.f - 75.f - 100.f, (FLOAT)clientHeight / 2.f + 140.f, 150.f, 60.f, 20.f,1.0f,1.0f,1.0f, std::wstring(L"Host"), std::function<void()>(ToMenuButtonFunc));
   auto bj = instance->Create<DOG::UIButton, float, float, float, float, float,float, float, float, std::wstring>(bjID, (FLOAT)clientWidth / 2.f - 75.f + 100.f, (FLOAT)clientHeight / 2.f + 140.f, 150.f, 60.f, 20.f,1.0f,1.0f,1.0f, std::wstring(L"Join"), std::function<void()>(ToMenuButtonFunc));
   instance->AddUIElementToScene(menuID, std::move(bp));
   instance->AddUIElementToScene(menuID, std::move(bm));
   instance->AddUIElementToScene(menuID, std::move(bo));
   instance->AddUIElementToScene(menuID, std::move(be));
   instance->AddUIElementToScene(optionsID, std::move(optback));
   instance->AddUIElementToScene(multiID, std::move(mulback));
   instance->AddUIElementToScene(multiID, std::move(bh));
   instance->AddUIElementToScene(multiID, std::move(bj));


   //Splash screen
   // UINT sID;
   // auto s = DOG::UI::Get().Create<DOG::UISplashScreen, float, float>(sID, (float)clientWidth, (float)clientHeight);
   // DOG::UI::Get().AddUIElementToScene(menuID, std::move(s));

}

void AddScenes()
{
   auto instance = DOG::UI::Get();
   menuID = instance->AddScene();
   gameID = instance->AddScene();
   multiID = instance->AddScene();
   optionsID = instance->AddScene();
   instance->ChangeUIscene(menuID);
}

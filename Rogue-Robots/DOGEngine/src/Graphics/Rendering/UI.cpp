#include "UI.h"
#include "../../Core/Time.h"
#include "../../Core/Window.h"
#include "../../Input/Mouse.h"
#include "../../Input/Keyboard.h"
#include "../../EventSystem/KeyboardEvents.h"
#include "../../EventSystem/MouseEvents.h"



DOG::UI* DOG::UI::s_instance = nullptr;

UINT menuID, gameID, optionsID, multiID, lobbyID, joinID, WaitingForHostID, GameOverID, WinScreenID, LoadingID, creditsID, levelSelectSoloID, levelSelectMultID;
UINT menuBackID, optionsBackID, multiBackID, hostBackID, creditsBackID, levelSelectSoloBackID, levelSelectMultBackID;
UINT bStartLevelSelectorSoloID, bStartLevelSelectorMultID, bGoBackLevelSelectorSoloID, bGoBackLevelSelectorMultID;
UINT bpID, bmID, boID, beID, mulbackID, bhID, bjID, r1ID, r2ID, r3ID, r4ID, r5ID, r6ID, r7ID, r8ID, r9ID, r10ID, l1ID, l2ID, l3ID, l4ID, l5ID, l6ID, bjjID,
lWinTextID, lredScoreID, lblueScoreID, lgreenScoreID, lyellowScoreID, lredScoreWinID, lblueScoreWinID, lgreenScoreWinID, lyellowScoreWinID, pbarID;
UINT lNamesCreditsID, lTheTeamID, lFiverrArtistsID, lFiverrArtistsTextID, lIconsCreditsID, lIconsCreditsTextID, lMusicID, lMusicTextID;
UINT lStartTextID;
UINT bcID, credbackID;
UINT cID, tID, hID, carouselSoloID, carouselMultID;
UINT iconID, icon2ID, icon3ID, iconGun, iconActiveID, lActiveItemTextID, flashlightID, glowstickID; //Icons.
UINT buffID;
UINT playerListID, playerListJoinID;
UINT lAcknowledgementsID, lAcknowledgementsTextID;
UINT ipBarID;
UINT bpLobbyID;

std::vector<bool> buffsVisible;
std::vector<UINT> m_stacks;

void LevelSelectSoloButtonFunc(void)
{
   DOG::UI::Get()->ChangeUIscene(levelSelectSoloID);
}

void LevelSelectMultButtonFunc(void)
{
   DOG::UI::Get()->ChangeUIscene(levelSelectMultID);
}

void PlayButtonFunc(void);

void SliderFunc(float value)
{
   UNREFERENCED_PARAMETER(value);
   return;
}


void OptionsButtonFunc(void)
{
   DOG::UI::Get()->ChangeUIscene(optionsID);
}

void CreditsButtonFunc(void)
{
   DOG::UI::Get()->ChangeUIscene(creditsID);
}

void MultiplayerButtonFunc(void)
{
   DOG::UI::Get()->ChangeUIscene(multiID);
}

void HostButtonFunc(void);

void BackFromHost(void);

void HostLaunch(void);

void ToMenuButtonFunc(void)
{
   DOG::UI::Get()->ChangeUIscene(menuID);
}

void CheckBoxFunc(bool value)
{
   UNREFERENCED_PARAMETER(value);
}

void JoinButton(void);
void Room1Button(void);
void Room2Button(void);
void Room3Button(void);
void Room4Button(void);
void Room5Button(void);
void Room6Button(void);
void Room7Button(void);
void Room8Button(void);
void Room9Button(void);
void Room10Button(void);

void ExitButtonFunc(void)
{
   DOG::Window::CloseWindow();
}

DOG::UI::UI(DOG::gfx::RenderDevice* rd, DOG::gfx::Swapchain* sc, UINT numBuffers, UINT clientWidth, UINT clientHeight): m_visible(true), Layer("UILayer")
{
   //srand((UINT)time(NULL));
   int err;
   err = AddFontResource(TEXT("Assets/Fonts/robotaur.ttf"));
   Sleep(1000);
   assert(err);
   m_width = clientWidth;
   m_height = clientHeight;
   m_d2d = std::make_unique<DOG::gfx::D2DBackend_DX12>(rd, sc, numBuffers);
}

DOG::UI::~UI()
{
   RemoveFontResource(TEXT("Assets/Fonts/robotaur.ttf"));
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

   //UIRebuild(clientWidth, clientHeight);
}

void DOG::UI::Destroy()
{
   if (s_instance)
   {
      delete s_instance;
      s_instance = nullptr;
   }
   RemoveFontResource(TEXT("Assets/Fonts/robotaur.ttf"));
}

std::vector<std::function<void(u32, u32)>>& DOG::UI::GetExternalUI()
{
   return m_externUI;
}

void DOG::UI::AddExternalUI(std::function<void(u32, u32)>&& createFunc)
{
   m_externUI.emplace_back(createFunc)(m_width, m_height);
}


UINT DOG::UI::GetActiveUIScene() const
{
   return m_currsceneID;
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

DOG::UIElement::UIElement(UINT id): m_ID(id)
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

DOG::UIButton::UIButton(DOG::gfx::D2DBackend_DX12& d2d, UINT id, float x, float y, float width, float height, float fontSize, float r, float g, float b, const std::wstring& text, std::function<void(void)> callback): pressed(false), m_callback(callback), UIElement(id)
{
   m_show = true;

   this->m_size = D2D1::Vector2F(width, height);
   m_textRect = D2D1::RectF(x, y, x + width, y + height);
   this->m_text = text;
   HRESULT hr = d2d.Get2DDeviceContext()->CreateSolidColorBrush(D2D1::ColorF(r, g, b, 1.0f), &m_brush);
   HR_VFY(hr);
   hr = d2d.GetDWriteFactory()->CreateTextFormat(
      L"robotaur",
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
   if (m_show)
   {
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
}

void DOG::UIButton::Show(bool mode)
{
    m_show = mode;
}

DOG::UIScene::UIScene(UINT id): m_ID(id)
{

}


DOG::UISplashScreen::UISplashScreen(DOG::gfx::D2DBackend_DX12& d2d, UINT id, float width, float height): UIElement(id)
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
      L"robotaur",
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


DOG::UIHealthBar::UIHealthBar(DOG::gfx::D2DBackend_DX12& d2d, UINT id, float x, float y, float width, float height, float textSize): UIElement(id)
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
      L"robotaur",
      NULL,
      DWRITE_FONT_WEIGHT_NORMAL,
      DWRITE_FONT_STYLE_NORMAL,
      DWRITE_FONT_STRETCH_NORMAL,
      textSize,
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
   m_bar.right = (m_value / m_maxValue) * m_barWidth + m_bar.left - 2.0f;
   m_text = std::to_wstring((UINT)(m_value * (m_value >= 0)));
   m_text += L"/";
   m_text += std::to_wstring((UINT)(m_maxValue));
   m_text += L" HP";
}
void DOG::UIHealthBar::SetBarValue(float value, float maxValue)
{
   m_value = value;
   m_maxValue = maxValue;
}

DOG::UIBackground::UIBackground(DOG::gfx::D2DBackend_DX12& d2d, UINT id, float width, float heigt, const std::wstring& title, float left, float top): UIElement(id)
{
   m_title = title;
   m_background = D2D1::RectF(left, top, left + width, top + heigt);
   m_textRect = D2D1::RectF(width / 2 - 600.f, heigt / 2 - 200.f, width / 2 + 600.f, heigt / 2 - 50.f);
   HRESULT hr = d2d.Get2DDeviceContext()->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &m_textBrush);
   HR_VFY(hr);
   hr = d2d.Get2DDeviceContext()->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &m_backBrush);
   HR_VFY(hr);
   hr = d2d.GetDWriteFactory()->CreateTextFormat(
      L"robotaur",
      NULL,
      DWRITE_FONT_WEIGHT_NORMAL,
      DWRITE_FONT_STYLE_NORMAL,
      DWRITE_FONT_STRETCH_NORMAL,
      50,
      L"en-us",
      &m_textFormat
   );
   HR_VFY(hr);
   hr = m_textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
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

DOG::UICrosshair::UICrosshair(DOG::gfx::D2DBackend_DX12& d2d, UINT id): UIElement(id)
{
   HRESULT hr = d2d.Get2DDeviceContext()->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White, 0.3f), &m_brush);
   HR_VFY(hr);
   hr = d2d.GetDWriteFactory()->CreateTextFormat(
      L"robotaur",
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

DOG::UITextField::UITextField(DOG::gfx::D2DBackend_DX12& d2d, UINT id, float x, float y, float width, float height): UIElement(id)
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
      L"robotaur",
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

DOG::UIBuffTracker::UIBuffTracker(DOG::gfx::D2DBackend_DX12& d2d, UINT id, std::vector<std::wstring> filePaths): UIElement(id)
{
   UNREFERENCED_PARAMETER(d2d);
   ComPtr<IWICBitmapDecoder> m_decoder;
   ComPtr<IWICImagingFactory> m_imagingFactory;
   ComPtr<IWICBitmapFrameDecode> m_frame;
   ComPtr<IWICFormatConverter> m_converter;
   m_buffs = 0;
   HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_IWICImagingFactory, (LPVOID*)m_imagingFactory.GetAddressOf());
   HR_VFY(hr);
   auto rect = D2D1::RectF(50.f, 50.f, 100.f, 100.f);
   for (auto&& path : filePaths)
   {
      hr = m_imagingFactory->CreateDecoderFromFilename(
         path.c_str(),                            // Image to be decoded
         NULL,                            // Do not prefer a particular vendor
         GENERIC_READ,                    // Desired read access to the file
         WICDecodeMetadataCacheOnDemand,  // Cache metadata when needed
         m_decoder.GetAddressOf()                        // Pointer to the decoder
      );
      HR_VFY(hr);
      hr = m_decoder->GetFrame(0, m_frame.GetAddressOf());
      HR_VFY(hr);
      hr = m_imagingFactory->CreateFormatConverter(m_converter.GetAddressOf());
      HR_VFY(hr);
      hr = m_converter->Initialize(
         m_frame.Get(),                   // Input bitmap to convert
         GUID_WICPixelFormat32bppPBGRA,   // Destination pixel format
         WICBitmapDitherTypeNone,         // Specified dither pattern
         NULL,                            // Specify a particular palette
         0.f,                             // Alpha threshold
         WICBitmapPaletteTypeCustom       // Palette translation type
      );
      HR_VFY(hr);
      ComPtr<ID2D1Bitmap> bitmap;
      hr = d2d.Get2DDeviceContext()->CreateBitmapFromWicBitmap(m_converter.Get(), NULL, bitmap.GetAddressOf());
      HR_VFY(hr);
      m_bitmaps.push_back(bitmap);
      m_rects.push_back(rect);
      rect.left += 60.f;
      rect.right += 60.f;
      buffsVisible.push_back(false);
      m_animate.push_back(false);
      m_opacity.push_back(1.0f);
      m_stacks.push_back(0u);
      m_buffs++;
   }
   hr = d2d.Get2DDeviceContext()->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White, 1.0f), m_borderBrush.GetAddressOf());
   HR_VFY(hr);
   hr = d2d.GetDWriteFactory()->CreateTextFormat(
      L"robotaur",
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
   hr = m_textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
   HR_VFY(hr);
}

void DOG::UIBuffTracker::Draw(DOG::gfx::D2DBackend_DX12& d2d)
{
   for (size_t i = 0; i < m_buffs; i++)
   {
      if (buffsVisible[i])
      {
         d2d.Get2DDeviceContext()->DrawBitmap(m_bitmaps[i].Get(), m_rects[i], m_opacity[i]);
         d2d.Get2DDeviceContext()->DrawRectangle(m_rects[i], m_borderBrush.Get());
         if (m_stacks[i] > 1u)
         {
            D2D1_RECT_F textRect = m_rects[i];
            textRect.top += 40.f;
            textRect.bottom += 40.f;
            d2d.Get2DDeviceContext()->DrawTextW(std::wstring(std::to_wstring(m_stacks[i]) + L'x').c_str(), (UINT32)std::to_wstring(m_stacks[i]).length() + 1u, m_textFormat.Get(), textRect, m_borderBrush.Get());
         }
      }
   }

}
void DOG::UIBuffTracker::Update(DOG::gfx::D2DBackend_DX12& d2d)
{
   UNREFERENCED_PARAMETER(d2d);
   for (UINT i = 0; i < m_buffs; i++)
      if (m_animate[i])
         AnimateUp(i);

}

DOG::UIBuffTracker::~UIBuffTracker()
{

}

void DOG::UIBuffTracker::AnimateUp(UINT index)
{
   if (m_rects[index].top >= 20.f)
   {
      m_rects[index].top -= 100.0f * (float)DOG::Time::DeltaTime();
      m_rects[index].bottom -= 100.0f * (float)DOG::Time::DeltaTime();
   }
   if (m_opacity[index] <= 1.f)
      m_opacity[index] += 6.0f * (float)DOG::Time::DeltaTime();

   if (m_rects[index].top <= 20.f and m_opacity[index] >= 1.0f)
   {
      m_rects[index].top = 20.f;
      m_rects[index].bottom = m_rects[index].top + 50.0f;
      m_opacity[index] = 1.0f;
      m_animate[index] = false;
   }
}

void DOG::UIBuffTracker::ActivateIcon(UINT index)
{
   if (m_stacks[index] == 0)
   {
      buffsVisible[index] = true;
      size_t activeBuffs = std::count(buffsVisible.begin(), buffsVisible.end(), true);
      float x = 20.f + 80.f * (activeBuffs - 1);
      float y = 50.f + 30.f;
      m_rects[index] = D2D1::RectF(x, y, x + 50.f, y + 50.f);
      m_opacity[index] = 0.01f;
      m_animate[index] = true;
   }
   m_stacks[index]++;
}

void DOG::UIBuffTracker::DeactivateIcon(UINT index)
{
   buffsVisible[index] = false;
   for (size_t i = 0; i < m_buffs; i++)
   {
      if (m_rects[i].left > m_rects[index].left)
      {
         m_rects[i].left -= 60.f;
         m_rects[i].right -= 60.f;
      }
   }
   m_stacks[index] = 0;
}

DOG::UIPlayerList::UIPlayerList(DOG::gfx::D2DBackend_DX12& d2d, UINT id): UIElement(id)
{
   m_screensize = d2d.GetRTPixelSize();
   HRESULT hr = d2d.Get2DDeviceContext()->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White, 0.3f), &m_rectBrush);
   HR_VFY(hr);
   hr = d2d.GetDWriteFactory()->CreateTextFormat(
      L"robotaur",
      NULL,
      DWRITE_FONT_WEIGHT_NORMAL,
      DWRITE_FONT_STYLE_NORMAL,
      DWRITE_FONT_STRETCH_NORMAL,
      20,
      L"en-us",
      &m_textFormat
   );
   HR_VFY(hr);
   hr = m_textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
   HR_VFY(hr);
   hr = m_textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
}

DOG::UIPlayerList::~UIPlayerList()
{

}

void DOG::UIPlayerList::Draw(DOG::gfx::D2DBackend_DX12& d2d)
{
   auto rect = D2D1::RectF(m_screensize.width / 2 - 150.f, m_screensize.height / 2 - 20.f, m_screensize.width / 2 + 150.f, m_screensize.height / 2 + 20.f);
   auto colourrect = D2D1::RectF(m_screensize.width / 2 - 200.f, m_screensize.height / 2 - 20.f, m_screensize.width / 2 - 160.f, m_screensize.height / 2 + 20.f);
   for (size_t i = 0; i < m_players.size(); i++)
   {
      d2d.Get2DDeviceContext()->FillRectangle(rect, m_rectBrush.Get());
      m_rectBrush->SetColor(m_playerColours[i]);
      d2d.Get2DDeviceContext()->FillRectangle(colourrect, m_rectBrush.Get());
      m_rectBrush->SetColor(D2D1::ColorF(D2D1::ColorF::White, 0.3f));
      d2d.Get2DDeviceContext()->DrawTextW(
         m_players[i].c_str(),
         (UINT32)m_players[i].length(),
         m_textFormat.Get(),
         &rect,
         m_rectBrush.Get());
      rect.top += 60.f;
      rect.bottom += 60.f;
      colourrect.top += 60.f;
      colourrect.bottom += 60.f;
   }
}
void DOG::UIPlayerList::Update(DOG::gfx::D2DBackend_DX12& d2d)
{
   UNREFERENCED_PARAMETER(d2d);
}
void DOG::UIPlayerList::AddPlayer(const float r, const float g, const float b, const std::wstring name)
{
   m_players.push_back(name);
   m_playerColours.push_back(D2D1::ColorF(r, g, b, 0.3f));
   return;
}
void DOG::UIPlayerList::RemovePlayer(const std::wstring name)
{
   for (size_t i = 0; i < m_players.size(); i++)
   {
      if (m_players[i] == name)
      {
         m_players.erase(m_players.begin() + i);
         m_playerColours.erase(m_playerColours.begin() + i);
         return;
      }
   }
}

void DOG::UIPlayerList::Reset()
{
   m_players.clear();
   m_playerColours.clear();
   return;
}

DOG::UILabel::UILabel(DOG::gfx::D2DBackend_DX12& d2d, UINT id, std::wstring text, float x, float y, float width, float height, float size, DWRITE_TEXT_ALIGNMENT alignment): UIElement(id)
{
   m_draw = true;
   UNREFERENCED_PARAMETER(d2d);
   m_text = text;
   m_rect = D2D1::RectF(x, y, x + width, y + height);
   HRESULT hr = d2d.Get2DDeviceContext()->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White, 0.7f), &m_textBrush);
   HR_VFY(hr);
   hr = d2d.GetDWriteFactory()->CreateTextFormat(
      L"robotaur",
      NULL,
      DWRITE_FONT_WEIGHT_NORMAL,
      DWRITE_FONT_STYLE_NORMAL,
      DWRITE_FONT_STRETCH_NORMAL,
      size,
      L"en-us",
      &m_textFormat
   );
   HR_VFY(hr);
   hr = m_textFormat->SetTextAlignment(alignment);
   HR_VFY(hr);
   hr = m_textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

}

void DOG::UILabel::Update(DOG::gfx::D2DBackend_DX12& d2d)
{
   UNREFERENCED_PARAMETER(d2d);
   return;
}

void DOG::UILabel::SetDraw(bool draw)
{
   m_draw = draw;
}

void DOG::UILabel::Draw(DOG::gfx::D2DBackend_DX12& d2d)
{
   if (!m_draw)
      return;
   d2d.Get2DDeviceContext()->DrawTextW(
      m_text.c_str(),
      (UINT32)m_text.length(),
      m_textFormat.Get(),
      &m_rect,
      m_textBrush.Get());
}
DOG::UILabel::~UILabel()
{

}
void DOG::UILabel::SetText(std::wstring text)
{
   m_text = text;
}

DOG::UICarousel::UICarousel(DOG::gfx::D2DBackend_DX12& d2d, UINT id, std::vector<std::wstring> labels, float x, float y, float width, float height, float fontSize): UIElement(id)
{
   m_rect = D2D1::RectF(x, y, x + width, y + height);
   m_bright = D2D1::RectF(x + width + 10.f, y, x + width + 40.f, y + height);
   m_bleft = D2D1::RectF(x - 40.f, y, x - 10.f, y + height);
   HRESULT hr = d2d.Get2DDeviceContext()->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White, 0.5f), &m_rborderBrush);
   HR_VFY(hr);
   hr = d2d.Get2DDeviceContext()->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White, 0.5f), &m_lborderBrush);
   HR_VFY(hr);
   hr = d2d.Get2DDeviceContext()->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White, 1.0f), &m_textBrush);
   HR_VFY(hr);
   hr = d2d.Get2DDeviceContext()->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White, 0.5f), &m_borderBrush);
   HR_VFY(hr);
   hr = d2d.GetDWriteFactory()->CreateTextFormat(
      L"robotaur",
      NULL,
      DWRITE_FONT_WEIGHT_ULTRA_LIGHT,
      DWRITE_FONT_STYLE_NORMAL,
      DWRITE_FONT_STRETCH_NORMAL,
      fontSize,
      L"en-us",
      &m_textFormat
   );
   HR_VFY(hr);
   hr = m_textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
   HR_VFY(hr);
   hr = m_textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
   HR_VFY(hr);

   m_labels = labels;
   m_index = 0;

}
DOG::UICarousel::~UICarousel()
{

}
void DOG::UICarousel::Draw(DOG::gfx::D2DBackend_DX12& d2d)
{
   d2d.Get2DDeviceContext()->DrawRectangle(m_rect, m_borderBrush.Get());
   d2d.Get2DDeviceContext()->DrawRectangle(m_bright, m_rborderBrush.Get());
   d2d.Get2DDeviceContext()->DrawRectangle(m_bleft, m_lborderBrush.Get());
   d2d.Get2DDeviceContext()->DrawTextW(
      m_labels[m_index].c_str(),
      (UINT32)m_labels[m_index].length(),
      m_textFormat.Get(),
      &m_rect,
      m_textBrush.Get());
}
void DOG::UICarousel::Update(DOG::gfx::D2DBackend_DX12& d2d)
{
   UNREFERENCED_PARAMETER(d2d);
}

void DOG::UICarousel::OnEvent(IEvent& event)
{
   using namespace DOG;
   if (event.GetEventCategory() == EventCategory::MouseEventCategory)
   {
      if (event.GetEventType() == EventType::LeftMouseButtonPressedEvent)
      {
         auto mevent = EVENT(DOG::LeftMouseButtonPressedEvent);
         auto mpos = mevent.coordinates;
         if (mpos.x >= m_bright.left && mpos.x <= m_bright.right && mpos.y >= m_bright.top && mpos.y <= m_bright.bottom)
         {
            if (m_index == m_labels.size() - 1)
               m_index = 0;
            else
               m_index++;
         }
         if (mpos.x >= m_bleft.left && mpos.x <= m_bleft.right && mpos.y >= m_bleft.top && mpos.y <= m_bleft.bottom)
         {
            if (m_index == 0)
               m_index = (UINT)m_labels.size() - 1;
            else
               m_index--;
         }
      }
      else if (event.GetEventType() == EventType::MouseMovedEvent)
      {
         auto mevent = EVENT(DOG::MouseMovedEvent);
         auto mpos = mevent.coordinates;
         if (!(mpos.x >= m_bright.left && mpos.x <= m_bright.right && mpos.y >= m_bright.top && mpos.y <= m_bright.bottom))
            m_rborderBrush.Get()->SetOpacity(0.5f);
         else
            m_rborderBrush.Get()->SetOpacity(1.0f);
         if (!(mpos.x >= m_bleft.left && mpos.x <= m_bleft.right && mpos.y >= m_bleft.top && mpos.y <= m_bleft.bottom))
            m_lborderBrush.Get()->SetOpacity(0.5f);
         else
            m_lborderBrush.Get()->SetOpacity(1.0f);

      }
   }
}

std::wstring DOG::UICarousel::GetText(void)
{
   return m_labels[m_index];
}

UINT DOG::UICarousel::GetIndex()
{
   return m_index;
}

void DOG::UICarousel::SetIndex(UINT index)
{
    m_index = index;
}

void DOG::UICarousel::SendStrings(const std::vector<std::wstring>& filenames)
{
   m_labels = filenames;
}

DOG::UIIcon::UIIcon(DOG::gfx::D2DBackend_DX12& d2d, UINT id, std::vector<std::wstring> filePaths, float x, float y, float width, float height, float r, float g, float b, bool border): UIElement(id)
{
   ComPtr<IWICBitmapDecoder> m_decoder;
   ComPtr<IWICImagingFactory> m_imagingFactory;
   ComPtr<IWICBitmapFrameDecode> m_frame;
   ComPtr<IWICFormatConverter> m_converter;
   HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_IWICImagingFactory, (LPVOID*)m_imagingFactory.GetAddressOf());
   HR_VFY(hr);
   for (auto&& path : filePaths)
   {
      hr = m_imagingFactory->CreateDecoderFromFilename(
         path.c_str(),                            // Image to be decoded
         NULL,                            // Do not prefer a particular vendor
         GENERIC_READ,                    // Desired read access to the file
         WICDecodeMetadataCacheOnDemand,  // Cache metadata when needed
         m_decoder.GetAddressOf()                        // Pointer to the decoder
      );
      HR_VFY(hr);
      hr = m_decoder->GetFrame(0, m_frame.GetAddressOf());
      HR_VFY(hr);
      hr = m_imagingFactory->CreateFormatConverter(m_converter.GetAddressOf());
      HR_VFY(hr);
      hr = m_converter->Initialize(
         m_frame.Get(),                   // Input bitmap to convert
         GUID_WICPixelFormat32bppPBGRA,   // Destination pixel format
         WICBitmapDitherTypeNone,         // Specified dither pattern
         NULL,                            // Specify a particular palette
         0.f,                             // Alpha threshold
         WICBitmapPaletteTypeCustom       // Palette translation type
      );
      HR_VFY(hr);
      ComPtr<ID2D1Bitmap> bitmap;
      hr = d2d.Get2DDeviceContext()->CreateBitmapFromWicBitmap(m_converter.Get(), NULL, bitmap.GetAddressOf());
      HR_VFY(hr);
      m_bitmaps.push_back(bitmap);
   }
   HR_VFY(hr);
   hr = d2d.Get2DDeviceContext()->CreateSolidColorBrush(D2D1::ColorF(r, g, b, 1.0f), m_borderBrush.GetAddressOf());
   HR_VFY(hr);
   hr = d2d.Get2DDeviceContext()->CreateSolidColorBrush(D2D1::ColorF(0.f, 0.f, 0.f, 1.0f), m_backBrush.GetAddressOf());
   HR_VFY(hr);
   m_rect = D2D1::RectF(x, y, x + width, y + height);
   m_opacity = 1.0f;
   m_show = false;
   m_index = 0u;
   m_border = border;
}
DOG::UIIcon::~UIIcon()
{

}

void DOG::UIIcon::Draw(DOG::gfx::D2DBackend_DX12& d2d)
{
   if (m_border)
   {
      d2d.Get2DDeviceContext()->DrawRectangle(&m_rect, m_borderBrush.Get(), 4.f);
      d2d.Get2DDeviceContext()->FillRectangle(&m_rect, m_backBrush.Get());
   }
   if (m_show)
      d2d.Get2DDeviceContext()->DrawBitmap(m_bitmaps[m_index].Get(), &m_rect, m_opacity);
}
void DOG::UIIcon::Update(DOG::gfx::D2DBackend_DX12& d2d)
{
   UNREFERENCED_PARAMETER(d2d);
}

void DOG::UIIcon::Hide()
{
   m_show = false;
}

void DOG::UIIcon::DeactivateBorder()
{
   m_border = false;
}

void DOG::UIIcon::ActivateBorder()
{
   m_border = true;
}

void DOG::UIIcon::Show(UINT index)
{
   m_show = true;
   m_index = index;
}

DOG::UISlider::UISlider(DOG::gfx::D2DBackend_DX12& d2d, UINT id, float x, float y, float width, float height, std::function<void(float)> callback): UIElement(id)
{
   HRESULT hr = d2d.Get2DDeviceContext()->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White, 0.3f), &m_barBrush);
   HR_VFY(hr);
   hr = d2d.Get2DDeviceContext()->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White, 0.5f), &m_sliderBrush);
   HR_VFY(hr);
   m_slider = D2D1::RectF(x, y, x + 20.f, y + height);
   m_bar = D2D1::RectF(x + 10.f, y + height / 2.f - 1.f, x + width - 10.f, y + height / 2.f + 1.f);
   m_callback = callback;
   m_value = 0;
   m_width = width;
   m_normwidth = 1.f / width;
}
DOG::UISlider::~UISlider()
{

}
void DOG::UISlider::Draw(DOG::gfx::D2DBackend_DX12& d2d)
{
   d2d.Get2DDeviceContext()->FillRectangle(m_bar, m_barBrush.Get());
   d2d.Get2DDeviceContext()->FillRectangle(m_slider, m_sliderBrush.Get());
}
void DOG::UISlider::Update(DOG::gfx::D2DBackend_DX12& d2d)
{
   UNREFERENCED_PARAMETER(d2d);
}

void DOG::UISlider::OnEvent(IEvent& event)
{
   using namespace DOG;
   if (event.GetEventCategory() == EventCategory::MouseEventCategory)
   {
      auto mevent = EVENT(DOG::MouseMovedEvent);
      auto mpos = mevent.coordinates;
      bool prevIsSliding = m_isSliding;
      if (mpos.x >= m_bar.left && mpos.x <= m_bar.right && mpos.y >= m_slider.top && mpos.y <= m_slider.bottom && Mouse::IsButtonPressed(Button::Left))
      {
         m_slider.left = (float)mpos.x - 10.f;
         m_slider.right = m_slider.left + 20.f;
         float s = 0.5f * (m_slider.left + m_slider.right);
         m_value = Remap(m_bar.left, m_bar.right, 0, 1, s);
         m_isSliding = true;
      }
      else if(prevIsSliding)
      {
          m_isSliding = false;
          m_callback(m_value);
      }


      if (mpos.x >= m_slider.left && mpos.x <= m_slider.right && mpos.y >= m_slider.top && mpos.y <= m_slider.bottom)
         m_sliderBrush.Get()->SetOpacity(1.0f);
      else
      {
        m_sliderBrush.Get()->SetOpacity(0.5f);
      }
   }
}
float DOG::UISlider::GetValue()
{
   return m_value;
}

void DOG::UISlider::SetValue(float value)
{
    m_value = std::clamp(value, 0.0f, 1.0f);
    m_slider.left = std::lerp(m_bar.left, m_bar.right, m_value) - 10.0f;
    m_slider.right = m_slider.left + 20.f;
}

DOG::UIVertStatBar::UIVertStatBar(DOG::gfx::D2DBackend_DX12& d2d, UINT id, float x, float y, float width, float height, float fontSize, float r, float g, float b): UIElement(id)
{
   HRESULT hr = d2d.Get2DDeviceContext()->CreateSolidColorBrush(D2D1::ColorF(r, g, b, 0.3f), m_borderBrush.GetAddressOf());
   HR_VFY(hr);
   hr = d2d.Get2DDeviceContext()->CreateSolidColorBrush(D2D1::ColorF(r, g, b, 0.7f), m_barBrush.GetAddressOf());
   HR_VFY(hr);
   hr = d2d.Get2DDeviceContext()->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White, 0.7f), m_textBrush.GetAddressOf());
   HR_VFY(hr);
   hr = d2d.GetDWriteFactory()->CreateTextFormat(
      L"robotaur",
      NULL,
      DWRITE_FONT_WEIGHT_ULTRA_LIGHT,
      DWRITE_FONT_STYLE_NORMAL,
      DWRITE_FONT_STRETCH_NORMAL,
      fontSize,
      L"en-us",
      &m_textFormat
   );
   HR_VFY(hr);
   hr = m_textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
   HR_VFY(hr);
   hr = m_textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
   HR_VFY(hr);
   hr = m_textFormat->SetLineSpacing(DWRITE_LINE_SPACING_METHOD_UNIFORM, 28.f, 20.f);
   HR_VFY(hr);
   m_border = D2D1::RectF(x, y, x + width, y + height);
   m_bar = D2D1::RectF(x + 2.0f, y + 2.0f, x + width - 2.f, y + height - 2.f);
   m_barHeight = height;
   m_value = 0.0f;
   m_maxValue = 1.f;
   m_text = L"C\nh\na\nr\ng\ne";
   m_visible = true;

}
DOG::UIVertStatBar::~UIVertStatBar()
{

}
void DOG::UIVertStatBar::Draw(DOG::gfx::D2DBackend_DX12& d2d)
{
   if (m_visible)
   {
      if (m_value > 0.0f)
         d2d.Get2DDeviceContext()->FillRectangle(m_bar, m_barBrush.Get());
      d2d.Get2DDeviceContext()->DrawRectangle(m_border, m_barBrush.Get());
      d2d.Get2DDeviceContext()->DrawTextW(
         m_text.c_str(),
         (UINT32)m_text.length(),
         m_textFormat.Get(),
         &m_border,
         m_textBrush.Get());
   }
}
void DOG::UIVertStatBar::Update(DOG::gfx::D2DBackend_DX12& d2d)
{
   UNREFERENCED_PARAMETER(d2d);
   m_bar.top = m_bar.bottom - (m_value / m_maxValue) * m_barHeight + 4.f;
}

void DOG::UIVertStatBar::SetBarValue(float value, float maxValue)
{
   m_value = value;
   m_maxValue = maxValue;
}

DOG::UICheckBox::UICheckBox(DOG::gfx::D2DBackend_DX12& d2d, UINT id, float x, float y, float width, float height, float r, float g, float b, std::function<void(bool)> callback) : UIElement(id)
{
   HRESULT hr;
   hr = d2d.Get2DDeviceContext()->CreateSolidColorBrush(D2D1::ColorF(r, g, b, 0.7f), m_borderBrush.GetAddressOf());
   HR_VFY(hr);
   m_value = false;
   m_border = D2D1::RectF(x,y,x+width, y+height);
   m_callback = callback;
}
DOG::UICheckBox::~UICheckBox()
{

}
void DOG::UICheckBox::Draw(DOG::gfx::D2DBackend_DX12& d2d)
{
   if(m_value)
      d2d.Get2DDeviceContext()->FillRectangle(m_border, m_borderBrush.Get());
   else
      d2d.Get2DDeviceContext()->DrawRectangle(m_border, m_borderBrush.Get());

}
void DOG::UICheckBox::Update(DOG::gfx::D2DBackend_DX12& d2d)
{
   UNREFERENCED_PARAMETER(d2d);
}
bool DOG::UICheckBox::GetValue()
{
   return m_value;
}

void DOG::UICheckBox::SetValue(bool value)
{
    m_value = value;
}

void DOG::UICheckBox::OnEvent(IEvent& event)
{
   using namespace DOG;
   if (event.GetEventCategory() == EventCategory::MouseEventCategory)
   {
      if (event.GetEventType() == EventType::LeftMouseButtonPressedEvent)
      {
         auto mevent = EVENT(DOG::LeftMouseButtonPressedEvent);
         auto mpos = mevent.coordinates;
         if (mpos.x >= m_border.left && mpos.x <= m_border.right && mpos.y >= m_border.top && mpos.y <= m_border.bottom)
         {
            m_value = m_value ? false : true;
            m_callback(m_value);
         }
      }
      else if (event.GetEventType() == EventType::MouseMovedEvent)
      {
         auto mevent = EVENT(DOG::MouseMovedEvent);
         auto mpos = mevent.coordinates;
         if (mpos.x >= m_border.left && mpos.x <= m_border.right && mpos.y >= m_border.top && mpos.y <= m_border.bottom)
            m_borderBrush.Get()->SetOpacity(1.0f);
         else
            m_borderBrush.Get()->SetOpacity(0.7f);
      }
   }
}  

void DOG::UIVertStatBar::Hide(bool show)
{
   m_visible = show;
}

void UIRebuild(UINT clientHeight, UINT clientWidth)
{
   auto instance = DOG::UI::Get();

   //HealthBar
   float healthBarWidth = 300.f;
   float healthBarHeight = 50.f;
   auto h = instance->Create<DOG::UIHealthBar, float, float, float, float, float>(hID, (clientWidth - healthBarWidth) * 0.5f, clientHeight - 100.f, healthBarWidth, healthBarHeight, 20.f);
   instance->AddUIElementToScene(gameID, std::move(h));

   //Crosshair
   auto c = instance->Create<DOG::UICrosshair>(cID);
   instance->AddUIElementToScene(gameID, std::move(c));

   //Misc components
   std::vector<std::wstring> paths = { L"Assets/Sprites/FullAuto.bmp", L"Assets/Sprites/ChargeShot.bmp" };
   float xPos = 100.0f;
   float yPos = (FLOAT)clientHeight - 220.f;
   auto icon = instance->Create<DOG::UIIcon>(iconID, paths, xPos, yPos, 45.f, 45.f, 1.f, 1.f, 1.f, true);
   instance->AddUIElementToScene(gameID, std::move(icon));

   //Barrel components
   paths = { L"Assets/Sprites/Grenade.bmp", L"Assets/Sprites/Missile.bmp", L"Assets/Sprites/Laser.bmp" };
   xPos -= 75.0f;
   yPos += 75.0f;
   icon = instance->Create<DOG::UIIcon>(icon2ID, paths, xPos, yPos, 45.f, 45.f, 1.f, 1.f, 1.f, true);
   instance->AddUIElementToScene(gameID, std::move(icon));

   //Magazine components
   paths = { L"Assets/Sprites/Frost.bmp", L"Assets/Sprites/Fire.bmp" };
   xPos += 75.0f;
   yPos += 75.0f;
   icon = instance->Create<DOG::UIIcon>(icon3ID, paths, xPos, yPos, 45.f, 45.f, 1.f, 1.f, 1.f, true);
   instance->AddUIElementToScene(gameID, std::move(icon));

   //Active Item
   paths = { L"Assets/Sprites/TrampolineIcon.bmp", L"Assets/Sprites/TurretIcon.bmp", L"Assets/Sprites/ReviverIcon.bmp", L"Assets/Sprites/RadarIcon.bmp", L"Assets/Sprites/SyringeIcon.bmp" };
   icon = instance->Create<DOG::UIIcon>(iconActiveID, paths, 310.f, (FLOAT)clientHeight - 150.f, 75.f * 0.8f, 75.f * 0.8f, 1.f, 1.f, 1.f, true);
   instance->AddUIElementToScene(gameID, std::move(icon));

   //Weapon
   paths = { L"Assets/Sprites/WeaponSillhouette.bmp" };
   icon = instance->Create<DOG::UIIcon>(iconGun, paths, 90.f, (FLOAT)clientHeight - 145.f, 766.f * 0.2f, 373.f * 0.2f, 0.f, 0.f, 0.f, false);
   icon->Show(0u);
   instance->AddUIElementToScene(gameID, std::move(icon));

   //Flashlight & Glowstick icon
   paths = { L"Assets/Sprites/FlashlightIcon.bmp" };
   icon = instance->Create<DOG::UIIcon>(flashlightID, paths, 165.f, (FLOAT)clientHeight - 220.f, 81.f * 0.6f, 70.f * 0.6f, 0.f, 0.f, 0.f, false);
   icon->Show(0u);
   instance->AddUIElementToScene(gameID, std::move(icon));
   paths = { L"Assets/Sprites/Glowstick.bmp" };
   icon = instance->Create<DOG::UIIcon>(glowstickID, paths, 230.f, (FLOAT)clientHeight - 212.f, 78.f * 0.6f, 72.f * 0.6f, 0.f, 0.f, 0.f, false);
   icon->Show(0u);
   instance->AddUIElementToScene(gameID, std::move(icon));



   //Menu backgrounds
   auto menuBack = instance->Create<DOG::UIBackground, float, float, std::wstring>(menuBackID, (FLOAT)clientWidth, (FLOAT)clientHeight, std::wstring(L"Rogue Robots"));
   instance->AddUIElementToScene(menuID, std::move(menuBack));
   auto optionsBack = instance->Create<DOG::UIBackground, float, float, std::wstring>(optionsBackID, (FLOAT)clientWidth, (FLOAT)clientHeight, std::wstring(L"Options"));
   instance->AddUIElementToScene(optionsID, std::move(optionsBack));
   auto creditsBack = instance->Create<DOG::UIBackground, float, float, std::wstring>(creditsBackID, (FLOAT)clientWidth, (FLOAT)clientHeight, std::wstring(L""));
   instance->AddUIElementToScene(creditsID, std::move(creditsBack));
   auto multiBack = instance->Create<DOG::UIBackground, float, float, std::wstring>(multiBackID, (FLOAT)clientWidth, (FLOAT)clientHeight, std::wstring(L"Multiplayer"));
   instance->AddUIElementToScene(multiID, std::move(multiBack));
   auto levelSelectSoloBack = instance->Create<DOG::UIBackground, float, float, std::wstring>(levelSelectSoloBackID, (FLOAT)clientWidth, (FLOAT)clientHeight, std::wstring(L""));
   instance->AddUIElementToScene(levelSelectSoloID, std::move(levelSelectSoloBack));
   auto levelSelectMultBack = instance->Create<DOG::UIBackground, float, float, std::wstring>(levelSelectMultBackID, (FLOAT)clientWidth, (FLOAT)clientHeight, std::wstring(L""));
   instance->AddUIElementToScene(levelSelectMultID, std::move(levelSelectMultBack));
   auto lobbyBack = instance->Create<DOG::UIBackground, float, float, std::wstring>(multiBackID, (FLOAT)clientWidth, (FLOAT)clientHeight, std::wstring(L"Lobby"));
   instance->AddUIElementToScene(lobbyID, std::move(lobbyBack));

   auto bplaylobby = instance->Create<DOG::UIButton, float, float, float, float, float, float, float, float, std::wstring>(bpLobbyID, (FLOAT)clientWidth / 2.f - 150.f / 2, (FLOAT)clientHeight / 2.f + 250.f, 150.f, 60.f, 20.f, 1.0f, 1.0f, 1.0f, std::wstring(L"Play"), std::function<void()>(HostLaunch));
   instance->AddUIElementToScene(lobbyID, std::move(bplaylobby));

   auto joinback = instance->Create<DOG::UIBackground, float, float, std::wstring>(menuBackID, (FLOAT)clientWidth, (FLOAT)clientHeight, std::wstring(L"Join Room"));
   instance->AddUIElementToScene(joinID, std::move(joinback));

   auto joining = instance->Create<DOG::UIBackground, float, float, std::wstring>(menuBackID, (FLOAT)clientWidth, (FLOAT)clientHeight, std::wstring(L"Joined Room"));
   instance->AddUIElementToScene(WaitingForHostID, std::move(joining));

   auto gameOver = instance->Create<DOG::UIBackground, float, float, std::wstring>(menuBackID, (FLOAT)clientWidth, (FLOAT)clientHeight, std::wstring(L"Game Over"));
   instance->AddUIElementToScene(GameOverID, std::move(gameOver));

   auto winText = instance->Create<DOG::UIBackground, float, float, std::wstring>(menuBackID, (FLOAT)clientWidth + 80.0f, (FLOAT)clientHeight, std::wstring(L"You won!"));
   instance->AddUIElementToScene(WinScreenID, std::move(winText));

   auto loading = instance->Create<DOG::UIBackground, float, float, std::wstring>(menuBackID, (FLOAT)clientWidth, (FLOAT)clientHeight, std::wstring(L"\n\nTrying to connect...\n\n\n\n\n\n\n\n\nPro Tip: Shooting teammates is fun "));
   instance->AddUIElementToScene(LoadingID, std::move(loading));

   //Credit text
   //Music
   auto lMusic = instance->Create<DOG::UILabel>(
      lMusicID,
      std::wstring(L"Music"),
      (FLOAT)clientWidth / 2.0f - 850.0f,
      150.0f,
      500.f,
      200.f,
      45.0f
      );

   auto lMusicText = instance->Create<DOG::UILabel>(
      lMusicTextID,
      std::wstring(L"Douglas Runebj") + wchar_t(214) + L"rk",
      (FLOAT)clientWidth / 2.0f - 850.0f,
      210.0f,
      500.f,
      200.f,
      25.0f
      );

   //Fiverr artists
   auto lFiverrArtists = instance->Create<DOG::UILabel>(
      lFiverrArtistsID,
      std::wstring(L"Fiverr Artists"),
      (FLOAT)clientWidth / 2.0f - 850.0f,
      350.0f,
      500.f,
      200.f,
      45.0f
      );

   auto lFiverrArtistsText = instance->Create<DOG::UILabel>(
      lFiverrArtistsTextID,
      std::wstring(L"The Weapon & Component Models - francisle997\nThe Player Models - frein4"),
      (FLOAT)clientWidth / 2.0f - 850.0f,
      440.0f,
      500.f,
      200.f,
      25.0f
      );

   //The team
   auto lTheTeam = instance->Create<DOG::UILabel>(
      lTheTeamID,
      std::wstring(L"The Team"),
      (FLOAT)clientWidth / 2.0f - 250.0f,
      20.0f,
      500.f,
      100.f,
      60.0f
      );
   auto lNamesCredits = instance->Create<DOG::UILabel>(
      lNamesCreditsID,
      std::wstring(
         L"Sam Axelsson\nGunnar Cerne\nFilip Eriksson\nEmil Fransson\nNadhif Ginola\nJonatan Hermansson\nEmil H") + wchar_t(214) + L"gstedt\nAxel Lundberg\nOscar Milstein\nOve " + wchar_t(216) + L"deg" + wchar_t(229) + L"rd",
      (FLOAT)clientWidth / 2.0f - 350.0f,
      (FLOAT)clientHeight / 2.f - 270.0f,
      700.0f,
      200.0f,
      40.f
      );

   //Fiverr artists
   auto lIconsCredits = instance->Create<DOG::UILabel>(
      lIconsCreditsID,
      std::wstring(L"Icons"),
      (FLOAT)clientWidth / 2.0f + 350.0f,
      150.0f,
      500.f,
      200.f,
      45.0f
      );

   auto lIconsCreditsText = instance->Create<DOG::UILabel>(
      lIconsCreditsTextID,
      std::wstring(L"All icons got from 'game-icons.net'\nJump across icon - Delapouite\nCrosshair icon - Delapouite\nFlamer icon - sbed\nHealth increase icon - sbed\nSprint icon - Lorc\nSnowflake 2 icon - Lorc\nRay gun icon - Lorc\nGrenade icon - Lorc\nBullets icon - Lorc\nRocket icon - Lorc"),
      (FLOAT)clientWidth / 2.0f + 250.0f,
      350.0f,
      700.f,
      200.f,
      25.0f
      );

   auto lAcknowledgements = instance->Create<DOG::UILabel>(
      lAcknowledgementsID,
      std::wstring(L"Acknowledgements"),
      (FLOAT)clientWidth / 2.0f - 350.0f,
      (FLOAT)clientHeight - 400.f,
      700.f,
      100.f,
      45.0f
      );

   auto lAcknowledgementsText = instance->Create<DOG::UILabel>(
      lAcknowledgementsTextID,
      std::wstring(L"We want to thank all of our teachers for mentoring us through this project.\nA special thanks to Blackdrop Interactive for sponsoring us with money to be able to commission assets.\nA very special thanks to Pascal Deraed for teaching us so much about Blender and spending his free time to help us."),
      (FLOAT)clientWidth / 2.0f - 400.0f,
      (FLOAT)clientHeight - 350.f,
      800.f,
      300.f,
      25.0f
      );

   //Menu buttons
   auto bp = instance->Create<DOG::UIButton, float, float, float, float, float, float, float, float, std::wstring>(bpID, (FLOAT)clientWidth / 2.f - 200.f / 2, (FLOAT)clientHeight / 2.f, 200.f, 60.f, 20.f, 0.0f, 1.0f, 0.0f, std::wstring(L"Play"), std::function<void()>(LevelSelectSoloButtonFunc));
   auto bm = instance->Create<DOG::UIButton, float, float, float, float, float, float, float, float, std::wstring>(bmID, (FLOAT)clientWidth / 2.f - 200.f / 2, (FLOAT)clientHeight / 2.f + 70.f, 200.f, 60.f, 20.f, 1.0f, 1.0f, 1.0f, std::wstring(L"Multiplayer"), std::function<void()>(MultiplayerButtonFunc));
   //Options menu is not used atm.
   auto bo = instance->Create<DOG::UIButton, float, float, float, float, float, float, float, float, std::wstring>(boID, (FLOAT)clientWidth / 2.f - 200.f / 2, (FLOAT)clientHeight / 2.f + 140.f, 200.f, 60.f, 20.f, 1.0f, 1.0f, 1.0f, std::wstring(L"Options"), std::function<void()>(OptionsButtonFunc));
   auto bc = instance->Create<DOG::UIButton, float, float, float, float, float, float, float, float, std::wstring>(bcID, (FLOAT)clientWidth / 2.f - 200.f / 2, (FLOAT)clientHeight / 2.f + 210.f, 200.f, 60.f, 20.f, 1.0f, 1.0f, 1.0f, std::wstring(L"Credits"), std::function<void()>(CreditsButtonFunc));
   auto be = instance->Create<DOG::UIButton, float, float, float, float, float, float, float, float, std::wstring>(beID, (FLOAT)clientWidth / 2.f - 200.f / 2, (FLOAT)clientHeight / 2.f + 280.f, 200.f, 60.f, 20.f, 1.0f, 1.0f, 1.0f, std::wstring(L"Exit"), std::function<void()>(ExitButtonFunc));
   auto credback = instance->Create<DOG::UIButton, float, float, float, float, float, float, float, float, std::wstring>(credbackID, (FLOAT)clientWidth / 2.f - 150.f / 2, (FLOAT)clientHeight - 80.f, 150.f, 60.f, 20.f, 1.0f, 1.0f, 1.0f, std::wstring(L"Back"), std::function<void()>(ToMenuButtonFunc));
   auto mulback = instance->Create<DOG::UIButton, float, float, float, float, float, float, float, float, std::wstring>(mulbackID, (FLOAT)clientWidth / 2.f - 150.f / 2, (FLOAT)clientHeight / 2.f + 200.f, 150.f, 60.f, 20.f, 1.0f, 1.0f, 1.0f, std::wstring(L"Back"), std::function<void()>(ToMenuButtonFunc));
   auto hostBack = instance->Create<DOG::UIButton, float, float, float, float, float, float, float, float, std::wstring>(mulbackID, (FLOAT)clientWidth / 2.f - 200.f / 2, (FLOAT)clientHeight / 2.f + 350.f, 200.f, 60.f, 20.f, 1.0f, 1.0f, 1.0f, std::wstring(L"Disconnect"), std::function<void()>(BackFromHost));
   auto bh = instance->Create<DOG::UIButton, float, float, float, float, float, float, float, float, std::wstring>(bhID, (FLOAT)clientWidth / 2.f - 75.f - 100.f, (FLOAT)clientHeight / 2.f + 120.f, 150.f, 60.f, 20.f, 1.0f, 1.0f, 1.0f, std::wstring(L"Host"), std::function<void()>(LevelSelectMultButtonFunc));
   auto bj = instance->Create<DOG::UIButton, float, float, float, float, float, float, float, float, std::wstring>(bjID, (FLOAT)clientWidth / 2.f - 75.f + 100.f, (FLOAT)clientHeight / 2.f + 120.f, 150.f, 60.f, 20.f, 1.0f, 1.0f, 1.0f, std::wstring(L"Join"), std::function<void()>(JoinButton));
   auto clientBack = instance->Create<DOG::UIButton, float, float, float, float, float, float, float, float, std::wstring>(mulbackID, (FLOAT)clientWidth / 2.f - 200.f / 2, (FLOAT)clientHeight / 2.f + 250.f, 200.f, 60.f, 20.f, 1.0f, 1.0f, 1.0f, std::wstring(L"Disconnect"), std::function<void()>(BackFromHost));
   auto bjj = instance->Create<DOG::UIButton, float, float, float, float, float, float, float, float, std::wstring>(bjjID, (FLOAT)clientWidth / 2.f - 75.f + 100.f, (FLOAT)clientHeight / 2.f + 140.f, 150.f, 60.f, 20.f, 1.0f, 1.0f, 1.0f, std::wstring(L"Back"), std::function<void()>(MultiplayerButtonFunc));

   //Level selector.
   auto bStartLevelSelectorSolo = instance->Create<DOG::UIButton, float, float, float, float, float, float, float, float, std::wstring>(bStartLevelSelectorSoloID, (FLOAT)clientWidth / 2.f - 150.f / 2, (FLOAT)clientHeight / 2.f + 200.f, 150.f, 60.f, 20.f, 0.0f, 1.0f, 0.0f, std::wstring(L"Start"), std::function<void()>(PlayButtonFunc));
   auto bGoBackLevelSelectorSolo = instance->Create<DOG::UIButton, float, float, float, float, float, float, float, float, std::wstring>(bGoBackLevelSelectorSoloID, (FLOAT)clientWidth / 2.f - 150.f / 2, (FLOAT)clientHeight / 2.f + 300.f, 150.f, 60.f, 20.f, 0.0f, 1.0f, 0.0f, std::wstring(L"Back"), std::function<void()>(ToMenuButtonFunc));
   auto bStartLevelSelectorMult = instance->Create<DOG::UIButton, float, float, float, float, float, float, float, float, std::wstring>(bStartLevelSelectorMultID, (FLOAT)clientWidth / 2.f - 150.f / 2, (FLOAT)clientHeight / 2.f + 200.f, 150.f, 60.f, 20.f, 0.0f, 1.0f, 0.0f, std::wstring(L"Start Hosting"), std::function<void()>(HostButtonFunc));
   auto bGoBackLevelSelectorMult = instance->Create<DOG::UIButton, float, float, float, float, float, float, float, float, std::wstring>(bGoBackLevelSelectorMultID, (FLOAT)clientWidth / 2.f - 150.f / 2, (FLOAT)clientHeight / 2.f + 300.f, 150.f, 60.f, 20.f, 0.0f, 1.0f, 0.0f, std::wstring(L"Back"), std::function<void()>(ToMenuButtonFunc));

   //Room Join buttons
   auto ipBar = instance->Create<DOG::UITextField>(ipBarID, (FLOAT)clientWidth / 2.f - 125.f, (FLOAT)clientHeight / 2.f, 200.0f, 50.0f);
   auto r1 = instance->Create<DOG::UIButton>(r1ID, (FLOAT)clientWidth / 2.f - 500.f / 2, (FLOAT)clientHeight / 2.f + 140.f, 150.f, 60.f, 20.f, 0.0f, 1.0f, 0.0f, std::wstring(L"Join"), std::function<void()>(Room1Button));
   
   //Labels
   auto l1 = instance->Create<DOG::UILabel>(l1ID, std::wstring(L""), (FLOAT)clientWidth / 2.f - 250.0f, (FLOAT)clientHeight / 2.f - 450.f, 500.f, 60.f, 40.f);
   auto l2 = instance->Create<DOG::UILabel>(l2ID, std::wstring(L""), (FLOAT)clientWidth / 2.f, (FLOAT)clientHeight / 2.f - 350.f, 500.f, 60.f, 40.f);
   auto l3 = instance->Create<DOG::UILabel>(l3ID, std::wstring(L""), (FLOAT)clientWidth / 2.f, (FLOAT)clientHeight / 2.f - 250.f, 500.f, 60.f, 40.f);

   //Client
   auto l4 = instance->Create<DOG::UILabel>(l4ID, std::wstring(L""), (FLOAT)clientWidth / 2.f - 250.0f, (FLOAT)clientHeight / 2.f - 450.f, 500.f, 60.f, 40.f);
   auto l5 = instance->Create<DOG::UILabel>(l5ID, std::wstring(L""), (FLOAT)clientWidth / 2.f - 250.0f, (FLOAT)clientHeight / 2.f - 350.f, 500.f, 60.f, 40.f);
   auto l6 = instance->Create<DOG::UILabel>(l6ID, std::wstring(L""), (FLOAT)clientWidth / 2.f, (FLOAT)clientHeight / 2.f - 250.f, 500.f, 60.f, 40.f);

   auto lWinText = instance->Create<DOG::UILabel>(lWinTextID, std::wstring(L" "), (FLOAT)clientWidth / 2.f - 350.0f, (FLOAT)clientHeight / 2.f - 150.f, 700.f, 60.f, 40.f);
   auto lredScore = instance->Create<DOG::UILabel>(lredScoreID, std::wstring(L" "), 50.f, (FLOAT)clientHeight / 2.f - 250.f, 800.f, 160.f, 40.f);
   auto lblueScore = instance->Create<DOG::UILabel>(lblueScoreID, std::wstring(L" "), (FLOAT)clientWidth / 2.f + 50.f, (FLOAT)clientHeight / 2.f - 250.f, 800.f, 160.f, 40.f);
   auto lgreenScore = instance->Create<DOG::UILabel>(lgreenScoreID, std::wstring(L" "), 50.f, (FLOAT)clientHeight / 2.f + 50.f, 800.f, 160.f, 40.f);
   auto lyellowScore = instance->Create<DOG::UILabel>(lyellowScoreID, std::wstring(L" "), (FLOAT)clientWidth / 2.f + 50.f, (FLOAT)clientHeight / 2.f + 50.f, 800.f, 160.f, 40.f);

   auto lredScoreWin = instance->Create<DOG::UILabel>(lredScoreWinID, std::wstring(L" "), 50.f, (FLOAT)clientHeight / 2.f - 250.f, 800.f, 160.f, 40.f);
   auto lblueScoreWin = instance->Create<DOG::UILabel>(lblueScoreWinID, std::wstring(L" "), (FLOAT)clientWidth / 2.f + 50.f, (FLOAT)clientHeight / 2.f - 250.f, 800.f, 160.f, 40.f);
   auto lgreenScoreWin = instance->Create<DOG::UILabel>(lgreenScoreWinID, std::wstring(L" "), 50.f, (FLOAT)clientHeight / 2.f + 50.f, 800.f, 160.f, 40.f);
   auto lyellowScoreWin = instance->Create<DOG::UILabel>(lyellowScoreWinID, std::wstring(L" "), (FLOAT)clientWidth / 2.f + 50.f, (FLOAT)clientHeight / 2.f + 50.f, 800.f, 160.f, 40.f);

  /* UINT sliderID;
   auto slider = instance->Create<DOG::UISlider, float, float, float, float>(sliderID, 100.f, 100.f, 250.f, 30.f, std::function<void(float)>(SliderFunc));
   instance->AddUIElementToScene(optionsID, std::move(slider));*/

   auto labelButtonTextActiveItem = instance->Create<DOG::UILabel>(lActiveItemTextID, std::wstring(L"G"), 315.0f, (FLOAT)clientHeight - 90.0f, 50.f, 50.f, 40.f);

   auto lStartText = instance->Create<DOG::UILabel>(lStartTextID, std::wstring(L""), (FLOAT)clientWidth / 2.f - 350.f, (FLOAT)clientHeight / 2.f - 400.f, 700.f, 300.f, 60.f);

   //player list
   auto playerList = instance->Create<DOG::UIPlayerList>(playerListID);
   auto playerListJoin = instance->Create<DOG::UIPlayerList>(playerListJoinID);

   //Singleplayer UI carousel
   auto carouselSolo = instance->Create<DOG::UICarousel, std::vector<std::wstring>, float, float, float, float, float>(carouselSoloID, { L"" }, (FLOAT)clientWidth / 2.f - 150.f, (FLOAT)clientHeight / 2.f + 100.f, 300.f, 75.f, 25.f);
   instance->AddUIElementToScene(levelSelectSoloID, std::move(carouselSolo));

   //Multiplayer UI carousel
   auto carouselMult = instance->Create<DOG::UICarousel, std::vector<std::wstring>, float, float, float, float, float>(carouselMultID, { L"" }, (FLOAT)clientWidth / 2.f - 150.f, (FLOAT)clientHeight / 2.f + 100.f, 300.f, 75.f, 25.f);
   instance->AddUIElementToScene(levelSelectMultID, std::move(carouselMult));

   std::vector<std::wstring> vec = { L"Assets/Sprites/MaxHP.bmp" , L"Assets/Sprites/MoveSpeed.bmp" , L"Assets/Sprites/JumpBoost.bmp" };
   auto pic = instance->Create<DOG::UIBuffTracker, std::vector<std::wstring>>(buffID, vec);
   instance->AddUIElementToScene(gameID, std::move(pic));

   auto pbar = instance->Create<DOG::UIVertStatBar, float, float, float, float, float, float, float, float>(pbarID, 27.f, (FLOAT)clientHeight - 425.f, 40.f, 250.f, 25.f, 0.34f, 0.69f, 0.99f);
   instance->AddUIElementToScene(gameID, std::move(pbar));
   DOG::UI::Get()->GetUI<DOG::UIVertStatBar>(pbarID)->Hide(false);

   /*UINT cboxID;
   auto checkbox = instance->Create<DOG::UICheckBox, float, float, float, float, float, float, float>(cboxID, 300.f, (FLOAT)clientHeight - 425.f, 25.f, 25.f, 1.0f, 1.0f, 1.0f, std::function<void(bool)>(CheckBoxFunc));
   instance->AddUIElementToScene(optionsID, std::move(checkbox));*/

   instance->AddUIElementToScene(menuID, std::move(bp));
   instance->AddUIElementToScene(menuID, std::move(bm));
   //Options menu is not used atm.
   instance->AddUIElementToScene(menuID, std::move(bo));
   instance->AddUIElementToScene(menuID, std::move(bc));
   instance->AddUIElementToScene(menuID, std::move(be));
   instance->AddUIElementToScene(creditsID, std::move(credback));
   instance->AddUIElementToScene(creditsID, std::move(lFiverrArtists));
   instance->AddUIElementToScene(creditsID, std::move(lFiverrArtistsText));
   instance->AddUIElementToScene(creditsID, std::move(lNamesCredits));
   instance->AddUIElementToScene(creditsID, std::move(lTheTeam));
   instance->AddUIElementToScene(creditsID, std::move(lIconsCredits));
   instance->AddUIElementToScene(creditsID, std::move(lIconsCreditsText));
   instance->AddUIElementToScene(creditsID, std::move(lMusic));
   instance->AddUIElementToScene(creditsID, std::move(lMusicText));
   instance->AddUIElementToScene(creditsID, std::move(lAcknowledgements));
   instance->AddUIElementToScene(creditsID, std::move(lAcknowledgementsText));
   instance->AddUIElementToScene(multiID, std::move(mulback));
   instance->AddUIElementToScene(multiID, std::move(bh));
   instance->AddUIElementToScene(multiID, std::move(bj));
   instance->AddUIElementToScene(joinID, std::move(r1));
   instance->AddUIElementToScene(joinID, std::move(ipBar));
   instance->AddUIElementToScene(joinID, std::move(bjj));
   instance->AddUIElementToScene(lobbyID, std::move(l1));
   instance->AddUIElementToScene(lobbyID, std::move(l2));
   instance->AddUIElementToScene(lobbyID, std::move(l3));
   instance->AddUIElementToScene(lobbyID, std::move(hostBack));
   instance->AddUIElementToScene(WaitingForHostID, std::move(clientBack));
   instance->AddUIElementToScene(WaitingForHostID, std::move(l4));
   instance->AddUIElementToScene(gameID, std::move(l5));
   instance->AddUIElementToScene(WaitingForHostID, std::move(l6));
   instance->AddUIElementToScene(gameID, std::move(lWinText));
   instance->AddUIElementToScene(gameID, std::move(lStartText));
   instance->AddUIElementToScene(GameOverID, std::move(lredScore));
   instance->AddUIElementToScene(GameOverID, std::move(lblueScore));
   instance->AddUIElementToScene(GameOverID, std::move(lgreenScore));
   instance->AddUIElementToScene(GameOverID, std::move(lyellowScore));
   instance->AddUIElementToScene(WinScreenID, std::move(lredScoreWin));
   instance->AddUIElementToScene(WinScreenID, std::move(lblueScoreWin));
   instance->AddUIElementToScene(WinScreenID, std::move(lgreenScoreWin));
   instance->AddUIElementToScene(WinScreenID, std::move(lyellowScoreWin));
   instance->AddUIElementToScene(lobbyID, std::move(playerList));
   instance->AddUIElementToScene(WaitingForHostID, std::move(playerListJoin));
   instance->AddUIElementToScene(gameID, std::move(labelButtonTextActiveItem));
   instance->AddUIElementToScene(levelSelectSoloID, std::move(bStartLevelSelectorSolo));
   instance->AddUIElementToScene(levelSelectSoloID, std::move(bGoBackLevelSelectorSolo));
   instance->AddUIElementToScene(levelSelectMultID, std::move(bStartLevelSelectorMult));
   instance->AddUIElementToScene(levelSelectMultID, std::move(bGoBackLevelSelectorMult));

   //Splash screen
   // UINT sID;
   // auto s = DOG::UI::Get().Create<DOG::UISplashScreen, float, float>(sID, (float)clientWidth, (float)clientHeight);
   // DOG::UI::Get().AddUIElementToScene(menuID, std::move(s));

   auto& externalUI = instance->GetExternalUI();
   for (auto& e : externalUI)
   {
      e(clientWidth, clientHeight);
   }
}

void AddScenes()
{
   auto instance = DOG::UI::Get();
   menuID = instance->AddScene();
   gameID = instance->AddScene();
   multiID = instance->AddScene();
   optionsID = instance->AddScene();
   lobbyID = instance->AddScene();
   instance->ChangeUIscene(menuID);
   joinID = instance->AddScene();
   WaitingForHostID = instance->AddScene();
   GameOverID = instance->AddScene();
   WinScreenID = instance->AddScene();
   LoadingID = instance->AddScene();
   creditsID = instance->AddScene();
   levelSelectSoloID = instance->AddScene();
   levelSelectMultID = instance->AddScene();
}

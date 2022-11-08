#pragma once
#include <time.h>
#include <functional>
#include "../RHI/DX12/D2DBackend_DX12.h"
#include "../RHI/RenderDevice.h"
#include "../../EventSystem/IEvent.h"
#include "../../EventSystem/Layer.h"
#include "../../EventSystem/EventPublisher.h"


void UIRebuild(UINT clientHeight, UINT clientWidth);
void AddScenes();

extern UINT menuID, gameID, optionsID, multiID;
extern UINT menuBackID, optionsBackID, multiBackID;
extern UINT bpID, bmID, boID, beID, optbackID, mulbackID, bhID, bjID;
extern UINT cID, tID, hID;

namespace DOG
{
   class UIElement;
   class RenderDevice;
   class Swapchain;
   class UIScene;

   class UIElement
   {
      public:
         UIElement(UINT id);
         virtual ~UIElement();
         virtual void Draw(DOG::gfx::D2DBackend_DX12& d2d) = 0;
         virtual void Update(DOG::gfx::D2DBackend_DX12& d2d);
         UINT GetID();
         virtual void OnEvent(IEvent& event);
      private:
         UINT m_ID;
   };



   /** @brief This is a singleton. To use this interface call the Get() function to obtain the global instance. 
    * To create UI elements call create<UIelement>() and pass the UIelement derived class as a template parameter.
    * To create a scene and switch bewtween scenes use AddScene() and ChangeUIscene() accordingly. 
    * To register a UIelement to a scene call AddUIElementToScene().
    * To add new elements create a class, inherit the UIElement class and implement its pure virtual funtions.
   **/
   class UI : public Layer
   {
      public:
         UI(DOG::gfx::RenderDevice* rd, DOG::gfx::Swapchain* sc, UINT numBuffers, UINT clientWidth, UINT clientHeight);
         ~UI();
         void DrawUI();
         void ChangeUIscene(UINT sceneID);
         UINT AddUIElementToScene(UINT sceneID, std::unique_ptr<UIElement> element);
         UINT RemoveUIElement(UINT elementID);
         UINT AddScene();
         void RemoveScene(UINT sceneID);
         UIScene* GetScene(UINT sceneID);
         void Resize(UINT clientWidth, UINT clientHeight);
         void FreeResize();
         static void Initialize(DOG::gfx::RenderDevice* rd, DOG::gfx::Swapchain* sc, UINT numBuffers, UINT clientWidth, UINT clientHeight);
         static UI* Get();
         static void Destroy();
         void OnEvent(IEvent& event) override final;
         DOG::gfx::D2DBackend_DX12* GetBackend();


         template<typename T, typename... Params>
         std::unique_ptr<T> Create(UINT& uidOut, Params... args)
         {
            uidOut = GenerateUID();
            return std::make_unique<T>(*m_d2d.get(), uidOut, std::forward<Params>(args)...);
         }

         template<typename T>
         T* GetUI(UINT& elementID)
         {
            for (auto&& s : m_scenes)
            {
               auto res = std::find_if(s->GetScene().begin(), s->GetScene().end(), [&](std::unique_ptr<UIElement> const& e) { return e->GetID() == elementID; });
               if (res == s->GetScene().end())
                  continue;
               else
                  return static_cast<T*>((*res).get());
            }
            return nullptr;
         }

         UI(UI& other) = delete;
         void operator=(const UI&) = delete;

      private:
         static UI* s_instance;
         std::vector<std::unique_ptr<UIScene>> m_scenes;
         std::unique_ptr<DOG::gfx::D2DBackend_DX12> m_d2d; //The thing that renders everything
         UINT m_width, m_height;
         UINT m_menuID, m_gameID;
         UINT m_currsceneID, m_currsceneIndex;
         bool m_visible;
         UINT QueryScene(UINT sceneID);
         UINT GenerateUID();
         void BuildMenuUI();
         void BuildGameUI();
         std::vector<UINT> m_generatedIDs;
   };

   class UIScene 
   {
      public:
      UIScene(UINT id);
      ~UIScene() = default;
      UINT GetID();
      
      std::vector<std::unique_ptr<UIElement>>& GetScene();
      private:
      std::vector<std::unique_ptr<UIElement>> m_scene;
      UINT m_ID;

   };

   //UIScene ärver från Layuer
   //UIscene tar emot events med onEvent
   //UI scene sckickar eventet till alla ui elements som har en gemensam funktion som tar emot events.
   

   class UIButton : public UIElement
   {
      public:
         bool pressed;
         UIButton(DOG::gfx::D2DBackend_DX12& d2d, UINT id, float x, float y, float width, float height, float fontSize, const std::wstring& text, std::function<void(void)> callback);
         void Draw(DOG::gfx::D2DBackend_DX12& d2d) override final;
         //void Update(DOG::gfx::D2DBackend_DX12& d2d) override final;
         void OnEvent(IEvent& event) override final;
         ~UIButton();
      private:
         D2D_POINT_2F m_pos;
         D2D1_RECT_F m_textRect;
         D2D_VECTOR_2F m_size;
         std::wstring m_text;
         std::function<void(void)> m_callback;
         ComPtr<IDWriteTextFormat> m_format;
         ComPtr<ID2D1SolidColorBrush> m_brush;
   };

   

   class UISplashScreen : public UIElement
   {
      public:
         UISplashScreen(DOG::gfx::D2DBackend_DX12& d2d, UINT id, float width, float height);
         void Draw(DOG::gfx::D2DBackend_DX12& d2d) override final;
         void Update(DOG::gfx::D2DBackend_DX12& d2d) override final;
         ~UISplashScreen();
      private:
         D2D1_RECT_F m_background;
         clock_t m_timer;
         ComPtr<ID2D1SolidColorBrush> m_splashBrush, m_textBrush;
         ComPtr<IDWriteTextFormat> m_format;
         std::wstring m_text;
         float m_textOp, m_backOp;
   };

   class UIHealthBar : public UIElement
   {
      public:
         UIHealthBar(DOG::gfx::D2DBackend_DX12& d2d, UINT id, float x, float y, float width, float height);
         ~UIHealthBar();

         void Draw(DOG::gfx::D2DBackend_DX12& d2d) override final;
         void Update(DOG::gfx::D2DBackend_DX12& d2d) override final;

         void SetBarValue(float value);

      private:
         D2D1_RECT_F m_border, m_bar;
         std::wstring m_text;
         ComPtr<ID2D1SolidColorBrush> m_barBrush, m_borderBrush;
         ComPtr<IDWriteTextFormat> m_textFormat;
         float m_value, m_barWidth, m_test;
   };

   class UIBackground : public UIElement
   {
      public:
         UIBackground(DOG::gfx::D2DBackend_DX12& d2d, UINT id, float width, float heigt, const std::wstring& title);
         ~UIBackground();
         void Draw(DOG::gfx::D2DBackend_DX12& d2d) override final;
         void Update(DOG::gfx::D2DBackend_DX12& d2d) override final;
      private:
         D2D1_RECT_F m_background, m_textRect;
         std::wstring m_title;
         ComPtr<IDWriteTextFormat> m_textFormat;
         ComPtr<ID2D1SolidColorBrush> m_textBrush, m_backBrush;
   };

   class UICrosshair : public UIElement
   {
      public:
         UICrosshair(DOG::gfx::D2DBackend_DX12& d2d, UINT id);
         ~UICrosshair();
         void Draw(DOG::gfx::D2DBackend_DX12& d2d) override final;
      private:
         ComPtr<IDWriteTextFormat> m_textFormat;
         ComPtr<ID2D1SolidColorBrush> m_brush;
         D2D_RECT_F m_screenSize;

   };

   class UITextField : public UIElement
   {
      public:
         UITextField(DOG::gfx::D2DBackend_DX12& d2d, UINT id, float x, float y, float width, float height);
         ~UITextField();
         void Draw(DOG::gfx::D2DBackend_DX12& d2d) override final;
         //void Update(DOG::gfx::D2DBackend_DX12& d2d) override final;
         void OnEvent(IEvent& event) override final;
         std::wstring GetText();
      private:
         bool m_active;
         D2D1_RECT_F m_border, m_background;
         ComPtr<IDWriteTextFormat> m_textFormat;
         ComPtr<ID2D1SolidColorBrush> m_backBrush, m_borderBrush, m_textBrush;
         std::wstring m_text;
         std::wstring m_displayText;
   };

}
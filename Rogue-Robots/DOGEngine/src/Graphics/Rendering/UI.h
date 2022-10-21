#pragma once
#include <time.h>
#include <functional>
#include "../RHI/DX12/D2DBackend_DX12.h"
#include "../RHI/RenderDevice.h"

namespace DOG
{
   class UIElement;
   class RenderDevice;
   class Swapchain;
   class UIScene;

   class UI
   {
      public:
      UI(DOG::gfx::RenderDevice* rd, DOG::gfx::Swapchain* sc, u_int numBuffers, UINT clientWidth, UINT clientHeight);
      ~UI();
      void DrawUI();
      void ChangeUIscene(UINT sceneID);
      UINT AddUIElementToScene(UINT sceneID, std::unique_ptr<UIElement> element);
      UINT GenerateUID();
      UINT AddScene();
      void RemoveScene(UINT sceneID);
      void Resize(UINT clientWidth, UINT clientHeight);
      void FreeResize();
      DOG::gfx::D2DBackend_DX12* GetBackend();
      template<typename T, typename... Params>
      std::unique_ptr<T> Create(UINT& uidOut, Params... args)
      {
         uidOut = GenerateUID();
         return std::make_unique<T>(*m_d2d.get(), uidOut, std::forward<Params>(args)...);
      }

      private:
      std::vector<std::unique_ptr<UIScene>> m_scenes;
      std::unique_ptr<DOG::gfx::D2DBackend_DX12> m_d2d; //The thing that renders everything
      UINT m_width, m_height;
      UINT m_menuID, m_gameID;
      UINT QueryScene(UINT sceneID);
      UINT m_currsceneID, m_currsceneIndex;
      bool m_visible;
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

   class UIElement
   {
      public:
      UIElement(UINT id);
      virtual ~UIElement();
      virtual void Draw(DOG::gfx::D2DBackend_DX12& d2d) = 0;
      virtual void Update(DOG::gfx::D2DBackend_DX12& d2d);
      UINT GetID();
      private:
      UINT m_ID;
   };

   class UIButton : public UIElement
   {
      public:
      bool pressed;
      UIButton(DOG::gfx::D2DBackend_DX12& d2d, UINT id, float x, float y, float width, float height, float fontSize, std::wstring text, std::function<void(void)> callback);
      void Draw(DOG::gfx::D2DBackend_DX12& d2d) override;
      void Update(DOG::gfx::D2DBackend_DX12& d2d) override;
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
      void Draw(DOG::gfx::D2DBackend_DX12& d2d) override;
      void Update(DOG::gfx::D2DBackend_DX12& d2d) override;
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

      void Draw(DOG::gfx::D2DBackend_DX12& d2d) override;
      void Update(DOG::gfx::D2DBackend_DX12& d2d) override;

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
      UIBackground(DOG::gfx::D2DBackend_DX12& d2d, UINT id, float width, float heigt, std::wstring title);
      ~UIBackground();
      void Draw(DOG::gfx::D2DBackend_DX12& d2d) override;
      void Update(DOG::gfx::D2DBackend_DX12& d2d) override;
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
      void Draw(DOG::gfx::D2DBackend_DX12& d2d) override;
      private:
      ComPtr<IDWriteTextFormat> m_textFormat;
      ComPtr<ID2D1SolidColorBrush> m_brush;
      D2D_RECT_F m_screenSize;

   };

}
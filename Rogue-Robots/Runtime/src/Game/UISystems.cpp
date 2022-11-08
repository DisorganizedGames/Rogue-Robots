#include "UISystems.h"
#include "../../../DOGEngine/src/Graphics/Rendering/UI.h"
#include "GameComponent.h"

using namespace DOG;


void UIUpdate()
{
   EntityManager& eMan = EntityManager::Get();
   eMan.Collect<PlayerStatsComponent, ThisPlayer>().Do([&](PlayerStatsComponent& stats, ThisPlayer&) {
      auto ui_i = UI::Get();
      auto hpbar = ui_i->GetUI<UIHealthBar>(hID);
      hpbar->SetBarValue(stats.health / stats.maxHealth);
});



}
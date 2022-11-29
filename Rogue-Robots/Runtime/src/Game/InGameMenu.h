#pragma once

class InGameMenu
{
public:
	static void Initialize(std::function<void(void)> resumeGameMenuCallback, std::function<void(void)> exitToMainMenuCallback, std::function<void(void)> exitToDesktopCallback);

	static void Open();
private:
	static inline u32 s_sceneID;
	static inline u32 s_backgroundID;
	static inline u32 s_resumeButtonID;
	static inline u32 s_exitToMainMenuButtonID;
	static inline u32 s_exitToDesktopButtonID;
};

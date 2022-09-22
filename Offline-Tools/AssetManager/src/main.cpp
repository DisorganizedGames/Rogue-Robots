#include "AssetManager.h"

int main(int argc, char** argv)
{
	CMP_InitFramework();

	WriteAssetFiles("../../assets", "../../asset_output");

	return 0;
}


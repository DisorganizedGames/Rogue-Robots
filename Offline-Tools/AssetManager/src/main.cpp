#include "AssetManager.h"

int main(int argc, char** argv)
{
	CMP_InitFramework();

	WriteAssetFiles("../../assets", "../../asset_output");

	/*
		Settings:
		{
			TextureEncoding: BC7		// Dx settings
			CompressionRate: ...		// zlib settings
		}
		
		CompressTexture(input, output.dog, settings)
	*/


	return 0;
}




int main()
{
	CMP_InitFramework();

	CMP_MipSet mipSet{};
	auto error = CMP_LoadTexture("../../assets/planet.png", &mipSet);

	if (error == CMP_OK)
	{
		std::cout << "Texture loaded!\n" << "Size: " << mipSet.m_nWidth << "x" << mipSet.m_nHeight << std::endl;
	}
	else
	{
		std::cerr << "Failed to load texture" << std::endl;
		return -1;
	}

	CMP_FreeMipSet(&mipSet);

	return 0;
}


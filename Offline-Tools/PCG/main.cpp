#include "WFC.h"

int main()
{
    uint32_t w = 6;
    uint32_t h = 4;
    uint32_t d = 5;

    std::string input = "largerTestOutput";

    //Create a WFC interface and send the input.
    WFC* wfc = new WFC(input + ".txt", w, h, d);

    //The generation has 20 chances to succeed.
    unsigned chances = 20;
    while (!wfc->GenerateLevel() && chances > 0)
    {
        chances--;
    }

    if (chances != 0)
    {
        //Output the generated level to a textfile.
        std::vector<std::string> generatedLevel = wfc->GetGeneratedLevel();

        std::ofstream output;
        output.open(input + "_generatedLevel.txt");

        for (uint32_t i{ 0u }; i < d; ++i)
        {
            for (uint32_t j{ 0u }; j < h; ++j)
            {
                for (uint32_t k{ 0u }; k < w; ++k)
                {
                    output << generatedLevel[i * h * w + j * w + k] << " ";
                }
                output << "\n";
            }
            output << "-\n";
        }
        output.close();
    }
    else
    {
        std::cout << "OUT OF TRIES!" << std::endl;
    }
    
    delete wfc;
}
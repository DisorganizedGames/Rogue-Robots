#include "WFC.h"

int main()
{
    //Dimensions for the whole level.
    uint32_t w = 50;
    uint32_t h = 20;
    uint32_t d = 50;

    //Number of levels to generate.
    uint32_t nrOfRooms = 7;

    std::string input = "largerTestOutput";

    //Create a WFC interface and send the input.
    WFC* wfc = new WFC(w, h, d);

    //Change so that we can setinput per room.
    wfc->SetInput(input + ".txt");

    //The generation has a certain amount of chances to succeed.
    unsigned chances = 100;
    while (!wfc->GenerateLevel(nrOfRooms) && chances > 0)
    {
        chances--;
    }
    if (chances != 0)
    {
        //Output the generated level to a textfile.
        std::vector<std::string> generatedLevel = wfc->GetGeneratedLevel();

        std::ofstream output;
        output.open("testRooms_generatedLevel.txt");

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